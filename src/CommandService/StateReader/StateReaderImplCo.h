// jeremie


#ifndef OWLACCESSTERMINAL_STATEREADER_ImplCo_H
#define OWLACCESSTERMINAL_STATEREADER_ImplCo_H


#include "StateReader.h"
#include <utility>
#include <string_view>
#include <memory>
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

    class StateReaderImplCo : std::enable_shared_from_this<StateReaderImplCo> {
    public:
        StateReaderImplCo(
                std::weak_ptr<StateReader> parentRef,
                std::shared_ptr<boost::asio::serial_port> serialPort
        ) : parentRef_(std::move(parentRef)), serialPort_(std::move(serialPort)) {}

    private:
        std::weak_ptr<StateReader> parentRef_;

        boost::asio::streambuf readBuffer_;
        std::shared_ptr<boost::asio::serial_port> serialPort_;

        std::shared_ptr<AirplaneState> airplaneState_;
        uint8_t dataSize_ = 0;

        boost::system::error_code ec_{};
        std::size_t bytes_transferred_ = 0;

        size_t strange = 0;
    public:

        void start() {
            BOOST_LOG_TRIVIAL(warning) << "StateReaderImplCo start()";
            if constexpr (true) {
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

        boost::asio::awaitable<bool> next_read(std::shared_ptr<StateReaderImplCo> _ptr_);


        void loadData(std::shared_ptr<StateReaderImplCo> _ptr_);


    };


}

#endif //OWLACCESSTERMINAL_STATEREADER_ImplCo_H
