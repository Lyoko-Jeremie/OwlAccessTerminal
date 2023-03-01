// jeremie

#include "./StateReaderImplCo.h"

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/spawn.hpp>

using boost::asio::use_awaitable;
#if defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)
# define use_awaitable \
  boost::asio::use_awaitable_t(__FILE__, __LINE__, __PRETTY_FUNCTION__)
#endif


namespace OwlSerialController {

    void StateReaderImplCo::start_next_read() {
        BOOST_LOG_OWL(trace) << "StateReaderImplCo start_next_read()";
        BOOST_ASSERT(!weak_from_this().expired());
        auto selfPtr = shared_from_this();
        boost::ignore_unused(selfPtr);
        BOOST_LOG_OWL(trace) << "StateReaderImplCo start_next_read() co_spawn";
        boost::asio::co_spawn(
                serialPort_->get_executor(),
                // [this, self = shared_from_this()]() -> boost::asio::awaitable<bool> {
                //     co_return co_await run(self);
                // },
                boost::bind(&StateReaderImplCo::next_read, this, selfPtr),
                [selfPtr](std::exception_ptr e, bool r) {
                    if (r) {
                        BOOST_LOG_OWL(warning) << "StateReaderImplCo run() ok";
                    } else {
                        BOOST_LOG_OWL(error) << "StateReaderImplCo run() error";
                    }
                    // https://stackoverflow.com/questions/14232814/how-do-i-make-a-call-to-what-on-stdexception-ptr
                    try { std::rethrow_exception(std::move(e)); }
                    catch (const std::exception &e) {
                        BOOST_LOG_OWL(error) << "co_spawn catch std::exception " << e.what();
                    }
                    catch (const std::string &e) { BOOST_LOG_OWL(error) << "co_spawn catch std::string " << e; }
                    catch (const char *e) { BOOST_LOG_OWL(error) << "co_spawn catch char *e " << e; }
                    catch (...) {
                        BOOST_LOG_OWL(error) << "StateReaderImplCo co_spawn catch (...)"
                                             << "\n" << boost::current_exception_diagnostic_information();
                    }
                });
    }

    boost::asio::awaitable<bool> StateReaderImplCo::next_read(std::shared_ptr<StateReaderImplCo> _ptr_) {
        BOOST_LOG_OWL(trace) << "StateReaderImplCo next_read()";
        // https://www.boost.org/doc/libs/1_81_0/doc/html/boost_asio/example/cpp20/coroutines/echo_server.cpp

        boost::ignore_unused(_ptr_);
        auto executor = co_await boost::asio::this_coro::executor;

        try {
            // https://github.com/chriskohlhoff/asio/issues/915
            // https://www.boost.org/doc/libs/1_78_0/doc/html/boost_asio/overview/core/cpp20_coroutines.html

            // ======================== find start
            BOOST_LOG_OWL(trace) << "StateReaderImplCo"
                                 << " find start : strange " << strange;
            for (;;) {
                BOOST_LOG_OWL(trace) << "StateReaderImplCo"
                                     << " find when strange :" << strange;
                {
                    std::string s{
                            (std::istreambuf_iterator<char>(&readBuffer_)),
                            std::istreambuf_iterator<char>()
                    };
                    auto p = s.find(delimStart);
                    if (p == std::string::npos) {
                        // ignore
                        // BOOST_LOG_OWL(warning) << "StateReaderImplCo"
                        //                            << " cannot find start delim, next loop";
                    } else {
                        BOOST_LOG_OWL(trace) << "StateReaderImplCo"
                                             << " we find the start delim, next step";
                        // we find the start delim
                        // trim the other data before start delim
                        readBuffer_.consume(p);
                        // goto next step
                        break;
                    }
                }
                ec_.clear();
                bytes_transferred_ = 0;
                bytes_transferred_ = co_await boost::asio::async_read(
                        *serialPort_,
                        readBuffer_,
                        boost::asio::redirect_error(use_awaitable, ec_));
                boost::ignore_unused(_ptr_);
                if (ec_) {
                    // error
                    BOOST_LOG_OWL(error) << "StateReaderImplCo"
                                         << " async_read find start error: "
                                         << ec_.what();
                    co_return false;
                }
                if (bytes_transferred_ == 0) {
                    ++strange;
                    BOOST_LOG_OWL(trace) << "StateReaderImplCo"
                                         << " do strange " << strange;
                    if (strange > 10) {
                        BOOST_LOG_OWL(error) << "StateReaderImplCo"
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
                        BOOST_LOG_OWL(warning) << "StateReaderImplCo"
                                               << " cannot find start delim, next loop";
                        continue;
                    } else {
                        BOOST_LOG_OWL(trace) << "StateReaderImplCo"
                                             << " we find the start delim, next step";
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
                BOOST_LOG_OWL(error) << "StateReaderImplCo"
                                     << " check next start tag error, never go there !!!";
                BOOST_ASSERT_MSG(false, "StateReaderImplCo check next start tag error, never go there !!!");
                co_return false;
            }
            // remove start tag
            BOOST_LOG_OWL(trace) << "StateReaderImplCo"
                                 << " remove start tag";
            readBuffer_.consume(delimStart.size());
            // find data length tag
            if (readBuffer_.size() < sizeof(typeof(dataSize_))) {
                BOOST_LOG_OWL(trace) << "StateReaderImplCo"
                                     << " find data length tag, need read more : "
                                     << (sizeof(typeof(dataSize_)) - readBuffer_.size());
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
                    BOOST_LOG_OWL(error) << "StateReaderImplCo"
                                         << " async_read_until data length tag error: "
                                         << ec_.what();
                    co_return false;
                }
                if (readBuffer_.size() < sizeof(typeof(dataSize_))) {
                    BOOST_LOG_OWL(error) << "StateReaderImplCo"
                                         << " async_read_until data length tag bad";
                    co_return false;
                }
            }


            // now we have the data length tag and more data
            // load size
            dataSize_ = 0;
            {
                BOOST_LOG_OWL(trace) << "StateReaderImplCo"
                                     << " try load dataSize_";
                static_assert(sizeof(typeof(dataSize_)) == 1);
                // dataSize_ = uint8_t
                std::string d{
                        (std::istreambuf_iterator<char>(&readBuffer_)),
                        std::istreambuf_iterator<char>()
                };
                // dataSize <- d
                dataSize_ = static_cast<uint8_t>(d[0]);
                BOOST_LOG_OWL(trace) << "StateReaderImplCo"
                                     << " dataSize_ : " << dataSize_;
            }
            if (dataSize_ != AirplaneStateDataSize) {
                BOOST_LOG_OWL(error) << "StateReaderImplCo"
                                     << " (dataSize_ != AirplaneStateDataSize) , ignore!!!";
                // ignore this package and clean
                dataSize_ = 0;
                boost::ignore_unused(_ptr_);
                start_next_read();
                co_return true;
            }
            // dataSize+len_tag+end_tag
            if (readBuffer_.size() < (dataSize_ + sizeof(uint32_t) + delimEnd.size())) {
                BOOST_LOG_OWL(trace) << "StateReaderImplCo"
                                     << " to read data+endTag, need read more : "
                                     << (dataSize_ + sizeof(uint32_t) + delimEnd.size()) -
                                        readBuffer_.size();
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
                    BOOST_LOG_OWL(error) << "StateReaderImplCo"
                                         << " async_read_until data error: "
                                         << ec_.what();
                    co_return false;
                }
                if (readBuffer_.size() < (dataSize_ + sizeof(uint32_t) + delimEnd.size())) {
                    BOOST_LOG_OWL(error) << "StateReaderImplCo"
                                         << " async_read_until data bad";
                    co_return false;
                }
            }

            if (readBuffer_.size() < (dataSize_ + sizeof(uint32_t) + delimEnd.size())) {
                BOOST_LOG_OWL(error) << "StateReaderImplCo"
                                     << " async_read_until data bad";
                co_return false;
            }
            // ======================================= process data
            BOOST_LOG_OWL(trace) << "StateReaderImplCo"
                                 << " do process data";
            airplaneState_ = std::make_shared<AirplaneState>();
            BOOST_ASSERT(airplaneState_);
            airplaneState_->initTimestamp();
            {
                if (dataSize_ != AirplaneStateDataSize) {
                    BOOST_LOG_OWL(error) << "StateReaderImplCo"
                                         << " (dataSize_ != AirplaneStateDataSize) , ignore!!!";
                    // ignore this package
                } else {
                    BOOST_LOG_OWL(trace) << "StateReaderImplCo"
                                         << " do loadData";
                    loadData(_ptr_);
                    BOOST_LOG_OWL(trace) << "StateReaderImplCo"
                                         << " do loadData ok";
                    // send
                    {
                        auto ptr_sr = parentRef_.lock();
                        if (!ptr_sr) {
                            BOOST_LOG_OWL(error) << "StateReaderImplCo"
                                                 << " parentRef_.lock() ptr_sr failed.";
                            co_return false;
                        }
                        // do a ptr copy to make sure ptr not release by next loop too early
                        BOOST_LOG_OWL(trace) << "StateReaderImplCo"
                                             << " do sendAirplaneState";
                        ptr_sr->sendAirplaneState(airplaneState_->shared_from_this());
                    }
                }

            }


            // ======================================= make clean
            BOOST_LOG_OWL(trace) << "StateReaderImplCo"
                                 << " do make clean";
            // clean all used data
            {
                std::string s{
                        (std::istreambuf_iterator<char>(&readBuffer_)),
                        std::istreambuf_iterator<char>()
                };
                auto p = s.find(delimEnd);
                if (p == std::string::npos) {
                    // error, never go there
                    BOOST_LOG_OWL(error) << "StateReaderImplCo"
                                         << " make clean check error. never gone there!!!";
                    BOOST_ASSERT(p != std::string::npos);
                    co_return false;
                } else {
                    BOOST_LOG_OWL(trace) << "StateReaderImplCo"
                                         << " trim the other data include end delim";
                    // we find the end delim
                    // trim the other data include end delim
                    readBuffer_.consume(p + delimEnd.size());
                    dataSize_ = 0;
                    // goto next loop
                    BOOST_LOG_OWL(trace) << "StateReaderImplCo"
                                         << " goto next loop";
                    boost::ignore_unused(_ptr_);
                    start_next_read();
                    co_return true;
                }
            }

        } catch (const std::exception &e) {
            BOOST_LOG_OWL(error) << "StateReaderImplCo awaitable catch (const std::exception &e) :" << e.what();
        }

        boost::ignore_unused(_ptr_);
        co_return true;
    }

    void StateReaderImplCo::loadData(std::shared_ptr<StateReaderImplCo> _ptr_) {

        // https://stackoverflow.com/questions/41220792/how-copy-or-reuse-boostasiostreambuf
        // std::vector<uint8_t> data(readBuffer_.size());
        // boost::asio::buffer_copy(boost::asio::buffer(data), readBuffer_.data());
        std::string data{
                (std::istreambuf_iterator<char>(&readBuffer_)),
                std::istreambuf_iterator<char>()
        };
        if (data.size() < dataSize_) {
            BOOST_LOG_OWL(error) << "StateReaderImplCo"
                                 << " loadData (data.size() < dataSize_), never go there !!!!!";
            BOOST_ASSERT(!(data.size() < dataSize_));
        }

        // https://www.ruanyifeng.com/blog/2016/11/byte-order.html
        static_assert(sizeof(typeof(AirplaneState::stateFly)) == sizeof(uint8_t));
        BOOST_ASSERT(data.size() >= sizeof(uint8_t));
        airplaneState_->stateFly = loadDataLittleEndian<uint8_t>({data.begin(), data.end()});
        data.erase(data.begin(), data.end() + sizeof(uint8_t) * 1);


        static_assert(sizeof(typeof(AirplaneState::pitch)) == sizeof(int32_t));
        BOOST_ASSERT(data.size() >= sizeof(int32_t));
        airplaneState_->pitch = loadDataLittleEndian<int32_t>({data.begin(), data.end()});
        // airplaneState_->pitch = (data[0] << 0) | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
        static_assert(sizeof(typeof(AirplaneState::roll)) == sizeof(int32_t));
        BOOST_ASSERT(data.size() >= sizeof(int32_t) * 2);
        airplaneState_->roll = loadDataLittleEndian<int32_t>(
                {data.begin() + sizeof(int32_t) * 1, data.end()});
        // airplaneState_->roll = (data[0] << 0) | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
        static_assert(sizeof(typeof(AirplaneState::yaw)) == sizeof(int32_t));
        BOOST_ASSERT(data.size() >= sizeof(int32_t) * 3);
        airplaneState_->yaw = loadDataLittleEndian<int32_t>(
                {data.begin() + sizeof(int32_t) * 2, data.end()});

        BOOST_ASSERT(data.size() >= sizeof(int32_t) * 3);
        data.erase(data.begin(), data.end() + sizeof(int32_t) * 3);


        static_assert(sizeof(typeof(AirplaneState::vx)) == sizeof(int32_t));
        BOOST_ASSERT(data.size() >= sizeof(int32_t) * 1);
        airplaneState_->vx = loadDataLittleEndian<int32_t>(
                {data.begin(), data.end()});
        static_assert(sizeof(typeof(AirplaneState::vy)) == sizeof(int32_t));
        BOOST_ASSERT(data.size() >= sizeof(int32_t) * 2);
        airplaneState_->vy = loadDataLittleEndian<int32_t>(
                {data.begin() + sizeof(int32_t) * 1, data.end()});
        static_assert(sizeof(typeof(AirplaneState::vz)) == sizeof(int32_t));
        BOOST_ASSERT(data.size() >= sizeof(int32_t) * 3);
        airplaneState_->vz = loadDataLittleEndian<int32_t>(
                {data.begin() + sizeof(int32_t) * 2, data.end()});

        BOOST_ASSERT(data.size() > sizeof(int32_t) * 3);
        data.erase(data.begin(), data.end() + sizeof(int32_t) * 3);

        static_assert(sizeof(typeof(AirplaneState::high)) == sizeof(uint16_t));
        BOOST_ASSERT(data.size() >= sizeof(uint16_t));
        airplaneState_->high = loadDataLittleEndian<uint16_t>(
                {data.begin(), data.end()});
        data.erase(data.begin(), data.end() + sizeof(uint16_t) * 1);

        static_assert(sizeof(typeof(AirplaneState::voltage)) == sizeof(uint16_t));
        BOOST_ASSERT(data.size() >= sizeof(uint16_t));
        airplaneState_->voltage = loadDataLittleEndian<uint16_t>(
                {data.begin(), data.end()});
        data.erase(data.begin(), data.end() + sizeof(uint16_t) * 1);

        boost::ignore_unused(_ptr_);
    }
};
