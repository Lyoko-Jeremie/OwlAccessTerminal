// jeremie

#ifndef OWLACCESSTERMINAL_CAMERAREADER_H
#define OWLACCESSTERMINAL_CAMERAREADER_H

#include "../MemoryBoost.h"
#include <utility>
#include <tuple>
#include <vector>
#include <mutex>
#include <chrono>
#include <atomic>
#include <mutex>
#include <opencv2/opencv.hpp>
#include "../OwlLog/OwlLog.h"
#include "ImageServiceMail.h"
#include "../ConfigLoader/ConfigLoader.h"

namespace OwlCameraReader {

    class CameraReaderGetImageCoImpl;

    struct CameraItem : public boost::enable_shared_from_this<CameraItem> {
        boost::asio::strand<boost::asio::io_context::executor_type> strand_;
        int id;
        OwlConfigLoader::CameraAddrType path;
        OwlCameraConfig::VideoCaptureAPIs api;
        int w;
        int h;

        // init as 0
        std::atomic<std::chrono::steady_clock::time_point> lastRead{};

    private:
//        std::mutex mtx_vc;
        std::unique_ptr<cv::VideoCapture> vc;
    public:


        bool grab() {
            return vc && vc->isOpened() && vc->grab();
        }

        bool read(cv::Mat &image) {
//            std::lock_guard g{mtx_vc};
            return vc && vc->isOpened() && vc->read(image);
        }

        explicit CameraItem(
                boost::asio::strand<boost::asio::io_context::executor_type> strand,
                OwlCameraConfig::CameraInfoTuple config
        );

        bool isOpened() {
//            std::lock_guard g{mtx_vc};
            return vc && vc->isOpened();
        }

        void close() {
//            std::lock_guard g{mtx_vc};
            if (vc && vc->isOpened()) {
                vc->release();
            }
        }

    };

    class CameraReader : public boost::enable_shared_from_this<CameraReader> {
    public:
        CameraReader(
                boost::asio::io_context &ioc,
                boost::shared_ptr<OwlConfigLoader::ConfigLoader> config,
                std::vector<OwlCameraConfig::CameraInfoTuple> camera_info_list,
                OwlMailDefine::ServiceCameraMailbox &&mailbox_tcp_protobuf,
                OwlMailDefine::ServiceCameraMailbox &&mailbox_http
        );

        ~CameraReader() {
            BOOST_LOG_OWL(trace_dtor) << "~CameraReader()";
            mailbox_tcp_protobuf_->receiveA2B(nullptr);
            mailbox_http_->receiveA2B(nullptr);
        }

    private:
        boost::asio::io_context &ioc_;
        boost::shared_ptr<OwlConfigLoader::ConfigLoader> config_;
        std::vector<OwlCameraConfig::CameraInfoTuple> camera_info_list_;
        std::vector<boost::shared_ptr<CameraItem>> camera_item_list_;
        std::mutex mtx_camera_item_list_;
        OwlMailDefine::ServiceCameraMailbox mailbox_tcp_protobuf_;
        OwlMailDefine::ServiceCameraMailbox mailbox_http_;

        friend class CameraReaderGetImageCoImpl;

        friend class CameraReaderGetImageImpl;

    public:
        void
        start();

    private:

        void getImage(OwlMailDefine::MailService2Camera &&data, OwlMailDefine::ServiceCameraMailbox &mailbox);

        void resetCamera(OwlMailDefine::MailService2Camera &&data, OwlMailDefine::ServiceCameraMailbox &mailbox);

    };

} // OwlCameraReader

#endif //OWLACCESSTERMINAL_CAMERAREADER_H
