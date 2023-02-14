// jeremie

#ifndef OWLACCESSTERMINAL_STATEREADER_H
#define OWLACCESSTERMINAL_STATEREADER_H

#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/log/trivial.hpp>
#include <boost/core/ignore_unused.hpp>
#include <utility>
#include "CmdSerialMail.h"
#include "../ConfigLoader/ConfigLoader.h"

namespace OwlSerialController {

    struct AirplaneState {
        int32_t pitch{0};   // raid*100
        int32_t roll{0};
        int32_t yaw{0};
        uint16_t high{0};   // cm
        int32_t vx{0};  // cm/s
        int32_t vy{0};
        int32_t vz{0};
    };

    struct PortController;

    class StateReaderImpl;

    class StateReader : std::enable_shared_from_this<StateReader> {
    public:
        StateReader(
                std::weak_ptr<PortController> parentRef,
                std::shared_ptr<boost::asio::serial_port> serialPort
        );

    private:
        std::weak_ptr<PortController> parentRef_;

        std::shared_ptr<boost::asio::serial_port> serialPort_;

        std::shared_ptr<StateReaderImpl> impl;

    public:
        void start();

    private:

    };

} // OwlSerialController

#endif //OWLACCESSTERMINAL_STATEREADER_H
