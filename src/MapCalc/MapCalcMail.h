// jeremie

#ifndef OWLACCESSTERMINAL_MAPCALCMAIL_H
#define OWLACCESSTERMINAL_MAPCALCMAIL_H

#include <memory>
#include <functional>
#include <chrono>
#include "MapCalcPlaneInfoType.h"
#include "../AsyncCallbackMailbox/AsyncCallbackMailbox.h"
#include "../CommandService/CmdSerialMail.h"
#include "../CommandService/AirplaneState.h"

namespace OwlMailDefine {

    struct MapCalc2Service;
    struct Service2MapCalc {

        std::shared_ptr<OwlMailDefine::AprilTagCmd> tagInfo;
        std::shared_ptr<OwlSerialController::AirplaneState> airplaneState;

        // MapCalc2Service.runner = Service2MapCalc.callbackRunner
        std::function<void(std::shared_ptr<MapCalc2Service>)> callbackRunner;
    };
    struct MapCalc2Service {

        std::shared_ptr<OwlMapCalc::MapCalcPlaneInfoType> info;

        bool ok{false};

        std::function<void(std::shared_ptr<MapCalc2Service>)> runner;
    };
    using ServiceMapCalcMailbox =
            std::shared_ptr<
                    OwlAsyncCallbackMailbox::AsyncCallbackMailbox<
                            Service2MapCalc,
                            MapCalc2Service
                    >
            >;

    using MailService2MapCalc = ServiceMapCalcMailbox::element_type::A2B_t;
    using MailMapCalc2Service = ServiceMapCalcMailbox::element_type::B2A_t;

}

#endif //OWLACCESSTERMINAL_MAPCALCMAIL_H
