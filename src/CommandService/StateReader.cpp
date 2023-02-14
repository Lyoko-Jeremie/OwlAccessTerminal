// jeremie

#include "StateReader.h"
#include <utility>
#include <string_view>
#include <memory>
#include <deque>
#include <boost/asio/read_until.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/bind/bind.hpp>
#include <boost/array.hpp>
#include "./SerialController.h"
#include "./AirplaneState.h"

using boost::asio::use_awaitable;
#if defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)
# define use_awaitable \
  boost::asio::use_awaitable_t(__FILE__, __LINE__, __PRETTY_FUNCTION__)
#endif

namespace OwlSerialController {


    template<typename T>
    T loadDataLittleEndian(const std::string_view &data_) {
        // https://www.ruanyifeng.com/blog/2016/11/byte-order.html
        // https://zh.wikipedia.org/zh-cn/%E5%AD%97%E8%8A%82%E5%BA%8F
        if constexpr (sizeof(T) == sizeof(uint64_t)) {
            const uint64_t *data = reinterpret_cast<const uint64_t *>(data_.data());
            return (data[0] << 0) | (data[1] << 8) | (data[2] << 16) | (data[3] << 24)
                   | (data[4] << 32) | (data[5] << 40) | (data[6] << 49) | (data[7] << 58);
        }
        if constexpr (sizeof(T) == sizeof(uint32_t)) {
            const uint32_t *data = reinterpret_cast<const uint32_t *>(data_.data());
            return (data[0] << 0) | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
        }
        if constexpr (sizeof(T) == sizeof(uint16_t)) {
            const uint16_t *data = reinterpret_cast<const uint16_t *>(data_.data());
            return (data[0] << 0) | (data[1] << 8);
        }
        if constexpr (sizeof(T) == sizeof(uint8_t)) {
            const uint8_t *data = reinterpret_cast<const uint8_t *>(data_.data());
            return (data[0] << 0);
        }
    }

    class StateReaderImpl : std::enable_shared_from_this<StateReaderImpl> {
    public:
        StateReaderImpl(
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

    public:

        void start() {
            boost::asio::co_spawn(
                    serialPort_->get_executor(),
                    // [this, self = shared_from_this()]() -> boost::asio::awaitable<bool> {
                    //     co_return co_await run(self);
                    // },
                    boost::bind(&StateReaderImpl::run, this, shared_from_this()),
                    [](std::exception_ptr e, bool r) {
                        if (r) {
                            BOOST_LOG_TRIVIAL(warning) << "StateReaderImpl run() ok";
                        } else {
                            BOOST_LOG_TRIVIAL(error) << "StateReaderImpl run() error";
                        }
                        // https://stackoverflow.com/questions/14232814/how-do-i-make-a-call-to-what-on-stdexception-ptr
                        try { std::rethrow_exception(std::move(e)); }
                        catch (const std::exception &e) { BOOST_LOG_TRIVIAL(error) << e.what(); }
                        catch (const std::string &e) { BOOST_LOG_TRIVIAL(error) << e; }
                        catch (const char *e) { BOOST_LOG_TRIVIAL(error) << e; }
                        catch (...) { BOOST_LOG_TRIVIAL(error) << "StateReaderImpl co_spawn catch (...)"; }
                    });
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

        boost::asio::awaitable<bool> run(std::shared_ptr<StateReaderImpl> _ptr_) {
            // https://www.boost.org/doc/libs/1_81_0/doc/html/boost_asio/example/cpp20/coroutines/echo_server.cpp

            boost::ignore_unused(_ptr_);
            auto executor = co_await boost::asio::this_coro::executor;

            try {
                for (;;) {
                    // https://github.com/chriskohlhoff/asio/issues/915
                    // https://www.boost.org/doc/libs/1_78_0/doc/html/boost_asio/overview/core/cpp20_coroutines.html
                    size_t strange = 0;

                    // ======================== find start
                    for (;;) {
                        ec_.clear();
                        bytes_transferred_ = 0;
                        bytes_transferred_ = co_await boost::asio::async_read(
                                *serialPort_,
                                readBuffer_,
                                boost::asio::redirect_error(use_awaitable, ec_));
                        boost::ignore_unused(_ptr_);
                        if (ec_) {
                            // error
                            BOOST_LOG_TRIVIAL(error) << "StateReaderImpl"
                                                     << " async_read find start error: "
                                                     << ec_.what();
                            co_return false;
                        }
                        if (bytes_transferred_ == 0) {
                            ++strange;
                            if (strange > 10) {
                                BOOST_LOG_TRIVIAL(error) << "StateReaderImpl"
                                                         << " async_read strange";
                                co_return false;
                            }
                            continue;
                        }
                        strange = 0;
                        {
                            std::string s{
                                    (std::istreambuf_iterator<char>(&readBuffer_)),
                                    std::istreambuf_iterator<char>()
                            };
                            auto p = s.find(delimStart);
                            if (p == std::string::npos) {
                                continue;
                            } else {
                                // we find the start delim
                                // trim the other data before start delim
                                readBuffer_.consume(p);
                                // goto next step
                                break;
                            }
                        }
                    }


                    // ======================== find next
                    if (!std::string{
                            (std::istreambuf_iterator<char>(&readBuffer_)),
                            std::istreambuf_iterator<char>()
                    }.starts_with(delimStart)) {
                        BOOST_LOG_TRIVIAL(error) << "StateReaderImpl"
                                                 << " check next start tag error !!!";
                        co_return false;
                    }
                    // remove start tag
                    readBuffer_.consume(delimStart.size());
                    // find data length tag
                    if (readBuffer_.size() < sizeof(typeof(dataSize_))) {
                        ec_.clear();
                        bytes_transferred_ = 0;
                        bytes_transferred_ = co_await boost::asio::async_read(
                                *serialPort_,
                                readBuffer_,
                                boost::asio::transfer_at_least((sizeof(typeof(dataSize_)) - readBuffer_.size())),
                                boost::asio::redirect_error(use_awaitable, ec_));
                        boost::ignore_unused(bytes_transferred_);
                        boost::ignore_unused(_ptr_);
                        if (ec_) {
                            // error
                            BOOST_LOG_TRIVIAL(error) << "StateReaderImpl"
                                                     << " async_read_until data length tag error: "
                                                     << ec_.what();
                            co_return false;
                        }
                        if (readBuffer_.size() < sizeof(typeof(dataSize_))) {
                            BOOST_LOG_TRIVIAL(error) << "StateReaderImpl"
                                                     << " async_read_until data length tag bad";
                            co_return false;
                        }
                    }


                    // now we have the data length tag and more data
                    // load size
                    dataSize_ = 0;
                    {
                        static_assert(sizeof(typeof(dataSize_)) == 1);
                        // dataSize_ = uint8_t
                        std::string d{
                                (std::istreambuf_iterator<char>(&readBuffer_)),
                                std::istreambuf_iterator<char>()
                        };
                        // dataSize <- d
                        dataSize_ = static_cast<uint8_t>(d[0]);
                    }
                    // dataSize+len_tag+end_tag
                    if (readBuffer_.size() < (dataSize_ + sizeof(uint32_t) + delimEnd.size())) {
                        ec_.clear();
                        bytes_transferred_ = 0;
                        bytes_transferred_ = co_await boost::asio::async_read(
                                *serialPort_,
                                readBuffer_,
                                boost::asio::transfer_exactly(
                                        (dataSize_ + sizeof(uint32_t) + delimEnd.size()) - readBuffer_.size()),
                                boost::asio::redirect_error(use_awaitable, ec_));
                        boost::ignore_unused(bytes_transferred_);
                        boost::ignore_unused(_ptr_);
                        if (ec_) {
                            // error
                            BOOST_LOG_TRIVIAL(error) << "StateReaderImpl"
                                                     << " async_read_until data error: "
                                                     << ec_.what();
                            co_return false;
                        }
                        if (readBuffer_.size() < (dataSize_ + sizeof(uint32_t) + delimEnd.size())) {
                            BOOST_LOG_TRIVIAL(error) << "StateReaderImpl"
                                                     << " async_read_until data bad";
                            co_return false;
                        }
                    }

                    if (readBuffer_.size() < (dataSize_ + sizeof(uint32_t) + delimEnd.size())) {
                        BOOST_LOG_TRIVIAL(error) << "StateReaderImpl"
                                                 << " async_read_until data bad";
                        co_return false;
                    }
                    // ======================================= process data
                    airplaneState_ = std::make_shared<AirplaneState>();
                    {
                        // https://stackoverflow.com/questions/41220792/how-copy-or-reuse-boostasiostreambuf
                        // std::vector<uint8_t> data(readBuffer_.size());
                        // boost::asio::buffer_copy(boost::asio::buffer(data), readBuffer_.data());
                        std::string data{
                                (std::istreambuf_iterator<char>(&readBuffer_)),
                                std::istreambuf_iterator<char>()
                        };

                        // https://www.ruanyifeng.com/blog/2016/11/byte-order.html
                        static_assert(sizeof(typeof(AirplaneState::stateFly)) == sizeof(uint8_t));
                        airplaneState_->stateFly = loadDataLittleEndian<uint8_t>({data.begin(), data.end()});
                        data.erase(data.begin(), data.end() + sizeof(uint8_t) * 1);


                        static_assert(sizeof(typeof(AirplaneState::pitch)) == sizeof(int32_t));
                        airplaneState_->pitch = loadDataLittleEndian<int32_t>({data.begin(), data.end()});
                        // airplaneState_->pitch = (data[0] << 0) | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
                        static_assert(sizeof(typeof(AirplaneState::roll)) == sizeof(int32_t));
                        airplaneState_->roll = loadDataLittleEndian<int32_t>(
                                {data.begin() + sizeof(int32_t) * 1, data.end()});
                        // airplaneState_->roll = (data[0] << 0) | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
                        static_assert(sizeof(typeof(AirplaneState::yaw)) == sizeof(int32_t));
                        airplaneState_->yaw = loadDataLittleEndian<int32_t>(
                                {data.begin() + sizeof(int32_t) * 2, data.end()});

                        data.erase(data.begin(), data.end() + sizeof(int32_t) * 3);


                        static_assert(sizeof(typeof(AirplaneState::vx)) == sizeof(int32_t));
                        airplaneState_->vx = loadDataLittleEndian<int32_t>(
                                {data.begin(), data.end()});
                        static_assert(sizeof(typeof(AirplaneState::vy)) == sizeof(int32_t));
                        airplaneState_->vy = loadDataLittleEndian<int32_t>(
                                {data.begin() + sizeof(int32_t) * 1, data.end()});
                        static_assert(sizeof(typeof(AirplaneState::vz)) == sizeof(int32_t));
                        airplaneState_->vz = loadDataLittleEndian<int32_t>(
                                {data.begin() + sizeof(int32_t) * 2, data.end()});

                        data.erase(data.begin(), data.end() + sizeof(int32_t) * 3);

                        static_assert(sizeof(typeof(AirplaneState::high)) == sizeof(uint16_t));
                        airplaneState_->high = loadDataLittleEndian<uint16_t>(
                                {data.begin(), data.end()});
                        data.erase(data.begin(), data.end() + sizeof(uint16_t) * 1);

                        static_assert(sizeof(typeof(AirplaneState::voltage)) == sizeof(uint16_t));
                        airplaneState_->voltage = loadDataLittleEndian<uint16_t>(
                                {data.begin(), data.end()});
                        data.erase(data.begin(), data.end() + sizeof(uint16_t) * 1);

                    }

                    // send
                    {
                        auto ptr_sr = parentRef_.lock();
                        if (!ptr_sr) {
                            BOOST_LOG_TRIVIAL(error) << "StateReaderImpl"
                                                     << " parentRef_.lock() ptr_sr failed.";
                            co_return false;
                        }
                        // do a ptr copy to make sure ptr not release by next loop too early
                        ptr_sr->sendAirplaneState(airplaneState_->shared_from_this());
                    }


                    // ======================================= make clean
                    // clean all used data
                    {
                        std::string s{
                                (std::istreambuf_iterator<char>(&readBuffer_)),
                                std::istreambuf_iterator<char>()
                        };
                        auto p = s.find(delimEnd);
                        if (p == std::string::npos) {
                            // error, never go there
                            BOOST_LOG_TRIVIAL(error) << "StateReaderImpl"
                                                     << " make clean check error. never gone.";
                            co_return false;
                        } else {
                            // we find the end delim
                            // trim the other data include end delim
                            readBuffer_.consume(p + delimEnd.size());
                            // goto next loop
                            continue;
                        }
                    }

                }
            } catch (const std::exception &e) {
                BOOST_LOG_TRIVIAL(error) << e.what();
            }

            boost::ignore_unused(_ptr_);
            co_return true;
        }


    };


    StateReader::StateReader(std::weak_ptr<PortController> parentRef,
                             std::shared_ptr<boost::asio::serial_port> serialPort)
            : parentRef_(std::move(parentRef)),
              serialPort_(std::move(serialPort)),
              impl(std::make_shared<StateReaderImpl>(weak_from_this(), serialPort_)) {}

    void StateReader::start() {
        impl->start();
    }

    void StateReader::sendAirplaneState(const std::shared_ptr<AirplaneState> &airplaneState) {
        boost::asio::dispatch(serialPort_->get_executor(), [
                this, self = shared_from_this(), airplaneState]() {
            auto ptr = parentRef_.lock();
            if (!ptr) {
                BOOST_LOG_TRIVIAL(error) << "PortController"
                                         << " parentRef_.lock() failed.";
                return;
            }
            ptr->sendAirplaneState(airplaneState);
        });
    }

} // OwlSerialController