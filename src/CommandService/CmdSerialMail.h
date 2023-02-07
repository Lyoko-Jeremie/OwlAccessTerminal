// jeremie

#ifndef OWLACCESSTERMINAL_CMDSERIALMAIL_H
#define OWLACCESSTERMINAL_CMDSERIALMAIL_H

#include <memory>
#include <functional>
#include <vector>
#include "../AsyncCallbackMailbox/AsyncCallbackMailbox.h"

namespace OwlMailDefine {
    enum class AdditionCmd {
        ignore = 0,
        takeoff = 1,
        land = 2,
        stop = 3,
        keep = 4,
        move = 10,
        rotate = 11,
        gotoPosition = 11,
        AprilTag = 100,
    };

    struct AprilTagInfo {
        // from :
        //      typedef struct apriltag_detection apriltag_detection_t;
        //      struct apriltag_detection
        int id;
        int hamming;
        float decision_margin;
        double centerX;
        double centerY;
        double cornerLTx;
        double cornerLTy;
        double cornerRTx;
        double cornerRTy;
        double cornerRBx;
        double cornerRBy;
        double cornerLBx;
        double cornerLBy;
    };

    struct AprilTagCmd {
        using AprilTagListType = std::shared_ptr<std::vector<OwlMailDefine::AprilTagInfo>>;
        using AprilTagCenterType = std::shared_ptr<OwlMailDefine::AprilTagInfo>;

        AprilTagListType aprilTagList;
        AprilTagCenterType aprilTagCenter;
    };

    struct Serial2Cmd;
    struct Cmd2Serial {

        // AdditionCmd
        AdditionCmd additionCmd = AdditionCmd::ignore;
        // +forward,-back
        int16_t x = 0;
        // +right,-left
        int16_t z = 0;
        // +up,-down
        int16_t y = 0;
        // +cw,-ccw
        int16_t cw = 0;

        std::shared_ptr<AprilTagCmd> aprilTagCmdPtr;

        // Serial2Cmd.runner = Cmd2Serial.callbackRunner
        std::function<void(std::shared_ptr<Serial2Cmd>)> callbackRunner;
    };
    struct Serial2Cmd {
        std::function<void(std::shared_ptr<Serial2Cmd>)> runner;
        bool ok = false;
        bool openError = false;
    };
    using CmdSerialMailbox =
            std::shared_ptr<
                    OwlAsyncCallbackMailbox::AsyncCallbackMailbox<
                            Cmd2Serial,
                            Serial2Cmd
                    >
            >;

    using MailCmd2Serial = CmdSerialMailbox::element_type::A2B_t;
    using MailSerial2Cmd = CmdSerialMailbox::element_type::B2A_t;


}

#endif //OWLACCESSTERMINAL_CMDSERIALMAIL_H
