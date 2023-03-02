// jeremie

#ifndef OWLACCESSTERMINAL_STATEREADER_H
#define OWLACCESSTERMINAL_STATEREADER_H

#include "../../MemoryBoost.h"
#include <boost/asio.hpp>
#include <boost/asio/read_until.hpp>
#include "../../OwlLog/OwlLog.h"
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

    class StateReader : public boost::enable_shared_from_this<StateReader> {
    public:
        StateReader(
                boost::weak_ptr<PortController> parentRef,
                boost::shared_ptr<boost::asio::serial_port> serialPort
        );

        void init();

        ~StateReader() {
            BOOST_LOG_OWL(trace_dtor) << "~StateReader()";
        }

    private:
        friend class StateReaderImplCo;

        friend class StateReaderImplNormal;

        boost::weak_ptr<PortController> parentRef_;

        boost::shared_ptr<boost::asio::serial_port> serialPort_;

        boost::shared_ptr<StateReaderImpl> impl;

        void sendAirplaneState(const boost::shared_ptr<AirplaneState> &airplaneState);

    public:
        void start();

    private:

    };

} // OwlSerialController

#endif //OWLACCESSTERMINAL_STATEREADER_H
