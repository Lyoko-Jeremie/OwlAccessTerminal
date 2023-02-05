// jeremie

#ifndef OWLACCESSTERMINAL_CAMERAREADER_H
#define OWLACCESSTERMINAL_CAMERAREADER_H

#include <memory>
#include <utility>
#include <tuple>
#include <vector>
#include <mutex>
#include <opencv2/opencv.hpp>
#include <boost/log/trivial.hpp>
#include "ImageServiceMail.h"
#include "../ConfigLoader/ConfigLoader.h"

namespace OwlCameraReader {

    struct CameraItem : public std::enable_shared_from_this<CameraItem> {
        boost::asio::strand<boost::asio::io_context::executor_type> strand_;
        int id;
        OwlConfigLoader::CameraAddrType path;
        OwlCameraConfig::VideoCaptureAPIs api;
        int w;
        int h;

        std::unique_ptr<cv::VideoCapture> vc;

        explicit CameraItem(
                boost::asio::strand<boost::asio::io_context::executor_type> strand,
                OwlCameraConfig::CameraInfoTuple config
        ) : strand_(std::move(strand)),
            id(std::get<0>(config)),
            path(std::get<1>(config)),
            api(OwlCameraConfig::string2VideoCaptureAPI(std::get<2>(config))),
            w(std::get<3>(config)),
            h(std::get<4>(config)) {
            vc = std::make_unique<cv::VideoCapture>();

            if (std::visit([this]<typename T>(T &a) {
                return vc->open(a, api);
            }, path)) {
                BOOST_LOG_TRIVIAL(info) << "CameraItem open ok : id " << id << " path "
                                        << std::visit(OwlConfigLoader::helperCameraAddr2String, path);
                vc->set(cv::VideoCaptureProperties::CAP_PROP_FRAME_WIDTH, w);
                vc->set(cv::VideoCaptureProperties::CAP_PROP_FRAME_HEIGHT, h);
            } else {
                BOOST_LOG_TRIVIAL(error) << "CameraItem open error : id " << id << " path "
                                         << std::visit(OwlConfigLoader::helperCameraAddr2String, path);
            }
        }

        bool isOpened() const {
            return vc && vc->isOpened();
        }

        void close() {
            if (isOpened()) {
                vc->release();
            }
        }

    };

    class CameraReader : public std::enable_shared_from_this<CameraReader> {
    public:
        CameraReader(
                boost::asio::io_context &ioc,
                std::vector<OwlCameraConfig::CameraInfoTuple> camera_info_list,
                OwlMailDefine::ServiceCameraMailbox &&mailbox_tcp_protobuf,
                OwlMailDefine::ServiceCameraMailbox &&mailbox_http
        );

        ~CameraReader() {
            mailbox_tcp_protobuf_->receiveA2B = nullptr;
            mailbox_http_->receiveA2B = nullptr;
        }

    private:
        boost::asio::io_context &ioc_;
        std::vector<OwlCameraConfig::CameraInfoTuple> camera_info_list_;
        std::vector<std::shared_ptr<CameraItem>> camera_item_list_;
        std::mutex mtx_camera_item_list_;
        OwlMailDefine::ServiceCameraMailbox mailbox_tcp_protobuf_;
        OwlMailDefine::ServiceCameraMailbox mailbox_http_;
    public:
        void
        start();

    private:

        void getImage(OwlMailDefine::MailService2Camera &&data, OwlMailDefine::ServiceCameraMailbox &mailbox);

        void resetCamera(OwlMailDefine::MailService2Camera &&data, OwlMailDefine::ServiceCameraMailbox &mailbox);

    };

} // OwlCameraReader

#endif //OWLACCESSTERMINAL_CAMERAREADER_H
