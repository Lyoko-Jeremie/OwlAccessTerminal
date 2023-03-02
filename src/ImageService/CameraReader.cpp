// jeremie

#include "CameraReader.h"

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
                               std::vector<OwlCameraConfig::CameraInfoTuple> camera_info_list,
                               OwlMailDefine::ServiceCameraMailbox &&mailbox_tcp_protobuf,
                               OwlMailDefine::ServiceCameraMailbox &&mailbox_http) : ioc_(ioc),
                                                                                     camera_info_list_(std::move(
                                                                                             camera_info_list)),
                                                                                     mailbox_tcp_protobuf_(
                                                                                             mailbox_tcp_protobuf),
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

    void
    CameraReader::getImage(OwlMailDefine::MailService2Camera &&data, OwlMailDefine::ServiceCameraMailbox &mailbox) {
        boost::asio::dispatch(ioc_, [this, self = shared_from_this(), data, &mailbox]() {
            // make sure all the call to self and to CameraItem run in self ioc
            {
                std::unique_lock ul{mtx_camera_item_list_};
                for (auto &c: camera_item_list_) {
                    if (c->id == data->camera_id) {
                        auto cc = c;
                        // now we not need access `camera_item_list_` more
                        ul.unlock();
                        boost::asio::dispatch(cc->strand_, [this, self = shared_from_this(), data, &mailbox, cc]() {
                            OwlMailDefine::MailCamera2Service data_r = boost::make_shared<OwlMailDefine::Camera2Service>();
                            data_r->runner = data->callbackRunner;
                            data_r->camera_id = data->camera_id;
                            if (!cc->isOpened()) {
                                data_r->ok = false;
                                BOOST_LOG_OWL(warning) << "getImage (!c->isOpened()) cannot open: "
                                                       << data->camera_id;
                            } else {
                                // read the image
                                cv::Mat img;
                                if (!cc->vc->read(img)) {
                                    // `false` if no frames has been grabbed
                                    data_r->ok = false;
                                    BOOST_LOG_OWL(warning) << "getImage (!c->vc->read(img)) read frame fail: "
                                                           << data->camera_id;
                                } else {
                                    data_r->image = img;
                                    data_r->ok = true;
                                    if (img.empty()) {
                                        data_r->ok = false;
                                        BOOST_LOG_OWL(warning) << "getImage (img.empty()) read frame fail: "
                                                               << data->camera_id;
                                    }
                                }
                            }
                            mailbox->sendB2A(std::move(data_r));
                        });
                        return;
                    }
                }
            }
            // cannot find camera
            BOOST_LOG_OWL(warning) << "getImage cannot find camera: " << data->camera_id;
            OwlMailDefine::MailCamera2Service data_r = boost::make_shared<OwlMailDefine::Camera2Service>();
            data_r->runner = data->callbackRunner;
            data_r->camera_id = data->camera_id;
            data_r->ok = false;
            mailbox->sendB2A(std::move(data_r));
            return;
        });
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