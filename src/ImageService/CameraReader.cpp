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

        std::lock_guard g{mtx_vc};
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
        explicit CameraReaderGetImageCoImpl(
                boost::shared_ptr<CameraReader> parentPtr,
                OwlMailDefine::MailService2Camera &&data,
                OwlMailDefine::ServiceCameraMailbox &mailbox
        ) : parentPtr_(std::move(parentPtr)),
            ioc_st_(boost::asio::make_strand(parentPtr_->ioc_)),
            data_(data),
            mailbox_(mailbox),
            sleepTimer(ioc_st_) {}

    private:
        boost::shared_ptr<CameraReader> parentPtr_;
        boost::asio::strand<boost::asio::io_context::executor_type> ioc_st_;

        OwlMailDefine::MailService2Camera data_;
        OwlMailDefine::ServiceCameraMailbox &mailbox_;

        boost::system::error_code ec_{};

        boost::asio::steady_timer sleepTimer;
        cv::Mat img;
        cv::Mat imgGC;

        boost::asio::awaitable<bool> co_get_image(
                boost::shared_ptr<CameraReaderGetImageCoImpl> self
        ) {
            boost::ignore_unused(self);
            boost::ignore_unused(parentPtr_);
            BOOST_ASSERT(data_);
            BOOST_ASSERT(mailbox_);
            BOOST_ASSERT(self);
            BOOST_ASSERT(parentPtr_);

            BOOST_LOG_OWL(trace_camera_reader) << "co_get_image start";

            try {

                // switch io_context
                co_await boost::asio::dispatch(ioc_st_, use_awaitable);
                BOOST_ASSERT(data_);
                BOOST_ASSERT(mailbox_);
                BOOST_ASSERT(self);
                BOOST_ASSERT(parentPtr_);

                // make sure all the call to self and to CameraItem run in self ioc
                boost::shared_ptr<CameraItem> cc;
                {
                    std::lock_guard ul{parentPtr_->mtx_camera_item_list_};
                    for (auto &c: parentPtr_->camera_item_list_) {
                        if (c->id == data_->camera_id) {
                            cc = c;
                            break;
                        }
                    }
                }
                if (!cc) {

                    BOOST_ASSERT(data_);
                    BOOST_ASSERT(mailbox_);
                    BOOST_ASSERT(self);
                    BOOST_ASSERT(parentPtr_);
                    // cannot find camera
                    BOOST_LOG_OWL(warning) << "co_get_image cannot find camera: " << data_->camera_id;
                    OwlMailDefine::MailCamera2Service data_r = boost::make_shared<OwlMailDefine::Camera2Service>();
                    data_r->runner = data_->callbackRunner;
                    data_r->camera_id = data_->camera_id;
                    data_r->ok = false;
                    mailbox_->sendB2A(std::move(data_r));
                    co_return false;

                    // ================================ ............ ================================
                } else {
                    BOOST_ASSERT(cc);
                    BOOST_ASSERT(data_);
                    BOOST_ASSERT(mailbox_);
                    BOOST_ASSERT(self);
                    BOOST_ASSERT(parentPtr_);

                    // switch io_context
                    co_await boost::asio::dispatch(ioc_st_, use_awaitable);
                    BOOST_ASSERT(cc);
                    BOOST_ASSERT(data_);
                    BOOST_ASSERT(mailbox_);
                    BOOST_ASSERT(self);
                    BOOST_ASSERT(parentPtr_);

                    OwlMailDefine::MailCamera2Service data_r = boost::make_shared<OwlMailDefine::Camera2Service>();
                    data_r->runner = data_->callbackRunner;
                    data_r->camera_id = data_->camera_id;
                    if (!cc->isOpened()) {
                        BOOST_ASSERT(cc);
                        BOOST_ASSERT(data_);
                        BOOST_ASSERT(mailbox_);
                        BOOST_ASSERT(self);
                        BOOST_ASSERT(parentPtr_);
                        data_r->ok = false;
                        BOOST_LOG_OWL(warning) << "co_get_image (!c->isOpened()) cannot open: "
                                               << data_->camera_id;
                        mailbox_->sendB2A(std::move(data_r));
                        co_return false;

                        // ================================ ............ ================================
                    } else {
                        BOOST_ASSERT(cc);
                        BOOST_ASSERT(data_);
                        BOOST_ASSERT(mailbox_);
                        BOOST_ASSERT(self);
                        BOOST_ASSERT(parentPtr_);

                        BOOST_LOG_OWL(trace_camera_reader)
                            << " read_camera() "
                            << ((std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::steady_clock::now() - cc->lastRead.load()).count()
                                 < std::chrono::milliseconds(parentPtr_->config_->config().camera_read_max_ms).count())
                                || (data_->dont_retry))
                            << " dont_retry " << data_->dont_retry
                            << " time " << (std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::steady_clock::now() - cc->lastRead.load())).count()
                            << " limit "
                            << std::chrono::milliseconds(parentPtr_->config_->config().camera_read_max_ms).count();

                        if (((std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::steady_clock::now() - cc->lastRead.load())).count() <
                             std::chrono::milliseconds(parentPtr_->config_->config().camera_read_max_ms).count())
                            || (data_->dont_retry)) {
                            if (data_->dont_retry) {
                                // we dont need re-read
                                // example use android, or tag
                            }
                            BOOST_ASSERT(cc);
                            BOOST_ASSERT(data_);
                            BOOST_ASSERT(mailbox_);
                            BOOST_ASSERT(self);
                            BOOST_ASSERT(parentPtr_);
                            // read the image
                            if (!cc->read(img)) {
                                // `false` if no frames has been grabbed
                                data_r->ok = false;
                                BOOST_LOG_OWL(warning)
                                    << "co_get_image (!cc->read(img)) read frame fail: "
                                    << data_->camera_id;
                                mailbox_->sendB2A(std::move(data_r));
                                co_return false;
                            } else {
                                data_r->image = img;
                                data_r->ok = true;
                                if (img.empty()) {
                                    data_r->ok = false;
                                    BOOST_LOG_OWL(warning) << "co_get_image (img.empty()) read frame fail: "
                                                           << data_->camera_id;
                                }
                                // update lastRead
                                cc->lastRead = std::chrono::steady_clock::now();
                            }
                            // send
                            mailbox_->sendB2A(std::move(data_r));
                            co_return true;

                            // ================================ ............ ================================
                        } else {
                            BOOST_ASSERT(cc);
                            BOOST_ASSERT(data_);
                            BOOST_ASSERT(mailbox_);
                            BOOST_ASSERT(self);
                            BOOST_ASSERT(parentPtr_);
                            BOOST_LOG_OWL(trace_camera_reader) << "co_get_image camera cache is outdated";
                            // camera cache is outdated
                            // now we need trigger a pre-read to clear buffer

                            auto camera_read_retry_times = parentPtr_->config_->config().camera_read_retry_times;
                            auto camera_read_retry_ms = parentPtr_->config_->config().camera_read_retry_ms;
                            for (int i = 0; i < camera_read_retry_times; ++i) {
                                {
                                    BOOST_ASSERT(cc);
                                    BOOST_ASSERT(data_);
                                    BOOST_ASSERT(mailbox_);
                                    BOOST_ASSERT(self);
                                    BOOST_ASSERT(parentPtr_);
                                    // read the old image to clear buffer and trigger read new image from camera
                                    if (!cc->read(imgGC)) {
                                        // `false` if no frames has been grabbed
                                        data_r->ok = false;
                                        // we dont care it read ok or not when pre-read
                                    }
                                }

                                // now , wait a moment
                                sleepTimer.expires_from_now(std::chrono::milliseconds(camera_read_retry_ms));
                                co_await sleepTimer.async_wait(boost::asio::redirect_error(use_awaitable, ec_));
                                BOOST_ASSERT(cc);
                                BOOST_ASSERT(data_);
                                BOOST_ASSERT(mailbox_);
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

                            // ================================ ............ ================================

                            // switch io_context
                            co_await boost::asio::dispatch(ioc_st_, use_awaitable);
                            BOOST_ASSERT(cc);
                            BOOST_ASSERT(data_);
                            BOOST_ASSERT(mailbox_);
                            BOOST_ASSERT(self);
                            BOOST_ASSERT(parentPtr_);

                            // read the image
                            if (!cc->read(img)) {
                                // `false` if no frames has been grabbed
                                data_r->ok = false;
                                BOOST_LOG_OWL(warning)
                                    << "co_get_image (!cc->read(img)) read frame fail: "
                                    << data_->camera_id;
                                mailbox_->sendB2A(std::move(data_r));
                                co_return false;
                            } else {
                                BOOST_ASSERT(cc);
                                BOOST_ASSERT(data_);
                                BOOST_ASSERT(mailbox_);
                                BOOST_ASSERT(self);
                                BOOST_ASSERT(parentPtr_);
                                data_r->image = img;
                                data_r->ok = true;
                                if (img.empty()) {
                                    data_r->ok = false;
                                    BOOST_LOG_OWL(warning) << "co_get_image (img.empty()) read frame fail: "
                                                           << data_->camera_id;
                                }
                                // update lastRead
                                cc->lastRead = std::chrono::steady_clock::now();
                            }
                            mailbox_->sendB2A(std::move(data_r));
                            co_return true;

                            // ================================ ............ ================================
                        }
                    }
                }


                // ================================ ............ ================================
                // ================================ ............ ================================
                // ================================ ............ ================================
                // ================================ ............ ================================
                // ================================ ............ ================================


                BOOST_ASSERT(data_);
                BOOST_ASSERT(mailbox_);
                BOOST_ASSERT(self);
                BOOST_ASSERT(parentPtr_);
                boost::ignore_unused(self);
                boost::ignore_unused(parentPtr_);
                co_return true;

            } catch (const std::exception &e) {
                BOOST_LOG_OWL(error) << "co_get_image catch (const std::exception &e)" << e.what();
                throw;
            }

            boost::ignore_unused(self);
            boost::ignore_unused(parentPtr_);
            co_return true;
        }

    public:
        void getImage() {
            BOOST_ASSERT(shared_from_this());
            BOOST_ASSERT(parentPtr_);
            boost::asio::co_spawn(
                    ioc_st_,
                    [this, self = shared_from_this()]() {
                        return co_get_image(self);
                    },
                    [this, self = shared_from_this()](std::exception_ptr e, bool r) {
                        BOOST_ASSERT(self);
                        if (!e) {
                            if (r) {
                                BOOST_LOG_OWL(trace_camera_reader) << "CameraReaderGetImageCoImpl run() ok";
                                return;
                            } else {
                                BOOST_LOG_OWL(trace_camera_reader) << "CameraReaderGetImageCoImpl run() error";
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


    class CameraReaderGetImageImpl : public boost::enable_shared_from_this<CameraReaderGetImageImpl> {
    public:
        explicit CameraReaderGetImageImpl(boost::shared_ptr<CameraReader> parentPtr,
                                          OwlMailDefine::MailService2Camera &&data,
                                          OwlMailDefine::ServiceCameraMailbox &mailbox)
                : parentPtr_(std::move(parentPtr)),
                  ioc_st_(boost::asio::make_strand(parentPtr_->ioc_)),
                  data_(data),
                  mailbox_(mailbox),
                  sleepTimer(ioc_st_) {
            read_retry_times = parentPtr_->config_->config().camera_read_retry_times;
            camera_read_retry_ms = parentPtr_->config_->config().camera_read_retry_ms;
        }

    private:
        boost::shared_ptr<CameraReader> parentPtr_;
        boost::asio::strand<boost::asio::io_context::executor_type> ioc_st_;

        OwlMailDefine::MailService2Camera data_;
        OwlMailDefine::ServiceCameraMailbox &mailbox_;

        boost::asio::steady_timer sleepTimer;

    private:

        boost::shared_ptr<CameraItem> cc;

        int read_retry_times = 0;
        int camera_read_retry_ms = 0;

        cv::Mat img;
        cv::Mat imgGC;

    public:
        void getImage() {
            BOOST_ASSERT(data_);
            BOOST_ASSERT(mailbox_);
            BOOST_ASSERT(parentPtr_);
            boost::asio::dispatch(ioc_st_, [
                    this, self = shared_from_this()
            ]() {
                BOOST_ASSERT(self);
                BOOST_ASSERT(data_);
                BOOST_ASSERT(mailbox_);
                BOOST_ASSERT(parentPtr_);
                find_camera();
            });
        }

    private:

        void find_camera() {
            BOOST_ASSERT(data_);
            BOOST_ASSERT(mailbox_);
            BOOST_ASSERT(parentPtr_);
            {
                std::lock_guard ul{parentPtr_->mtx_camera_item_list_};
                for (auto &c: parentPtr_->camera_item_list_) {
                    if (c->id == data_->camera_id) {
                        cc = c;
                        break;
                    }
                }
            }
            if (!cc) {
                BOOST_ASSERT(data_);
                BOOST_ASSERT(mailbox_);
                BOOST_ASSERT(parentPtr_);
                // cannot find camera
                BOOST_LOG_OWL(warning) << "CameraReaderGetImageImpl find_camera cannot find camera: "
                                       << data_->camera_id;
                OwlMailDefine::MailCamera2Service data_r = boost::make_shared<OwlMailDefine::Camera2Service>();
                data_r->runner = data_->callbackRunner;
                data_r->camera_id = data_->camera_id;
                data_r->ok = false;
                mailbox_->sendB2A(std::move(data_r));
                return;

            } else {
                BOOST_ASSERT(data_);
                BOOST_ASSERT(mailbox_);
                BOOST_ASSERT(parentPtr_);
                BOOST_ASSERT(cc);
                if (!cc->isOpened()) {
                    BOOST_ASSERT(cc);
                    BOOST_ASSERT(data_);
                    BOOST_ASSERT(mailbox_);
                    BOOST_ASSERT(parentPtr_);
                    OwlMailDefine::MailCamera2Service data_r = boost::make_shared<OwlMailDefine::Camera2Service>();
                    data_r->runner = data_->callbackRunner;
                    data_r->camera_id = data_->camera_id;
                    data_r->ok = false;
                    BOOST_LOG_OWL(warning) << "CameraReaderGetImageImpl find_camera (!c->isOpened()) cannot open: "
                                           << data_->camera_id;
                    mailbox_->sendB2A(std::move(data_r));
                    return;

                    // ================================ ............ ================================
                } else {
                    read_camera();
                }
            }
        }

        void read_camera() {

            BOOST_ASSERT(cc);
            BOOST_ASSERT(data_);
            BOOST_ASSERT(mailbox_);
            BOOST_ASSERT(parentPtr_);

            BOOST_LOG_OWL(trace_camera_reader)
                << " read_camera() "
                << ((std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - cc->lastRead.load()).count()
                     < std::chrono::milliseconds(parentPtr_->config_->config().camera_read_max_ms).count())
                    || (data_->dont_retry))
                << " dont_retry " << data_->dont_retry
                << " time " << (std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - cc->lastRead.load())).count()
                << " limit "
                << std::chrono::milliseconds(parentPtr_->config_->config().camera_read_max_ms).count();

            if ((std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - cc->lastRead.load()).count()
                 < std::chrono::milliseconds(parentPtr_->config_->config().camera_read_max_ms).count())
                || (data_->dont_retry)) {
                if (data_->dont_retry) {
                    // we dont need re-read
                    // example use android, or tag
                }
                // direct read (dont_retry)
                direct_read();
            } else {
                // read with retry
                read_with_retry();
            }
        }

        void direct_read() {
            BOOST_ASSERT(cc);
            BOOST_ASSERT(data_);
            BOOST_ASSERT(mailbox_);
            BOOST_ASSERT(parentPtr_);
            OwlMailDefine::MailCamera2Service data_r = boost::make_shared<OwlMailDefine::Camera2Service>();
            data_r->runner = data_->callbackRunner;
            data_r->camera_id = data_->camera_id;
            // read the image
            if (!cc->read(img)) {
                // `false` if no frames has been grabbed
                data_r->ok = false;
                BOOST_LOG_OWL(warning)
                    << "CameraReaderGetImageImpl direct_read (!cc->read(img)) read frame fail: "
                    << data_->camera_id;
                mailbox_->sendB2A(std::move(data_r));
                return;
            } else {
                data_r->image = img;
                data_r->ok = true;
                if (img.empty()) {
                    data_r->ok = false;
                    BOOST_LOG_OWL(warning) << "CameraReaderGetImageImpl direct_read (img.empty()) read frame fail: "
                                           << data_->camera_id;
                }
                // update lastRead
                cc->lastRead = std::chrono::steady_clock::now();
            }
            // send
            mailbox_->sendB2A(std::move(data_r));
        }

        void read_with_retry() {
            BOOST_ASSERT(cc);
            BOOST_ASSERT(data_);
            BOOST_ASSERT(mailbox_);
            BOOST_ASSERT(parentPtr_);
            BOOST_LOG_OWL(trace_camera_reader) << "CameraReaderGetImageImpl read_with_retry camera cache is outdated";
            // camera cache is outdated
            // now we need trigger a pre-read to clear buffer
            pre_read_frame();
        }

        void pre_read_frame() {
            if (read_retry_times > 0) {
                sleepTimer.cancel();
                BOOST_ASSERT(cc);
                BOOST_ASSERT(data_);
                BOOST_ASSERT(mailbox_);
                BOOST_ASSERT(parentPtr_);
                // read the old image to clear buffer and trigger read new image from camera
                if (!cc->read(imgGC)) {
                    // `false` if no frames has been grabbed
                    // we dont care it read ok or not when pre-read
                }
                ++read_retry_times;
                // now , wait a moment
                sleepTimer.expires_from_now(std::chrono::milliseconds(camera_read_retry_ms));
                sleepTimer.async_wait([
                                              this, self = shared_from_this()
                                      ](boost::system::error_code ec) {
                    if (ec) {
                        if (ec == boost::asio::error::operation_aborted) {
                            // terminal
                            BOOST_LOG_OWL(warning)
                                << "CameraReaderGetImageImpl pre_read_frame sleepTimer operation_aborted";
                            return;
                        }
                        // Timer expired. means it ok.
                    }
                    pre_read_frame();
                });
            } else {
                sleepTimer.cancel();
                // now, read the 'true' frame
                direct_read();
            }
        }

    };

    void CameraReader::getImage(
            OwlMailDefine::MailService2Camera &&data,
            OwlMailDefine::ServiceCameraMailbox &mailbox) {
        BOOST_ASSERT(shared_from_this());
#ifdef UseCameraReaderGetImageCoImpl
        boost::make_shared<CameraReaderGetImageCoImpl>(shared_from_this(), std::move(data), mailbox)->getImage();
#else // UseCameraReaderGetImageImpl
        boost::make_shared<CameraReaderGetImageImpl>(shared_from_this(), std::move(data), mailbox)->getImage();
#endif
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