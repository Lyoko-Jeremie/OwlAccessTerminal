// jeremie

#ifndef OWLACCESSTERMINAL_CAMERAREADER_H
#define OWLACCESSTERMINAL_CAMERAREADER_H

#include <memory>
#include <utility>
#include <vector>
#include <opencv2/opencv.hpp>
#include <boost/log/trivial.hpp>
#include "ImageServiceMail.h"

namespace OwlCameraReader {

    struct CameraItem : public std::enable_shared_from_this<CameraItem> {
        int id;
        int path;
        std::unique_ptr<cv::VideoCapture> vc;

        CameraItem(
                int id,
                int path
        ) : id(id), path(path) {
            vc = std::make_unique<cv::VideoCapture>();
            // TODO test this
            if (vc->open(path)) {
                BOOST_LOG_TRIVIAL(info) << "CameraItem open ok : id " << id << " path " << path;
            } else {
                BOOST_LOG_TRIVIAL(error) << "CameraItem open error : id " << id << " path " << path;
            }
        }

        bool isOpened() const {
            return vc && vc->isOpened();
        }

    };

    class CameraReader : public std::enable_shared_from_this<CameraReader> {
    public:
        CameraReader(
                boost::asio::io_context &ioc,
                std::vector<std::pair<int, int>> camera_info_list,
                OwlMailDefine::ServiceCameraMailbox &&mailbox_tcp_protobuf,
                OwlMailDefine::ServiceCameraMailbox &&mailbox_http
        ) : ioc_(ioc),
            camera_info_list_(std::move(camera_info_list)),
            mailbox_tcp_protobuf_(mailbox_tcp_protobuf),
            mailbox_http_(mailbox_http) {

            mailbox_tcp_protobuf_->receiveA2B = [this](OwlMailDefine::MailService2Camera &&data) {
                getImage(std::move(data), mailbox_tcp_protobuf_);
            };
            mailbox_http_->receiveA2B = [this](OwlMailDefine::MailService2Camera &&data) {
                getImage(std::move(data), mailbox_http_);
            };
        }

        ~CameraReader() {
            mailbox_tcp_protobuf_->receiveA2B = nullptr;
            mailbox_http_->receiveA2B = nullptr;
        }

    private:
        boost::asio::io_context &ioc_;
        std::vector<std::pair<int, int>> camera_info_list_;
        std::vector<std::shared_ptr<CameraItem>> camera_item_list_;
        OwlMailDefine::ServiceCameraMailbox mailbox_tcp_protobuf_;
        OwlMailDefine::ServiceCameraMailbox mailbox_http_;
    public:
        void
        start() {
            boost::asio::dispatch(ioc_, [this, self = shared_from_this()]() {
                // make sure the construct of CameraItem run in self ioc
                for (auto &t: camera_info_list_) {
                    camera_item_list_.push_back(std::make_shared<CameraItem>(
                            t.first,
                            t.second
                    ));
                }
            });
        }

    private:

        void getImage(OwlMailDefine::MailService2Camera &&data, OwlMailDefine::ServiceCameraMailbox &mailbox) {
            boost::asio::dispatch(ioc_, [this, self = shared_from_this(), data, &mailbox]() {
                // make sure all the call to self and to CameraItem run in self ioc
                OwlMailDefine::MailCamera2Service data_r;
                data_r->runner = data->callbackRunner;
                data_r->camera_id = data->camera_id;
                for (auto &c: camera_item_list_) {
                    if (c->id == data->camera_id) {
                        if (!c->isOpened()) {
                            data_r->ok = false;
                            BOOST_LOG_TRIVIAL(warning) << "getImage (!c->isOpened()) cannot open: " << data->camera_id;
                        } else {
                            // read the image
                            cv::Mat img;
                            if (!c->vc->read(img)) {
                                // `false` if no frames has been grabbed
                                data_r->ok = false;
                                BOOST_LOG_TRIVIAL(warning) << "getImage (!c->vc->read(img)) read frame fail: "
                                                           << data->camera_id;
                            } else {
                                data_r->image = img;
                                data_r->ok = true;
                                if (img.empty()) {
                                    data_r->ok = false;
                                    BOOST_LOG_TRIVIAL(warning) << "getImage (img.empty()) read frame fail: "
                                                               << data->camera_id;
                                }
                            }
                        }
                        mailbox->sendB2A(std::move(data_r));
                        return;
                    }
                }
                // cannot find camera
                BOOST_LOG_TRIVIAL(warning) << "getImage cannot find camera: " << data->camera_id;
                data_r->ok = false;
                mailbox->sendB2A(std::move(data_r));
                return;
            });
        }

    };

} // OwlCameraReader

#endif //OWLACCESSTERMINAL_CAMERAREADER_H
