// jeremie

#ifndef OWLACCESSTERMINAL_ASYNCCALLBACKMAILBOX_H
#define OWLACCESSTERMINAL_ASYNCCALLBACKMAILBOX_H

#include <memory>
#include <functional>
#include <boost/asio.hpp>

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
            boost::asio::post(ioc_b_, [this, self = this->shared_from_this(), data]() {
                // avoid racing
                auto &c = receiveA2B;
                if (c)
                    c(std::move(data));
            });
        }

        // B call this function to send data to A
        void sendB2A(B2A_t &&data) {
            boost::asio::post(ioc_a_, [this, self = this->shared_from_this(), data]() {
                // avoid racing
                auto &c = receiveB2A;
                if (c)
                    c(std::move(data));
            });
        }

    private:

    };

} // OwlAsyncCallbackMailbox

#endif //OWLACCESSTERMINAL_ASYNCCALLBACKMAILBOX_H
