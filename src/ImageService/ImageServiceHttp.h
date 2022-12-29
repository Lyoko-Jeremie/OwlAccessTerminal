// jeremie

#ifndef OWLACCESSTERMINAL_IMAGESERVICEHTTP_H
#define OWLACCESSTERMINAL_IMAGESERVICEHTTP_H

#include <memory>
#include "ImageServiceMail.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>


namespace OwlImageServiceHttp {


    class ImageServiceHttpConnect : public std::enable_shared_from_this<ImageServiceHttpConnect> {
    public:
        ImageServiceHttpConnect(boost::asio::ip::tcp::socket socket)
                : socket_(std::move(socket)) {
        }

        // Initiate the asynchronous operations associated with the connection.
        void
        start() {
            read_request();
            check_deadline();
        }

    private:
        // The socket for the currently connected client.
        boost::asio::ip::tcp::socket socket_;

        // The buffer for performing reads.
        boost::beast::flat_buffer buffer_{8192};

        // The request message.
        boost::beast::http::request<boost::beast::http::dynamic_body> request_;

        // The response message.
        boost::beast::http::response<boost::beast::http::dynamic_body> response_;

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
            response_.version(request_.version());
            response_.keep_alive(false);

            switch (request_.method()) {
                case boost::beast::http::verb::get:
                    response_.result(boost::beast::http::status::ok);
                    response_.set(boost::beast::http::field::server, "Beast");
                    create_response();
                    break;

                default:
                    // We return responses indicating an error if
                    // we do not recognize the request method.
                    response_.result(boost::beast::http::status::bad_request);
                    response_.set(boost::beast::http::field::content_type, "text/plain");
                    boost::beast::ostream(response_.body())
                            << "Invalid request-method '"
                            << std::string(request_.method_string())
                            << "'";
                    break;
            }

            write_response();
        }

        // Construct a response message based on the program state.
        void
        create_response() {
            if (request_.target() == "/count") {
//                response_.set(boost::beast::http::field::content_type, "text/html");
//                boost::beast::ostream(response_.body())
//                        << "<html>\n"
//                        <<  "<head><title>Request count</title></head>\n"
//                        <<  "<body>\n"
//                        <<  "<h1>Request count</h1>\n"
//                        <<  "<p>There have been "
//                        <<  my_program_state::request_count()
//                        <<  " requests so far.</p>\n"
//                        <<  "</body>\n"
//                        <<  "</html>\n";
//            }
//            else if(request_.target() == "/time")
//            {
//                response_.set(boost::beast::http::field::content_type, "text/html");
//                boost::beast::ostream(response_.body())
//                        <<  "<html>\n"
//                        <<  "<head><title>Current time</title></head>\n"
//                        <<  "<body>\n"
//                        <<  "<h1>Current time</h1>\n"
//                        <<  "<p>The current time is "
//                        <<  my_program_state::now()
//                        <<  " seconds since the epoch.</p>\n"
//                        <<  "</body>\n"
//                        <<  "</html>\n";
            } else {
                response_.result(boost::beast::http::status::not_found);
                response_.set(boost::beast::http::field::content_type, "text/plain");
                boost::beast::ostream(response_.body()) << "File not found\r\n";
            }
        }

        // Asynchronously transmit the response message.
        void
        write_response() {
            auto self = shared_from_this();

            response_.content_length(response_.body().size());

            boost::beast::http::async_write(
                    socket_,
                    response_,
                    [self](boost::beast::error_code ec, std::size_t) {
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
            // TODO
        }

    };

} // OwlImageServiceHttp

#endif //OWLACCESSTERMINAL_IMAGESERVICEHTTP_H
