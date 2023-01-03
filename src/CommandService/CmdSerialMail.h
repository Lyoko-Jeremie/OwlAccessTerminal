// jeremie

#ifndef OWLACCESSTERMINAL_CMDSERIALMAIL_H
#define OWLACCESSTERMINAL_CMDSERIALMAIL_H

#include <memory>
#include <functional>
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

        // Serial2Cmd.runner = Cmd2Serial.callbackRunner
        std::function<void(std::shared_ptr<Serial2Cmd>)> callbackRunner;
    };
    struct Serial2Cmd {
        std::function<void(std::shared_ptr<Serial2Cmd>)> runner;
        bool ok = false;
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
