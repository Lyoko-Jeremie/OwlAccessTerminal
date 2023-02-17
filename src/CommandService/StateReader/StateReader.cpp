// jeremie

#include "StateReader.h"
#include <utility>
#include <memory>
#include <boost/assert.hpp>
#include "../SerialController.h"

#include "./StateReaderImplCo.h"
#include "./StateReaderImplNormal.h"

namespace OwlSerialController {


    StateReader::StateReader(std::weak_ptr<PortController> parentRef,
                             std::shared_ptr<boost::asio::serial_port> serialPort)
            : parentRef_(std::move(parentRef)),
              serialPort_(std::move(serialPort)) {
        impl = std::make_shared<StateReaderImpl>(weak_from_this(), serialPort_);
    }

    void StateReader::start() {
        BOOST_ASSERT(impl);
        impl->start();
    }

    void StateReader::sendAirplaneState(const std::shared_ptr<AirplaneState> &airplaneState) {
        BOOST_ASSERT(serialPort_);
        boost::asio::dispatch(serialPort_->get_executor(), [
                this, self = shared_from_this(), airplaneState]() {
            auto ptr = parentRef_.lock();
            if (!ptr) {
                BOOST_LOG_TRIVIAL(error) << "PortController"
                                         << " parentRef_.lock() failed.";
                return;
            }
            BOOST_ASSERT(ptr);
            ptr->sendAirplaneState(airplaneState);
        });
    }

} // OwlSerialController