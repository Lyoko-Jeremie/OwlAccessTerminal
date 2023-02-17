// jeremie

#ifndef OWLACCESSTERMINAL_STATEREADER_H
#define OWLACCESSTERMINAL_STATEREADER_H

#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/log/trivial.hpp>
#include <boost/core/ignore_unused.hpp>
#include <utility>
#include "../CmdSerialMail.h"
#include "../../ConfigLoader/ConfigLoader.h"

namespace OwlSerialController {

    struct AirplaneState;

    struct PortController;

    class StateReaderImplCo;

    class StateReaderImplNormal;

#ifdef UseStateReaderImplNormal
    using StateReaderImpl = StateReaderImplNormal;
#else // UseStateReaderImplCo
    using StateReaderImpl = StateReaderImplCo;
#endif // UseStateReaderImplNormal

    class StateReader : std::enable_shared_from_this<StateReader> {
    public:
        StateReader(
                std::weak_ptr<PortController> parentRef,
                std::shared_ptr<boost::asio::serial_port> serialPort
        );

    private:
        friend class StateReaderImplCo;

        friend class StateReaderImplNormal;

        std::weak_ptr<PortController> parentRef_;

        std::shared_ptr<boost::asio::serial_port> serialPort_;

        std::shared_ptr<StateReaderImpl> impl;

        void sendAirplaneState(const std::shared_ptr<AirplaneState> &airplaneState);

    public:
        void start();

    private:

    };

} // OwlSerialController

#endif //OWLACCESSTERMINAL_STATEREADER_H
