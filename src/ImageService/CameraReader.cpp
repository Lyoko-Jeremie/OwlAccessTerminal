// jeremie

#include "CameraReader.h"
#include <boost/asio/co_spawn.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/bind/bind.hpp>
#include <utility>

using boost::asio::use_awaitable;
#if defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)
# define use_awaitable \
  boost::asio::use_awaitable_t(__FILE__, __LINE__, __PRETTY_FUNCTION__)
#endif

namespace OwlCameraReader {

    CameraItem::CameraItem(boost::asio::strand<boost::asio::io_context::executor_type> strand,
                           OwlCameraConfig::CameraInfoTuple config) : strand_(std::move(strand)),
                                                                      id(std::get<0>(config)),
                                                                      path(std::get<1>(config)),
                                                                      api(OwlCameraConfig::string2VideoCaptureAPI(
                                                                              std::get<2>(config))),
                                                                      w(std::get<3>(config)),
                                                                      h(std::get<4>(config)) {
        vc = std::make_unique<cv::VideoCapture>();

        if (std::visit([this]<typename T>(T &a) {
            return vc->open(a, api);
        }, path)) {
            BOOST_LOG_OWL(info) << "CameraItem open ok : id " << id << " path "
                                << std::visit(OwlConfigLoader::helperCameraAddr2String, path);
            vc->set(cv::VideoCaptureProperties::CAP_PROP_FRAME_WIDTH, w);
            vc->set(cv::VideoCaptureProperties::CAP_PROP_FRAME_HEIGHT, h);
        } else {
            BOOST_LOG_OWL(error) << "CameraItem open error : id " << id << " path "
                                 << std::visit(OwlConfigLoader::helperCameraAddr2String, path);
        }
    }


    CameraReader::CameraReader(boost::asio::io_context &ioc,
                               boost::shared_ptr<OwlConfigLoader::ConfigLoader> config,
                               std::vector<OwlCameraConfig::CameraInfoTuple> camera_info_list,
                               OwlMailDefine::ServiceCameraMailbox &&mailbox_tcp_protobuf,
                               OwlMailDefine::ServiceCameraMailbox &&mailbox_http)
            : ioc_(ioc),
              config_(std::move(config)),
              camera_info_list_(std::move(camera_info_list)),
              mailbox_tcp_protobuf_(mailbox_tcp_protobuf),
              mailbox_http_(mailbox_http) {

        mailbox_tcp_protobuf_->receiveA2B([this](OwlMailDefine::MailService2Camera &&data) {
            if (data->cmd == OwlMailDefine::ControlCameraCmd::reset) {
                resetCamera(std::move(data), mailbox_tcp_protobuf_);
                return;
            }
//                BOOST_LOG_OWL(info) << "CameraReader mailbox_tcp_protobuf_->receiveA2B " << data->camera_id;
            getImage(std::move(data), mailbox_tcp_protobuf_);
        });
        mailbox_http_->receiveA2B([this](OwlMailDefine::MailService2Camera &&data) {
            if (data->cmd == OwlMailDefine::ControlCameraCmd::reset) {
                resetCamera(std::move(data), mailbox_http_);
                return;
            }
//                BOOST_LOG_OWL(info) << "CameraReader mailbox_http_->receiveA2B " << data->camera_id;
            getImage(std::move(data), mailbox_http_);
        });
    }

    void CameraReader::start() {
        boost::asio::dispatch(ioc_, [this, self = shared_from_this()]() {
            // make sure the construct of CameraItem run in self ioc
            std::lock_guard lg{mtx_camera_item_list_};
            for (auto &t: camera_info_list_) {
                camera_item_list_.push_back(boost::make_shared<CameraItem>(
                        boost::asio::make_strand(ioc_),
                        t
                ));
            }
        });
    }

    class CameraReaderGetImageCoImpl : public boost::enable_shared_from_this<CameraReaderGetImageCoImpl> {
    public:
        explicit CameraReaderGetImageCoImpl(boost::shared_ptr<CameraReader> parentPtr)
                : parentPtr_(std::move(parentPtr)), sleepTimer(parentPtr_->ioc_) {}

    private:
        boost::shared_ptr<CameraReader> parentPtr_;

        boost::system::error_code ec_{};

        boost::asio::steady_timer sleepTimer;
        cv::Mat img;
        cv::Mat imgGC;

        boost::asio::awaitable<bool> co_get_image(
                boost::shared_ptr<CameraReaderGetImageCoImpl> self,
                OwlMailDefine::MailService2Camera &&data,
                OwlMailDefine::ServiceCameraMailbox &mailbox
        ) {
            boost::ignore_unused(self);
            boost::ignore_unused(parentPtr_);

            BOOST_LOG_OWL(trace) << "co_get_image start";

            try {

                // switch io_context
                co_await boost::asio::dispatch(parentPtr_->ioc_, use_awaitable);
                BOOST_ASSERT(self);
                BOOST_ASSERT(parentPtr_);

                // make sure all the call to self and to CameraItem run in self ioc
                boost::shared_ptr<CameraItem> cc;
                std::unique_lock ul{parentPtr_->mtx_camera_item_list_};
                for (auto &c: parentPtr_->camera_item_list_) {
                    if (c->id == data->camera_id) {
                        cc = c;
                        // now we not need access `camera_item_list_` more
                        ul.unlock();
                        break;
                    }
                }
                if (!cc) {

                    // cannot find camera
                    BOOST_LOG_OWL(warning) << "co_get_image cannot find camera: " << data->camera_id;
                    OwlMailDefine::MailCamera2Service data_r = boost::make_shared<OwlMailDefine::Camera2Service>();
                    data_r->runner = data->callbackRunner;
                    data_r->camera_id = data->camera_id;
                    data_r->ok = false;
                    mailbox->sendB2A(std::move(data_r));
                    co_return false;

                } else {

                    // switch io_context
                    co_await boost::asio::dispatch(cc->strand_, use_awaitable);
                    BOOST_ASSERT(self);
                    BOOST_ASSERT(parentPtr_);

                    OwlMailDefine::MailCamera2Service data_r = boost::make_shared<OwlMailDefine::Camera2Service>();
                    data_r->runner = data->callbackRunner;
                    data_r->camera_id = data->camera_id;
                    if (!cc->isOpened()) {
                        data_r->ok = false;
                        BOOST_LOG_OWL(warning) << "co_get_image (!c->isOpened()) cannot open: "
                                               << data->camera_id;
                        mailbox->sendB2A(std::move(data_r));
                        co_return false;
                    } else {
//                        BOOST_LOG_OWL(trace) << "co_get_image duration " <<
//                                             std::chrono::duration_cast<std::chrono::milliseconds>(
//                                                     std::chrono::steady_clock::now() - cc->lastRead).count();
                        if ((std::chrono::steady_clock::now() - cc->lastRead) <
                            std::chrono::milliseconds(parentPtr_->config_->config().camera_read_max_ms)) {
                            // read the image
                            if (!cc->vc->read(img)) {
                                // `false` if no frames has been grabbed
                                data_r->ok = false;
                                BOOST_LOG_OWL(warning)
                                    << "co_get_image (!c->vc->read(img)) read frame fail: "
                                    << data->camera_id;
                            } else {
                                data_r->image = img;
                                data_r->ok = true;
                                if (img.empty()) {
                                    data_r->ok = false;
                                    BOOST_LOG_OWL(warning) << "co_get_image (img.empty()) read frame fail: "
                                                           << data->camera_id;
                                }
                                // update lastRead
                                cc->lastRead = std::chrono::steady_clock::now();
                            }
                            // send
                            mailbox->sendB2A(std::move(data_r));
                            co_return true;
                        } else {
                            BOOST_LOG_OWL(trace) << "co_get_image camera cache is outdated";
                            // camera cache is outdated
                            // now we need trigger a pre-read to clear buffer

                            auto camera_read_retry_times = parentPtr_->config_->config().camera_read_retry_times;
                            auto camera_read_retry_ms = parentPtr_->config_->config().camera_read_retry_ms;
                            for (int i = 0; i < camera_read_retry_times; ++i) {
                                {
                                    // read the old image to clear buffer and trigger read new image from camera
                                    if (!cc->vc->read(imgGC)) {
                                        // `false` if no frames has been grabbed
                                        data_r->ok = false;
                                        // we dont care it read ok or not when pre-read
                                    }
                                    imgGC.release();
                                }

                                // now , wait a moment
                                sleepTimer.expires_from_now(std::chrono::milliseconds(camera_read_retry_ms));
                                co_await sleepTimer.async_wait(boost::asio::redirect_error(use_awaitable, ec_));
                                BOOST_ASSERT(self);
                                BOOST_ASSERT(parentPtr_);
                                if (ec_) {
                                    if (ec_ == boost::asio::error::operation_aborted) {
                                        // terminal
                                        BOOST_LOG_OWL(warning) << "co_get_image sleepTimer operation_aborted";
                                        co_return false;
                                    }
                                    // Timer expired. means it ok.
                                }
                            }

                            // switch io_context
                            co_await boost::asio::dispatch(cc->strand_, use_awaitable);
                            BOOST_ASSERT(self);
                            BOOST_ASSERT(parentPtr_);

                            // read the image
                            if (!cc->vc->read(img)) {
                                // `false` if no frames has been grabbed
                                data_r->ok = false;
                                BOOST_LOG_OWL(warning)
                                    << "co_get_image (!c->vc->read(img)) read frame fail: "
                                    << data->camera_id;
                            } else {
                                data_r->image = img;
                                data_r->ok = true;
                                if (img.empty()) {
                                    data_r->ok = false;
                                    BOOST_LOG_OWL(warning) << "co_get_image (img.empty()) read frame fail: "
                                                           << data->camera_id;
                                }
                                // update lastRead
                                cc->lastRead = std::chrono::steady_clock::now();
                            }
                            mailbox->sendB2A(std::move(data_r));
                            co_return true;
                        }
                    }
                }


                // ================================ ............ ================================
                // ================================ ............ ================================
                // ================================ ............ ================================
                // ================================ ............ ================================
                // ================================ ............ ================================


                BOOST_ASSERT(self);
                BOOST_ASSERT(parentPtr_);
                boost::ignore_unused(self);
                boost::ignore_unused(parentPtr_);
                co_return true;

            } catch (const std::exception &e) {
                BOOST_LOG_OWL(error) << "co_get_image catch (const std::exception &e)" << e.what();
                throw;
//                co_return false;
            }

            boost::ignore_unused(self);
            boost::ignore_unused(parentPtr_);
            co_return true;
        }

    public:
        void getImage(OwlMailDefine::MailService2Camera &&data, OwlMailDefine::ServiceCameraMailbox &mailbox) {
            boost::asio::co_spawn(
                    parentPtr_->ioc_,
                    [this, self = shared_from_this(), &data, &mailbox]() {
                        return co_get_image(self, std::move(data), mailbox);
                    },
                    [this, self = shared_from_this()](std::exception_ptr e, bool r) {

                        if (!e) {
                            if (r) {
                                BOOST_LOG_OWL(trace) << "CameraReaderGetImageCoImpl run() ok";
                                return;
                            } else {
                                BOOST_LOG_OWL(trace) << "CameraReaderGetImageCoImpl run() error";
                                return;
                            }
                        } else {
                            std::string what;
                            // https://stackoverflow.com/questions/14232814/how-do-i-make-a-call-to-what-on-stdexception-ptr
                            try { std::rethrow_exception(std::move(e)); }
                            catch (const std::exception &e) {
                                BOOST_LOG_OWL(error) << "CameraReaderGetImageCoImpl co_spawn catch std::exception "
                                                     << e.what();
                                what = e.what();
                            }
                            catch (const std::string &e) {
                                BOOST_LOG_OWL(error) << "CameraReaderGetImageCoImpl co_spawn catch std::string " << e;
                                what = e;
                            }
                            catch (const char *e) {
                                BOOST_LOG_OWL(error) << "CameraReaderGetImageCoImpl co_spawn catch char *e " << e;
                                what = std::string{e};
                            }
                            catch (...) {
                                BOOST_LOG_OWL(error) << "CameraReaderGetImageCoImpl co_spawn catch (...)"
                                                     << "\n" << boost::current_exception_diagnostic_information();
                                what = boost::current_exception_diagnostic_information();
                            }
                            BOOST_LOG_OWL(error) << "CameraReaderGetImageCoImpl what " << what;

                        }

                    });
        }

    };

    void
    CameraReader::getImage(OwlMailDefine::MailService2Camera &&data, OwlMailDefine::ServiceCameraMailbox &mailbox) {
        boost::make_shared<CameraReaderGetImageCoImpl>(shared_from_this())->getImage(std::move(data), mailbox);
    }

    void
    CameraReader::resetCamera(OwlMailDefine::MailService2Camera &&data, OwlMailDefine::ServiceCameraMailbox &mailbox) {
        boost::asio::dispatch(ioc_, [this, self = shared_from_this(), data, &mailbox]() {

            {
                std::lock_guard lg{mtx_camera_item_list_};

                auto it1 = std::find_if(camera_item_list_.begin(), camera_item_list_.end(),
                                        [&data](boost::shared_ptr<CameraItem> &a) -> bool {
                                            return a->id == data->camera_id;
                                        });
                auto it2 = std::find_if(camera_info_list_.begin(), camera_info_list_.end(),
                                        [&data](OwlCameraConfig::CameraInfoTuple &a) -> bool {
                                            return std::get<0>(a) == data->camera_id;
                                        });

                if (it2 == camera_info_list_.end()) {
                    // // cannot fine the camera config, means that the camera not exist
                    // OwlMailDefine::MailCamera2Service data_r = boost::make_shared<OwlMailDefine::Camera2Service>();
                    // data_r->runner = data->callbackRunner;
                    // data_r->camera_id = data->camera_id;
                    // data_r->ok = false;
                    // data_r->cmd = data->cmd;
                    // mailbox->sendB2A(std::move(data_r));

                    // create new camera
                    camera_info_list_.push_back(std::get<1>(data->cmdParams));
                    camera_item_list_.push_back(boost::make_shared<CameraItem>(
                            boost::asio::make_strand(ioc_),
                            std::get<1>(data->cmdParams)
                    ));
                    return;
                }

                if (it1 != camera_item_list_.end()) {
                    // close camera
                    (*it1)->close();
                    // remove old camera
                    camera_item_list_.erase(it1);
                    it1 = camera_item_list_.end();
                }

                // re-config camera config
                //  camera_info_list_.tuple(x,y) = data->cmdParams.variant[<x,y>].tuple(id, CameraAddr, VideoCaptureAPI, w, h)
                std::get<3>(*it2) = std::get<3>(std::get<1>(data->cmdParams));
                std::get<3>(*it2) = std::get<3>(std::get<1>(data->cmdParams));

                //                      id, CameraAddr, VideoCaptureAPI, w, h
                if (std::get<2>(std::get<1>(data->cmdParams)) != OwlConfigLoader::Camera_VideoCaptureAPI_Placeholder) {
                    std::get<2>(*it2) = std::get<2>(std::get<1>(data->cmdParams));
                }

                auto addr = std::get<1>(std::get<1>(data->cmdParams));
                if (std::holds_alternative<OwlConfigLoader::CameraAddrType_1>(addr) &&
                    std::get<0>(addr) != OwlConfigLoader::CameraAddrType_1_Placeholder) {
                    //  path = CameraAddr
                    std::get<1>(*it2) = std::get<0>(addr);
                }
                if (std::holds_alternative<OwlConfigLoader::CameraAddrType_2>(addr) &&
                    std::get<1>(addr) != OwlConfigLoader::CameraAddrType_2_Placeholder) {
                    //  path = CameraAddr
                    std::get<1>(*it2) = std::get<0>(addr);
                }

                // create new camera
                camera_item_list_.push_back(boost::make_shared<CameraItem>(
                        boost::asio::make_strand(ioc_),
                        *it2
                ));

            }

            OwlMailDefine::MailCamera2Service data_r = boost::make_shared<OwlMailDefine::Camera2Service>();
            data_r->runner = data->callbackRunner;
            data_r->camera_id = data->camera_id;
            data_r->ok = true;
            data_r->cmd = data->cmd;
            mailbox->sendB2A(std::move(data_r));
            return;
        });
    }

} // OwlCameraReader