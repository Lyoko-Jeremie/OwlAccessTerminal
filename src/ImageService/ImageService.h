// jeremie

#ifndef OWLACCESSTERMINAL_IMAGESERVICE_H
#define OWLACCESSTERMINAL_IMAGESERVICE_H

#include <memory>
#include <string>
#include <iostream>
#include <sstream>
#include <boost/asio/executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/log/trivial.hpp>

namespace OwlImageService {

    class ImageServiceSession : public std::enable_shared_from_this<ImageServiceSession> {
    public:
        explicit ImageServiceSession(
                boost::asio::ip::tcp::socket &&socket
        ) : socket(std::move(socket)) {}

        static void fail(boost::system::error_code ec, const char *what) {
            BOOST_LOG_TRIVIAL(error) << what << ": " << ec.message();
        }

    private:
        boost::asio::ip::tcp::socket socket;

    public:
        void
        start() {
            do_receive();
        }

    private:
        void
        do_receive() {

        }

    };

    class ImageService : public std::enable_shared_from_this<ImageService> {
    public:
        ImageService(
                boost::asio::io_context &ioc,
                const boost::asio::ip::tcp::endpoint& endpoint
        ) : ioc_(ioc),
            acceptor_strand_(boost::asio::make_strand(ioc)),
            acceptor_(acceptor_strand_) {

            boost::system::error_code ec;

            // Open the acceptor
            acceptor_.open(endpoint.protocol(), ec);
            if (ec) {
                fail(ec, "open");
                return;
            }

            // Allow address reuse
            acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
            if (ec) {
                fail(ec, "set_option");
                return;
            }

            // Bind to the server address
            acceptor_.bind(endpoint, ec);
            if (ec) {
                fail(ec, "bind");
                return;
            }

            // Start listening for connections
            acceptor_.listen(
                    boost::asio::socket_base::max_listen_connections, ec);
            if (ec) {
                fail(ec, "listen");
                return;
            }

        }

        static void fail(boost::system::error_code ec, const char *what) {
            BOOST_LOG_TRIVIAL(error) << what << ": " << ec.message();
        }

    private:
        boost::asio::io_context &ioc_;
        boost::asio::strand<boost::asio::io_context::executor_type> acceptor_strand_;
        boost::asio::ip::tcp::acceptor acceptor_;

    public:
        // Start accepting incoming connections
        void
        start() {
            do_accept();
        }

    private:
        void
        do_accept() {
            // The new connection gets its own strand
            acceptor_.async_accept(
                    acceptor_strand_,
                    [this, self = shared_from_this()](
                            boost::system::error_code ec,
                            boost::asio::ip::tcp::socket socket) {
                        if (ec) {
                            ImageService::fail(ec, "accept");
                        } else {
                            on_accept(std::move(socket));
                        }
                    });
        }

        void
        on_accept(boost::asio::ip::tcp::socket &&socket) {
            std::make_shared<ImageServiceSession>(std::move(socket))->start();
        }

    };

} // OwlImageService

#endif //OWLACCESSTERMINAL_IMAGESERVICE_H
