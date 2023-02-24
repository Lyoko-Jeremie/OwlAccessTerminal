// jeremie

#ifndef OWLACCESSTERMINAL_CMDSERIALMAIL_H
#define OWLACCESSTERMINAL_CMDSERIALMAIL_H

#include <memory>
#include <functional>
#include <vector>
#include <map>
#include "../AsyncCallbackMailbox/AsyncCallbackMailbox.h"
#include "./AirplaneState.h"

namespace OwlMailDefine {

    enum class AdditionCmd {
        ignore = 0,
        takeoff = 1,
        land = 2,
        stop = 3,
        keep = 4,
        move = 10,
        rotate = 11,
        high = 12,
        speed = 13,
        led = 14,
        gotoPosition = 15,
        flyMode = 16,

        calibrate = 90,

        AprilTag = 100,

        JoyCon = 111,
        JoyConSimple = 112,
        JoyConGyro = 116,

        getAirplaneState = 180,

        ping = 251,
        emergencyStop = 252,
        unlock = 253,
    };
    extern const std::map<AdditionCmd, std::string> AdditionCmdNameLookupTable;

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

    struct AprilTagCmd : public std::enable_shared_from_this<AprilTagCmd> {
        using AprilTagListType = std::shared_ptr<std::vector<OwlMailDefine::AprilTagInfo>>;
        using AprilTagCenterType = std::shared_ptr<OwlMailDefine::AprilTagInfo>;

        uint64_t imageX{0};
        uint64_t imageY{0};

        AprilTagListType aprilTagList;
        AprilTagCenterType aprilTagCenter;

        int64_t timestamp{std::chrono::time_point_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now()).time_since_epoch().count()};
    };

    struct JoyConGyro {
        // area direct speed
        int16_t x;
        int16_t y;
        int16_t z;
        // rotate 4-matrix
        int16_t a;
        int16_t b;
        int16_t c;
        int16_t d;
    };
    struct JoyCon {
        // -127~+127
        int16_t leftRockerX{0};
        // -127~+127
        int16_t leftRockerY{0};
        // -127~+127
        int16_t rightRockerX{0};
        // -127~+127
        int16_t rightRockerY{0};
        // 0~+127(+127)
        int8_t leftBackTop{0};
        // 0~+127(+127)
        int8_t leftBackBottom{0};
        // 0~+127(+127)
        int8_t rightBackTop{0};
        // 0~+127(+127)
        int8_t rightBackBottom{0};

        // 0/+127
        int8_t CrossUp{0};
        // 0/+127
        int8_t CrossDown{0};
        // 0/+127
        int8_t CrossLeft{0};
        // 0/+127
        int8_t CrossRight{0};

        // 0/+127
        int8_t A{0};
        // 0/+127
        int8_t B{0};
        // 0/+127
        int8_t X{0};
        // 0/+127
        int8_t Y{0};
        // 0/+127
        int8_t buttonAdd{0};
        // 0/+127
        int8_t buttonReduce{0};

    };

    struct MoveCmd {
        // +forward,-back
        int16_t x{0};
        // +right,-left
        int16_t z{0};
        // +up,-down
        int16_t y{0};
        // +cw,-ccw
        int16_t cw{0};
    };

    struct Serial2Cmd;
    struct Cmd2Serial {

        // AdditionCmd
        AdditionCmd additionCmd = AdditionCmd::ignore;

        std::shared_ptr<MoveCmd> moveCmdPtr;
        std::shared_ptr<AprilTagCmd> aprilTagCmdPtr;
        std::shared_ptr<JoyCon> joyConPtr;
        std::shared_ptr<JoyConGyro> joyConGyroPtr;

        // Serial2Cmd.runner = Cmd2Serial.callbackRunner
        std::function<void(std::shared_ptr<Serial2Cmd>)> callbackRunner;
    };
    struct Serial2Cmd {
        std::function<void(std::shared_ptr<Serial2Cmd>)> runner;
        bool ok = false;
        bool openError = false;

        std::shared_ptr<OwlSerialController::AirplaneState> newestAirplaneState;
        std::shared_ptr<OwlMailDefine::AprilTagCmd> aprilTagCmdData;
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
