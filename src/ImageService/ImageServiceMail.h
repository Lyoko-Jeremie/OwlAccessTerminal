// jeremie

#ifndef OWLACCESSTERMINAL_IMAGESERVICEMAIL_H
#define OWLACCESSTERMINAL_IMAGESERVICEMAIL_H

#include <memory>
#include <functional>
#include <variant>
#include <tuple>
#include <utility>
#include <opencv2/opencv.hpp>
#include "../AsyncCallbackMailbox/AsyncCallbackMailbox.h"

namespace OwlMailDefine {

    enum class ControlCameraCmd {
        noop = 0,
        reset = 1,
    };

    struct Camera2Service;
    struct Service2Camera {
        int camera_id;

        // Serial2Cmd.runner = Cmd2Serial.callbackRunner
        std::function<void(std::shared_ptr<Camera2Service>)> callbackRunner;

        ControlCameraCmd cmd = ControlCameraCmd::noop;
        std::variant<bool, std::pair<int, int>> cmdParams{false};
    };
    struct Camera2Service {
        int camera_id;
        cv::Mat image;

        std::function<void(std::shared_ptr<Camera2Service>)> runner;
        bool ok = false;

        ControlCameraCmd cmd = ControlCameraCmd::noop;
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
