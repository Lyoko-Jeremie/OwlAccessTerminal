// jeremie

#ifndef OWLACCESSTERMINAL_IMAGESERVICEMAIL_H
#define OWLACCESSTERMINAL_IMAGESERVICEMAIL_H

#include <memory>
#include <functional>
#include <opencv2/opencv.hpp>
#include "../AsyncCallbackMailbox/AsyncCallbackMailbox.h"

namespace OwlMailDefine {
    struct Camera2Service;
    struct Service2Camera {
        int camera_id;

        // Serial2Cmd.runner = Cmd2Serial.callbackRunner
        std::function<void(std::shared_ptr<Camera2Service>)> callbackRunner;
    };
    struct Camera2Service {
        int camera_id;
        cv::Mat image;

        std::function<void(std::shared_ptr<Camera2Service>)> runner;
        bool ok = false;
    };
    using ServiceCameraMailbox =
            std::shared_ptr<
                    OwlAsyncCallbackMailbox::AsyncCallbackMailbox<
                            Service2Camera,
                            Camera2Service
                    >
            >;

    using MailService2Camera = ServiceCameraMailbox::element_type::A2B_t;
    using MailCamera2Service = ServiceCameraMailbox::element_type::B2A_t;

}


#endif //OWLACCESSTERMINAL_IMAGESERVICEMAIL_H
