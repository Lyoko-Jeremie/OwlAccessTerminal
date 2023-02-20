// jeremie

#include "StateReader.h"
#include <utility>
#include <memory>
#include <boost/assert.hpp>
#include "../SerialController.h"

#include "./StateReaderImplCo.h"
#include "./StateReaderImplNormal.h"

namespace OwlSerialController {

#ifdef DEBUG_IF_CHECK_POINT
    constexpr bool flag_DEBUG_IF_CHECK_POINT = true;
#else
    constexpr bool flag_DEBUG_IF_CHECK_POINT = false;
#endif // DEBUG_IF_CHECK_POINT

    StateReader::StateReader(std::weak_ptr<PortController> parentRef,
                             std::shared_ptr<boost::asio::serial_port> serialPort)
            : parentRef_(std::move(parentRef)),
              serialPort_(std::move(serialPort)) {
    }

    void StateReader::init() {
        if constexpr (flag_DEBUG_IF_CHECK_POINT) {
            BOOST_LOG_TRIVIAL(trace) << "weak_from_this().lock().use_count() : " << weak_from_this().lock().use_count();
            BOOST_ASSERT(!parentRef_.expired());
            BOOST_ASSERT(!parentRef_.lock()->parentRef_.expired());
            BOOST_ASSERT(parentRef_.lock()->parentRef_.lock());
            BOOST_ASSERT(!weak_from_this().expired());
        }
        BOOST_ASSERT(!weak_from_this().expired());
        impl = std::make_shared<StateReaderImpl>(weak_from_this(), serialPort_);
        BOOST_ASSERT(!weak_from_this().expired());
    }

    void StateReader::start() {
        BOOST_LOG_TRIVIAL(trace) << "StateReader::start()";
        if constexpr (flag_DEBUG_IF_CHECK_POINT) {
            BOOST_ASSERT(!weak_from_this().expired());
            BOOST_ASSERT(!parentRef_.expired());
            BOOST_ASSERT(!parentRef_.lock()->parentRef_.expired());
            BOOST_ASSERT(parentRef_.lock()->parentRef_.lock());
        }
        BOOST_ASSERT(impl);
        BOOST_LOG_TRIVIAL(trace) << "StateReader::start() impl.use_count():" << impl.use_count();
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