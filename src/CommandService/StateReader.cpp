// jeremie

#include "StateReader.h"
#include "./SerialController.h"
#include <utility>
#include <boost/asio/read_until.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/bind/bind.hpp>

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
                    boost::bind(&StateReaderImpl::run, this),
                    [](std::exception_ptr e, bool r) {
                        if (r) {
                            BOOST_LOG_TRIVIAL(error) << "StateReaderImpl run() ok";
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

        boost::asio::awaitable<bool> run() {
            // https://www.boost.org/doc/libs/1_81_0/doc/html/boost_asio/example/cpp20/coroutines/echo_server.cpp

            auto executor = co_await boost::asio::this_coro::executor;

            try {
                for (;;) {
                    // https://github.com/chriskohlhoff/asio/issues/915
                    // https://www.boost.org/doc/libs/1_78_0/doc/html/boost_asio/overview/core/cpp20_coroutines.html
                    boost::system::error_code ec;
                    std::size_t bytes_transferred = 0;
                    bytes_transferred = co_await boost::asio::async_read(
                            *serialPort_,
                            readBuffer_,
                            boost::asio::redirect_error(boost::asio::use_awaitable, ec));
                    if (!ec) {
                        // error
                        BOOST_LOG_TRIVIAL(error) << "StateReaderImpl"
                                                 << " async_read error: "
                                                 << ec.what();
                        co_return false;
                    }
                    // TODO process bytes_transferred
                    readBuffer_.consume(bytes_transferred);
                }
            } catch (const std::exception &e) {
                BOOST_LOG_TRIVIAL(error) << e.what();
            }

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
                        if (!ec) {
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
                        if (!ec) {
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
                        if (!ec) {
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