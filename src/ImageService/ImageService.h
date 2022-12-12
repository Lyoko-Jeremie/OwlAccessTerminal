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
#include <boost/asio/streambuf.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/array.hpp>
#include <boost/log/trivial.hpp>

namespace OwlImageService {

    struct CommonTcpPackage {
        uint32_t size = 0;
        boost::asio::streambuf data_{};

// https://stackoverflow.com/questions/7556327/int-char-conversion-in-network-frames-with-c-boostasio
//        // sender
//        // char 4 => uint
//        char char_len[4];
//        char_len[0] = (len >> 0);
//        char_len[1] = (len >> 8);
//        char_len[2] = (len >> 16);
//        char_len[3] = (len >> 24);
//        std::cout << "char[4] len=["
//        << char_len[0] << ',' << char_len[1] << ','
//        << char_len[2] << ',' << char_len[3] << ']'
//        << std::endl;
//        uint32_t uint_len = *(reinterpret_cast<uint32_t *>( char_len ));
//        std::cout << "uint len=" << uint_len << std::endl;
//        // network bytes order
//        uint32_t net_len = htonl( len );
//        std::cout << "net_len=" << net_len << std::endl;

//        // receiver
//        uint32_t net_len;
//        size_t len_len = sock->read_some( boost::asio::buffer( reinterpret_cast<char*>(&net_len), 4), error );
//        uint32_t len = ntohl( net_len );
//        std::cout << "uint len=" << len << std::endl;

        CommonTcpPackage() = default;
    };

    class ImageServiceSession : public std::enable_shared_from_this<ImageServiceSession> {
    public:
        explicit ImageServiceSession(
                boost::asio::ip::tcp::socket &&socket
        ) : socket_(std::move(socket)) {}

        static void fail(boost::system::error_code ec, const char *what) {
            BOOST_LOG_TRIVIAL(error) << what << ": " << ec.message();
        }

    private:
        boost::asio::ip::tcp::socket socket_;
        CommonTcpPackage package_r_{};

    public:
        void
        start() {
            do_receive();
        }

    private:
        void
        do_receive() {
            boost::asio::async_read(
                    socket_,
                    boost::asio::buffer(
                            reinterpret_cast<char *>(&package_r_.size), 4
                    ),
                    [this, self = shared_from_this()](
                            const boost::system::error_code &ec,
                            std::size_t bytes_transferred) {

                        if (ec == boost::asio::error::eof) {
                            // Connection closed cleanly by peer.
                            return;
                        }
                        if (ec) {
                            // Connection error
                            return;
                        }

                        package_r_.size = ntohl(package_r_.size);



                        return;
                    }
            );
        }

    };

    class ImageService : public std::enable_shared_from_this<ImageService> {
    public:
        ImageService(
                boost::asio::io_context &ioc,
                const boost::asio::ip::tcp::endpoint &endpoint
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
