// jeremie

#ifndef OWLACCESSTERMINAL_IMAGESERVICEHTTP_H
#define OWLACCESSTERMINAL_IMAGESERVICEHTTP_H

#include <memory>
#include "ImageServiceMail.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>
#include <utility>


namespace OwlImageServiceHttp {

    class ImageServiceHttp;

    class ImageServiceHttpConnect : public std::enable_shared_from_this<ImageServiceHttpConnect> {
    public:
        ImageServiceHttpConnect(
                boost::asio::io_context &ioc,
                boost::asio::ip::tcp::socket &&socket,
                std::weak_ptr<ImageServiceHttp> &&parents
        ) : ioc_(ioc),
            socket_(std::move(socket)),
            parents_(std::move(parents)) {
        }

        // Initiate the asynchronous operations associated with the connection.
        void
        start() {
            read_request();
            check_deadline();
        }

    private:
        boost::asio::io_context &ioc_;

        // The socket for the currently connected client.
        boost::asio::ip::tcp::socket socket_;

        std::weak_ptr<ImageServiceHttp> parents_;

        // The buffer for performing reads.
        boost::beast::flat_buffer buffer_{8192};

        // The request message.
        boost::beast::http::request<boost::beast::http::dynamic_body> request_;

        // The timer for putting a deadline on connection processing.
        boost::asio::steady_timer deadline_{
                socket_.get_executor(), std::chrono::seconds(60)};

        // Asynchronously receive a complete request message.
        void
        read_request() {
            auto self = shared_from_this();

            boost::beast::http::async_read(
                    socket_,
                    buffer_,
                    request_,
                    [self](boost::beast::error_code ec,
                           std::size_t bytes_transferred) {
                        boost::ignore_unused(bytes_transferred);
                        if (!ec)
                            self->process_request();
                    });
        }

        // Determine what needs to be done with the request message.
        void
        process_request() {

            switch (request_.method()) {
                case boost::beast::http::verb::get:
                    create_get_response();
                    break;

                default: {
                    auto response = std::make_shared<boost::beast::http::response<boost::beast::http::dynamic_body>>();
                    response->version(request_.version());
                    response->keep_alive(false);
                    // We return responses indicating an error if
                    // we do not recognize the request method.
                    response->result(boost::beast::http::status::bad_request);
                    response->set(boost::beast::http::field::content_type, "text/plain");
                    boost::beast::ostream(response->body())
                            << "Invalid request-method '"
                            << std::string(request_.method_string())
                            << "'";
                    response->content_length(response->body().size());
                    write_response(response);
                }
                    break;
            }

        }

        void create_get_response_image(int camera_id);

        // Construct a response message based on the program state.
        void
        create_get_response() {

            if (request_.target() == "/1") {
                create_get_response_image(1);
                return;
            }
            if (request_.target() == "/2") {
                create_get_response_image(2);
                return;
            }
            if (request_.target() == "/3") {
                create_get_response_image(3);
                return;
            }

            auto response = std::make_shared<boost::beast::http::response<boost::beast::http::dynamic_body>>();
            response->version(request_.version());
            response->keep_alive(false);

            response->set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);

            if (request_.target() == "/") {
                response->result(boost::beast::http::status::ok);
                response->set(boost::beast::http::field::content_type, "text/html");
                boost::beast::ostream(response->body())
                        << "<html>\n"
                        << "<head><title>Current time</title></head>\n"
                        << "<body>\n"
                        << "<h1>Current time</h1>\n"
                        << "<p>The current time is "
                        << std::time(nullptr)
                        << " seconds since the epoch.</p>\n"
                        << "</body>\n"
                        << "</html>\n";
                response->content_length(response->body().size());
                write_response(response);
                return;
            } else {
                response->result(boost::beast::http::status::not_found);
                response->set(boost::beast::http::field::content_type, "text/plain");
                boost::beast::ostream(response->body()) << "File not found\r\n";
                response->content_length(response->body().size());
                write_response(response);
                return;
            }
        }

        // Asynchronously transmit the response message.
        template<typename BodyType =boost::beast::http::dynamic_body>
        void write_response(
                std::shared_ptr<boost::beast::http::response<BodyType>> response
        ) {
            auto self = shared_from_this();

            boost::beast::http::async_write(
                    socket_,
                    *response,
                    [self, response](boost::beast::error_code ec, std::size_t) {
                        self->socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
                        self->deadline_.cancel();
                    });
        }

        // Check whether we have spent enough time on this connection.
        void
        check_deadline() {
            auto self = shared_from_this();

            deadline_.async_wait(
                    [self](boost::beast::error_code ec) {
                        if (!ec) {
                            // Close socket to cancel any outstanding operation.
                            self->socket_.close(ec);
                        }
                    });
        }
    };


    class ImageServiceHttp : public std::enable_shared_from_this<ImageServiceHttp> {
    public:
        ImageServiceHttp(
                boost::asio::io_context &ioc,
                const boost::asio::ip::tcp::endpoint &endpoint,
                OwlMailDefine::ServiceCameraMailbox &&mailbox
        ) : ioc_(ioc),
            acceptor_(boost::asio::make_strand(ioc)),
            mailbox_(std::move(mailbox)) {
            mailbox_->receiveB2A = [this](OwlMailDefine::MailCamera2Service &&data) {
                receiveMail(std::move(data));
            };

            boost::beast::error_code ec;

            // Open the acceptor
            acceptor_.open(endpoint.protocol(), ec);
            if (ec) {
                BOOST_LOG_TRIVIAL(error) << "open" << " : " << ec.message();
                return;
            }

            // Allow address reuse
            acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
            if (ec) {
                BOOST_LOG_TRIVIAL(error) << "set_option" << " : " << ec.message();
                return;
            }

            // Bind to the server address
            acceptor_.bind(endpoint, ec);
            if (ec) {
                BOOST_LOG_TRIVIAL(error) << "bind" << " : " << ec.message();
                return;
            }

            // Start listening for connections
            acceptor_.listen(
                    boost::asio::socket_base::max_listen_connections, ec);
            if (ec) {
                BOOST_LOG_TRIVIAL(error) << "listen" << " : " << ec.message();
                return;
            }
        }

        ~ImageServiceHttp() {
            mailbox_->receiveB2A = nullptr;
        }

    private:
        boost::asio::io_context &ioc_;
        boost::asio::ip::tcp::acceptor acceptor_;
        OwlMailDefine::ServiceCameraMailbox mailbox_;
    public:
        // Start accepting incoming connections
        void
        start() {
            do_accept();
        }

    private:
        friend ImageServiceHttpConnect;

        void receiveMail(OwlMailDefine::MailCamera2Service &&data) {
            // get callback from data and call it to send back image result
            data->runner(data);
        }

        void sendMail(OwlMailDefine::MailService2Camera &&data) {
            // send cmd to camera
            mailbox_->sendA2B(std::move(data));
        }

    private:
        void
        do_accept() {
            // The new connection gets its own strand
            acceptor_.async_accept(
                    boost::asio::make_strand(ioc_),
                    boost::beast::bind_front_handler(
                            &ImageServiceHttp::on_accept,
                            shared_from_this()));
        }

        void
        on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket) {
            if (ec) {
                BOOST_LOG_TRIVIAL(error) << "accept" << " : " << ec.message();
            } else {
                // Create the session and run it
                std::make_shared<ImageServiceHttpConnect>(
                        ioc_,
                        std::move(socket),
                        weak_from_this()
                )->start();
            }

            // Accept another connection
            do_accept();
        }
    };

} // OwlImageServiceHttp

#endif //OWLACCESSTERMINAL_IMAGESERVICEHTTP_H
