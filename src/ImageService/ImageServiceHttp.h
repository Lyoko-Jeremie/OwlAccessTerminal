// jeremie

#ifndef OWLACCESSTERMINAL_IMAGESERVICEHTTP_H
#define OWLACCESSTERMINAL_IMAGESERVICEHTTP_H

#include "../MemoryBoost.h"
#include "ImageServiceMail.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include "../OwlLog/OwlLog.h"
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include "../MemoryBoost.h"
#include <string>
#include <utility>
#include "../TimeService/TimeServiceMail.h"


namespace OwlImageServiceHttp {

    class ImageServiceHttp;

    class ImageServiceHttpConnect : public boost::enable_shared_from_this<ImageServiceHttpConnect> {
    public:
        ImageServiceHttpConnect(
                boost::asio::ip::tcp::socket &&socket,
                boost::shared_ptr<OwlConfigLoader::ConfigLoader> config,
                boost::weak_ptr<ImageServiceHttp> &&parents
        ) : socket_(std::move(socket)),
            config_(std::move(config)),
            parents_(std::move(parents)) {
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

        boost::shared_ptr<OwlConfigLoader::ConfigLoader> config_;

        boost::weak_ptr<ImageServiceHttp> parents_;

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
                        if (ec) {
                            BOOST_LOG_OWL(warning) << "ImageServiceHttpConnect read_request error: " << ec.what();
                            return;
                        }
                        self->process_request();
                    });
        }

        // Determine what needs to be done with the request message.
        void process_request();

        void bad_request(const std::string &r);

        void internal_server_error(const std::string &r);

        void create_get_response_image(int camera_id);

        void create_get_response_set_camera_image_size();

        void create_get_response_set_camera_direct();

        void create_get_response_time();

        void create_get_response_time_now();

        // Construct a response message based on the program state.
        void create_get_response();

        // Asynchronously transmit the response message.
        template<typename BodyType = boost::beast::http::dynamic_body>
        void write_response(
                boost::shared_ptr<boost::beast::http::response<BodyType>> response
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
                        if (ec == boost::beast::errc::operation_canceled) {
                            return;
                        }
                        if (ec == boost::beast::errc::timed_out) {
                            BOOST_LOG_OWL(warning) << "CmdServiceHttpConnect check_deadline : " << ec.what();
                            return;
                        }
                        if (ec) {
                            BOOST_LOG_OWL(warning) << "ImageServiceHttpConnect check_deadline : " << ec.what();
                            return;
                        }
                        // Close socket to cancel any outstanding operation.
                        self->socket_.close(ec);
                    });
        }
    };


    class ImageServiceHttp : public boost::enable_shared_from_this<ImageServiceHttp> {
    public:
        ImageServiceHttp(
                boost::asio::io_context &ioc,
                const boost::asio::ip::tcp::endpoint &endpoint,
                boost::shared_ptr<OwlConfigLoader::ConfigLoader> config,
                OwlMailDefine::ServiceCameraMailbox &&mailbox,
                OwlMailDefine::ServiceTimeMailbox &&mailbox_time
        );

        ~ImageServiceHttp() {
            BOOST_LOG_OWL(trace_dtor) << "~ImageServiceHttp()";
            mailbox_->receiveB2A(nullptr);
        }

    private:
        boost::asio::io_context &ioc_;
        boost::asio::ip::tcp::acceptor acceptor_;
        boost::shared_ptr<OwlConfigLoader::ConfigLoader> config_;
        OwlMailDefine::ServiceCameraMailbox mailbox_;
        OwlMailDefine::ServiceTimeMailbox mailbox_time_;
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

        void sendMailTime(OwlMailDefine::MailService2Time &&data) {
            // send cmd to camera
            mailbox_time_->sendA2B(std::move(data));
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
                BOOST_LOG_OWL(error) << "accept" << " : " << ec.message();
            } else {
                // Create the session and run it
                boost::make_shared<ImageServiceHttpConnect>(
                        std::move(socket),
                        config_->shared_from_this(),
                        weak_from_this()
                )->start();
            }

            // Accept another connection
            do_accept();

        }
    };

} // OwlImageServiceHttp

#endif //OWLACCESSTERMINAL_IMAGESERVICEHTTP_H
