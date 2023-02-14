// jeremie

#ifndef OWLACCESSTERMINAL_AIRPLANESTATE_H
#define OWLACCESSTERMINAL_AIRPLANESTATE_H

#include <memory>

namespace OwlSerialController {

    enum class StateFly {
        Lock = 0,       // locked
        Ready = 1,      // ready to fly
        Open = 2,       // on fly
        Takeoff = 3,    // when takeoff
        Land = 4,       // when land
    };

    struct AirplaneState : std::enable_shared_from_this<AirplaneState> {
        uint8_t stateFly{0};    // StateFly
        int32_t pitch{0};       // raid*100
        int32_t roll{0};
        int32_t yaw{0};
        int32_t vx{0};          // cm/s
        int32_t vy{0};
        int32_t vz{0};
        uint16_t high{0};       // cm
        uint16_t voltage{0};
    };


} // OwlAirplaneState

#endif //OWLACCESSTERMINAL_AIRPLANESTATE_H
