// jeremie


#ifndef OWLACCESSTERMINAL_STATEREADER_ImplCo_H
#define OWLACCESSTERMINAL_STATEREADER_ImplCo_H


#include "StateReader.h"
#include <utility>
#include <string_view>
#include "../../MemoryBoost.h"
#include <deque>
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/bind/bind.hpp>
#include <boost/array.hpp>
#include <boost/assert.hpp>
#include "../SerialController.h"
#include "../AirplaneState.h"
#include "./LoadDataLittleEndian.h"

namespace OwlSerialController {

    class StateReaderImplCo : public boost::enable_shared_from_this<StateReaderImplCo> {
    public:
        StateReaderImplCo(
                boost::weak_ptr<StateReader> parentRef,
                boost::shared_ptr<boost::asio::serial_port> serialPort
        ) : parentRef_(std::move(parentRef)), serialPort_(std::move(serialPort)) {}

        ~StateReaderImplCo() {
            BOOST_LOG_OWL(trace_dtor) << "~StateReaderImplCo()";
        }

    private:
        boost::weak_ptr<StateReader> parentRef_;

        boost::asio::streambuf readBuffer_;
        boost::shared_ptr<boost::asio::serial_port> serialPort_;

        boost::shared_ptr<AirplaneState> airplaneState_;
        uint8_t dataSize_ = 0;

        boost::system::error_code ec_{};
        std::size_t bytes_transferred_ = 0;

        size_t strange = 0;
    public:

        void start() {
#ifdef DEBUG_IF_CHECK_POINT
            constexpr bool flag_DEBUG_IF_CHECK_POINT = true;
#else
            constexpr bool flag_DEBUG_IF_CHECK_POINT = false;
#endif // DEBUG_IF_CHECK_POINT

            BOOST_LOG_OWL(trace_cmd_sp_r) << "StateReaderImplCo start()";
            if constexpr (flag_DEBUG_IF_CHECK_POINT) {
                BOOST_ASSERT(!weak_from_this().expired());
                BOOST_ASSERT(!parentRef_.expired());
                BOOST_ASSERT(!parentRef_.lock()->parentRef_.expired());
                BOOST_ASSERT(!parentRef_.lock()->parentRef_.lock()->parentRef_.expired());
                BOOST_ASSERT(parentRef_.lock()->parentRef_.lock()->parentRef_.lock());
            }
            start_next_read();
        }

    private:

        const std::string delimStart{
                static_cast<char>(0xAA),
                static_cast<char>(0xAA),
                static_cast<char>(0xAA),
                static_cast<char>(0xAA),
        };
        const std::string delimEnd{
                static_cast<char>(0xBB),
                static_cast<char>(0xBB),
                static_cast<char>(0xBB),
                static_cast<char>(0xBB),
        };
    private:

        void start_next_read();

        boost::asio::awaitable<bool> next_read(boost::shared_ptr<StateReaderImplCo> _ptr_);


        void loadData(boost::shared_ptr<StateReaderImplCo> _ptr_);


    };


}

#endif //OWLACCESSTERMINAL_STATEREADER_ImplCo_H
