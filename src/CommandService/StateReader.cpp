// jeremie

#include "StateReader.h"
#include "./SerialController.h"
#include <utility>
#include <string_view>
#include <boost/asio/read_until.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/bind/bind.hpp>
#include <boost/array.hpp>

using boost::asio::use_awaitable;
#if defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)
# define use_awaitable \
  boost::asio::use_awaitable_t(__FILE__, __LINE__, __PRETTY_FUNCTION__)
#endif

namespace OwlSerialController {


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

            boost::system::error_code ec;
            std::size_t bytes_transferred = 0;
            try {
                for (;;) {
                    // https://github.com/chriskohlhoff/asio/issues/915
                    // https://www.boost.org/doc/libs/1_78_0/doc/html/boost_asio/overview/core/cpp20_coroutines.html
                    size_t strange = 0;

                    // ======================== find start
                    for (;;) {
                        ec.clear();
                        bytes_transferred = 0;
                        bytes_transferred = co_await boost::asio::async_read(
                                *serialPort_,
                                readBuffer_,
                                boost::asio::redirect_error(use_awaitable, ec));
                        if (ec) {
                            // error
                            BOOST_LOG_TRIVIAL(error) << "StateReaderImpl"
                                                     << " async_read find start error: "
                                                     << ec.what();
                            co_return false;
                        }
                        if (bytes_transferred == 0) {
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
                                                 << " find next start error !!!";
                        co_return false;
                    }
                    // remove start tag
                    readBuffer_.consume(delimStart.size());
                    // find data length tag
                    if (readBuffer_.size() < sizeof(uint32_t)) {
                        ec.clear();
                        bytes_transferred = 0;
                        bytes_transferred = co_await boost::asio::async_read(
                                *serialPort_,
                                readBuffer_,
                                boost::asio::transfer_at_least((sizeof(uint32_t) - readBuffer_.size())),
                                boost::asio::redirect_error(use_awaitable, ec));
                        boost::ignore_unused(bytes_transferred);
                        if (ec) {
                            // error
                            BOOST_LOG_TRIVIAL(error) << "StateReaderImpl"
                                                     << " async_read_until data length tag error: "
                                                     << ec.what();
                            co_return false;
                        }
                        if (readBuffer_.size() < sizeof(uint32_t)) {
                            BOOST_LOG_TRIVIAL(error) << "StateReaderImpl"
                                                     << " async_read_until data length tag bad";
                            co_return false;
                        }
                    }


                    // now we have the data length tag and more data
                    // TODO load size
                    size_t dataSize = 0;
                    {
                        std::string d{
                                (std::istreambuf_iterator<char>(&readBuffer_)),
                                std::istreambuf_iterator<char>()
                        };
                        static_cast<uint16_t>(d[0]);
                        static_cast<uint16_t>(d[1]);
                        // TODO dataSize <- d
                    }
                    // dataSize+len_tag+end_tag
                    if (readBuffer_.size() < (dataSize + sizeof(uint32_t) + delimEnd.size())) {
                        ec.clear();
                        bytes_transferred = 0;
                        bytes_transferred = co_await boost::asio::async_read(
                                *serialPort_,
                                readBuffer_,
                                boost::asio::transfer_exactly(
                                        (dataSize + sizeof(uint32_t) + delimEnd.size()) - readBuffer_.size()),
                                boost::asio::redirect_error(use_awaitable, ec));
                        boost::ignore_unused(bytes_transferred);
                        if (ec) {
                            // error
                            BOOST_LOG_TRIVIAL(error) << "StateReaderImpl"
                                                     << " async_read_until data error: "
                                                     << ec.what();
                            co_return false;
                        }
                        if (readBuffer_.size() < (dataSize + sizeof(uint32_t) + delimEnd.size())) {
                            BOOST_LOG_TRIVIAL(error) << "StateReaderImpl"
                                                     << " async_read_until data bad";
                            co_return false;
                        }
                    }

                    // ======================================= process data
                    // TODO
                    auto data = std::make_shared<AirplaneState>();
                    // TODO data


                    // ======================================= make clean
                    // clean all data
                    readBuffer_.consume(readBuffer_.size());
                    // goto next read loop
                    continue;

                }
            } catch (const std::exception &e) {
                BOOST_LOG_TRIVIAL(error) << e.what();
            }

            boost::ignore_unused(_ptr_);
            co_return true;
        }

        void portDataIn(size_t bytes_transferred) {
            // TODO

            readBuffer_.consume(bytes_transferred);
        }

        void read() {
            boost::asio::async_read(
                    *serialPort_,
                    readBuffer_,
                    [this, self = shared_from_this()](
                            const boost::system::error_code &ec,
                            size_t bytes_transferred
                    ) {
                        if (ec) {
                            // error
                            BOOST_LOG_TRIVIAL(error) << "StateReaderImpl"
                                                     << " async_read error: "
                                                     << ec.what();
                            return;
                        }
                        portDataIn(bytes_transferred);
                    }
            );
        }

        void read_exactly(size_t need_bytes_transferred) {
            boost::asio::async_read(
                    *serialPort_,
                    readBuffer_,
                    boost::asio::transfer_exactly(need_bytes_transferred),
                    [this, self = shared_from_this()](
                            const boost::system::error_code &ec,
                            size_t bytes_transferred
                    ) {
                        if (ec) {
                            // error
                            BOOST_LOG_TRIVIAL(error) << "StateReaderImpl"
                                                     << " read_exactly error: "
                                                     << ec.what();
                            return;
                        }
                        portDataIn(bytes_transferred);
                    }
            );
        }

        void read_until(const std::shared_ptr<std::string> &until_delim_ptr) {
            boost::asio::async_read_until(
                    *serialPort_,
                    readBuffer_,
                    *until_delim_ptr,
                    [this, self = shared_from_this(), until_delim_ptr](
                            const boost::system::error_code &ec,
                            size_t bytes_transferred
                    ) {
                        if (ec) {
                            // error
                            BOOST_LOG_TRIVIAL(error) << "StateReaderImpl"
                                                     << " read_until error: "
                                                     << ec.what();
                            return;
                        }
                        portDataIn(bytes_transferred);
                    }
            );
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

} // OwlSerialController