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
#include <boost/asio/write.hpp>
#include <boost/array.hpp>
#include <boost/log/trivial.hpp>
#include "ImageProtobufDefine/ImageProtocol/ImageProtocol.pb.h"

namespace OwlImageService {

    enum {
        TCP_Receive_Package_Max_Size = (1024 * 1024 * 6)
    }; // 6M

    struct CommonTcpPackage {
        uint32_t size_ = 0;
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
                boost::asio::io_context &ioc,
                boost::asio::ip::tcp::socket &&socket
        ) : ioc_(ioc), socket_(std::move(socket)) {}

//        static void fail(boost::system::error_code ec, const char *what) {
//            BOOST_LOG_TRIVIAL(error) << what << ": " << ec.message();
//        }

    private:
        boost::asio::io_context &ioc_;
        boost::asio::ip::tcp::socket socket_;

    public:
        void
        start() {
            do_receive_size();
        }

    private:
        void
        close_connect() {

            if (socket_.is_open()) {
                boost::system::error_code ec;
                socket_.shutdown(socket_.shutdown_both, ec);
                ec.clear();
                socket_.cancel(ec);
                ec.clear();
                socket_.close(ec);
            }

        }

    public:
        void
        force_close() {
            boost::asio::post(ioc_, [this, self = shared_from_this()]() {
                close_connect();
            });
        }

    private:
        void
        do_receive_size() {

            if (!socket_.is_open()) {
                // is closed by other function, stop!
                BOOST_LOG_TRIVIAL(info) << "Connection closed by other function, stop!";
                return;
            }

            auto package_r_ = std::make_shared<CommonTcpPackage>();
            boost::asio::async_read(
                    socket_,
                    boost::asio::buffer(
                            reinterpret_cast<char *>(&package_r_->size_), 4
                    ),
                    [this, self = shared_from_this(), package_r_](
                            const boost::system::error_code &ec,
                            std::size_t bytes_transferred) {

                        if (ec == boost::asio::error::eof) {
                            // Connection closed cleanly by peer.
                            BOOST_LOG_TRIVIAL(info) << "Connection closed cleanly by peer.";
                            return;
                        }
                        if (ec) {
                            // Connection error
                            BOOST_LOG_TRIVIAL(error) << "Connection error " << ec.what();
                            return;
                        }
                        if (bytes_transferred != 4) {
                            // bytes_transferred
                            BOOST_LOG_TRIVIAL(error) << "do_receive_size bytes_transferred : " << bytes_transferred;
                            return;
                        }

                        package_r_->size_ = ntohl(package_r_->size_);

                        if (package_r_->size_ > TCP_Receive_Package_Max_Size) {
                            // size_ too big, maybe a wrong size_ byte
                            BOOST_LOG_TRIVIAL(warning)
                                << "(package_r_.size_ > TCP_Receive_Package_Max_Size), size_: " << package_r_->size_;
                            // TODO close the connect
                            return;
                        }

                        do_receive_data(package_r_);


                        return;
                    }
            );
        }

        void
        do_receive_data(std::shared_ptr<CommonTcpPackage> package_r_) {

            if (!socket_.is_open()) {
                // is closed by other function, stop!
                BOOST_LOG_TRIVIAL(info) << "Connection closed by other function, stop!";
                return;
            }

            boost::asio::async_read(
                    socket_,
                    package_r_->data_,
                    boost::asio::transfer_exactly(package_r_->size_),
                    [this, self = shared_from_this(), package_r_](
                            const boost::system::error_code &ec,
                            std::size_t bytes_transferred) {

                        if (ec == boost::asio::error::eof) {
                            // Connection closed cleanly by peer.
                            BOOST_LOG_TRIVIAL(info) << "Connection closed cleanly by peer.";
                            return;
                        }
                        if (ec) {
                            // Connection error
                            BOOST_LOG_TRIVIAL(error) << "Connection error " << ec.what();
                            return;
                        }
                        if (bytes_transferred != package_r_->size_) {
                            // bytes_transferred
                            BOOST_LOG_TRIVIAL(error) << "do_receive_data bytes_transferred : " << bytes_transferred;
                            return;
                        }

                        // https://stackoverflow.com/questions/44904295/convert-stdstring-to-boostasiostreambuf
                        std::string s((std::istreambuf_iterator<char>(&package_r_->data_)),
                                      std::istreambuf_iterator<char>());

                        do_process_request(std::move(s));

                        // next read

                        do_receive_size();

                        return;
                    }
            );

        }

    private:
        void
        do_send_size(std::shared_ptr<CommonTcpPackage> package_s_) {

            if (!socket_.is_open()) {
                // is closed by other function, stop!
                BOOST_LOG_TRIVIAL(info) << "Connection closed by other function, stop!";
                return;
            }

            boost::asio::async_write(
                    socket_,
                    boost::asio::buffer(
                            (reinterpret_cast<uint32_t *>( &package_s_->size_ )), 4
                    ),
                    [this, self = shared_from_this(), package_s_](
                            const boost::system::error_code &ec,
                            std::size_t bytes_transferred) {

                        if (ec == boost::asio::error::eof) {
                            // Connection closed cleanly by peer.
                            BOOST_LOG_TRIVIAL(info) << "Connection closed cleanly by peer.";
                            return;
                        }
                        if (ec) {
                            // Connection error
                            BOOST_LOG_TRIVIAL(error) << "Connection error " << ec.what();
                            return;
                        }
                        if (bytes_transferred != 4) {
                            // bytes_transferred
                            BOOST_LOG_TRIVIAL(error) << "do_send_size bytes_transferred : " << bytes_transferred;
                            return;
                        }

                        do_send_data(package_s_);

                    }
            );
        }

        void
        do_send_data(std::shared_ptr<CommonTcpPackage> package_s_) {

            if (!socket_.is_open()) {
                // is closed by other function, stop!
                BOOST_LOG_TRIVIAL(info) << "Connection closed by other function, stop!";
                return;
            }

            boost::asio::async_write(
                    socket_,
                    package_s_->data_,
                    boost::asio::transfer_exactly(package_s_->size_),
                    [this, self = shared_from_this(), package_s_](
                            const boost::system::error_code &ec,
                            std::size_t bytes_transferred) {

                        if (ec == boost::asio::error::eof) {
                            // Connection closed cleanly by peer.
                            BOOST_LOG_TRIVIAL(info) << "Connection closed cleanly by peer.";
                            return;
                        }
                        if (ec) {
                            // Connection error
                            BOOST_LOG_TRIVIAL(error) << "Connection error " << ec.what();
                            return;
                        }
                        if (bytes_transferred != package_s_->size_) {
                            // bytes_transferred
                            BOOST_LOG_TRIVIAL(error) << "do_send_size bytes_transferred : " << bytes_transferred;
                            return;
                        }

                        // all ok
                        BOOST_LOG_TRIVIAL(info) << "do_send_data complete." << ec.what();

                        return;
                    }
            );
        }

    private:
        void
        do_process_request(std::string &&s) {

            ImageRequest ir;
            ir.ParseFromString(s);

            BOOST_LOG_TRIVIAL(info) << "do_process_request ImageRequest: " << ir.DebugString();

            try {
                switch (ir.cmd_id()) {
                    case 1: {
                        // this is a read camera request
                        if (ir.has_package_id() && ir.has_camera_id()) {
                            // TODO get camera image data on here

                            // TODO now create ImageResponse package
                            ImageResponse is;

                            // now create send package and send it
                            auto package_s_ = std::make_shared<CommonTcpPackage>();
                            // https://stackoverflow.com/questions/44904295/convert-stdstring-to-boostasiostreambuf
                            std::iostream{&package_s_->data_} << is.SerializeAsString();
                            package_s_->size_ = is.GetCachedSize();
                            do_send_size(package_s_);
                        }
                    }
                        return;
                    default:
                        BOOST_LOG_TRIVIAL(warning) << "do_process_request switch default: " << ir.DebugString();
                        return;
                }
            } catch (const std::exception &e) {
                BOOST_LOG_TRIVIAL(error) << "do_process_request catch exception on " << ir.DebugString()
                                         << " e: " << e.what();
                return;
            } catch (...) {
                BOOST_LOG_TRIVIAL(error) << "do_process_request catch unknown exception on " << ir.DebugString();
                return;
            }
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
