// jeremie

#ifndef OWLACCESSTERMINAL_AIRPLANESTATE_H
#define OWLACCESSTERMINAL_AIRPLANESTATE_H

#include <memory>

namespace OwlSerialController {

    struct AirplaneState : std::enable_shared_from_this<AirplaneState> {
        int32_t pitch{0};   // raid*100
        int32_t roll{0};
        int32_t yaw{0};
        uint16_t high{0};   // cm
        int32_t vx{0};  // cm/s
        int32_t vy{0};
        int32_t vz{0};
    };


} // OwlAirplaneState

#endif //OWLACCESSTERMINAL_AIRPLANESTATE_H
