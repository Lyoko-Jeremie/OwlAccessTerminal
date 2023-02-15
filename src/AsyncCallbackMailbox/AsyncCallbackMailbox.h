// jeremie

#ifndef OWLACCESSTERMINAL_ASYNCCALLBACKMAILBOX_H
#define OWLACCESSTERMINAL_ASYNCCALLBACKMAILBOX_H

#include <memory>
#include <functional>
#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <boost/core/ignore_unused.hpp>
#include <utility>

namespace OwlAsyncCallbackMailbox {

    template<typename A2B, typename B2A>
    class AsyncCallbackMailbox :
            public std::enable_shared_from_this<AsyncCallbackMailbox<A2B, B2A>> {
    public:
        AsyncCallbackMailbox() = delete;

        AsyncCallbackMailbox(
                boost::asio::io_context &ioc_a,
                boost::asio::io_context &ioc_b
        ) : ioc_a_(ioc_a), ioc_b_(ioc_b) {}

        using A2B_t = std::shared_ptr<A2B>;
        using B2A_t = std::shared_ptr<B2A>;

#ifdef DEBUG_AsyncCallbackMailbox
        std::string debugTag_;

        AsyncCallbackMailbox(
                boost::asio::io_context &ioc_a,
                boost::asio::io_context &ioc_b,
                const std::string &debugTag
        ) : ioc_a_(ioc_a), ioc_b_(ioc_b), debugTag_(std::move(debugTag)) {}

#else // DEBUG_AsyncCallbackMailbox

        AsyncCallbackMailbox(
                boost::asio::io_context &ioc_a,
                boost::asio::io_context &ioc_b,
                const std::string &debugTag
        ) : ioc_a_(ioc_a), ioc_b_(ioc_b) {
            boost::ignore_unused(debugTag);
        }

#endif // DEBUG_AsyncCallbackMailbox

    private:
        boost::asio::io_context &ioc_a_;
        boost::asio::io_context &ioc_b_;

    public:
        // B register the callback to receive mail from A
        std::function<void(A2B_t)> receiveA2B;
        // A register the callback to receive mail from B
        std::function<void(B2A_t)> receiveB2A;

        // A call this function to send data to B
        void sendA2B(A2B_t &&data) {
#ifdef DEBUG_AsyncCallbackMailbox
            BOOST_LOG_TRIVIAL(trace) << "AsyncCallbackMailbox sendA2B" << " [" << debugTag_ << "]";
#endif // DEBUG_AsyncCallbackMailbox
            boost::asio::post(ioc_b_, [this, self = this->shared_from_this(), data]() {
#ifdef DEBUG_AsyncCallbackMailbox
                BOOST_LOG_TRIVIAL(trace) << "AsyncCallbackMailbox sendA2B post" << " [" << debugTag_ << "]";
#endif // DEBUG_AsyncCallbackMailbox
                // avoid racing
                auto &c = receiveA2B;
                if (c) {
#ifdef DEBUG_AsyncCallbackMailbox
                    BOOST_LOG_TRIVIAL(trace) << "AsyncCallbackMailbox sendA2B before call callback" << " [" << debugTag_ << "]";
#endif // DEBUG_AsyncCallbackMailbox
                    c(std::move(data));
#ifdef DEBUG_AsyncCallbackMailbox
                    BOOST_LOG_TRIVIAL(trace) << "AsyncCallbackMailbox sendA2B after call callback" << " [" << debugTag_ << "]";
#endif // DEBUG_AsyncCallbackMailbox
                } else {
#ifdef DEBUG_AsyncCallbackMailbox
                    BOOST_LOG_TRIVIAL(error) << "AsyncCallbackMailbox sendA2B receiveA2B empty" << " [" << debugTag_ << "]";
#else // DEBUG_AsyncCallbackMailbox
                    BOOST_LOG_TRIVIAL(error) << "AsyncCallbackMailbox sendA2B receiveA2B empty";
#endif // DEBUG_AsyncCallbackMailbox
                }
            });
        }

        // B call this function to send data to A
        void sendB2A(B2A_t &&data) {
#ifdef DEBUG_AsyncCallbackMailbox
            BOOST_LOG_TRIVIAL(trace) << "AsyncCallbackMailbox sendB2A" << " [" << debugTag_ << "]";
#endif // DEBUG_AsyncCallbackMailbox
            boost::asio::post(ioc_a_, [this, self = this->shared_from_this(), data]() {
#ifdef DEBUG_AsyncCallbackMailbox
                BOOST_LOG_TRIVIAL(trace) << "AsyncCallbackMailbox sendB2A post" << " [" << debugTag_ << "]";
#endif // DEBUG_AsyncCallbackMailbox
                // avoid racing
                auto &c = receiveB2A;
                if (c) {
#ifdef DEBUG_AsyncCallbackMailbox
                    BOOST_LOG_TRIVIAL(trace) << "AsyncCallbackMailbox sendB2A before call callback" << " [" << debugTag_ << "]";
#endif // DEBUG_AsyncCallbackMailbox
                    c(std::move(data));
#ifdef DEBUG_AsyncCallbackMailbox
                    BOOST_LOG_TRIVIAL(trace) << "AsyncCallbackMailbox sendB2A after call callback" << " [" << debugTag_ << "]";
#endif // DEBUG_AsyncCallbackMailbox
                } else {
#ifdef DEBUG_AsyncCallbackMailbox
                    BOOST_LOG_TRIVIAL(error) << "AsyncCallbackMailbox sendB2A receiveB2A empty" << " [" << debugTag_ << "]";
#else // DEBUG_AsyncCallbackMailbox
                    BOOST_LOG_TRIVIAL(error) << "AsyncCallbackMailbox sendB2A receiveB2A empty";
#endif // DEBUG_AsyncCallbackMailbox
                }
            });
        }

    private:

    };

} // OwlAsyncCallbackMailbox

#endif //OWLACCESSTERMINAL_ASYNCCALLBACKMAILBOX_H
