// jeremie

#include "ImageServiceHttp.h"
#include <boost/lexical_cast.hpp>
#include <boost/lexical_cast/try_lexical_convert.hpp>
#include <regex>
#include "../QueryPairsAnalyser/QueryPairsAnalyser.h"

namespace OwlImageServiceHttp {
    void ImageServiceHttpConnect::create_get_response_image(int camera_id) {

        auto p = parents_.lock();
        if (!p) {
            // inner error
            auto response = std::make_shared<boost::beast::http::response<boost::beast::http::dynamic_body>>();
            response->version(request_.version());
            response->keep_alive(false);

            response->set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);

            response->result(boost::beast::http::status::internal_server_error);
            response->set(boost::beast::http::field::content_type, "text/plain");
            boost::beast::ostream(response->body()) << "(!parents_.lock())\r\n";
            response->content_length(response->body().size());
            write_response(response);
            return;
        }

        OwlMailDefine::MailService2Camera cmd_data = std::make_shared<OwlMailDefine::Service2Camera>();
        cmd_data->camera_id = camera_id;

        cmd_data->callbackRunner = [this, self = shared_from_this()](
                const OwlMailDefine::MailCamera2Service &camera_data
        ) {

            // now, send back
            if (!camera_data->ok) {

                // try to run immediately if now on the same strand, or run it later
                boost::asio::dispatch(socket_.get_executor(), [this, self = shared_from_this()]() {
                    auto response = std::make_shared<boost::beast::http::response<boost::beast::http::dynamic_body>>();
                    response->version(request_.version());
                    response->keep_alive(false);

                    response->set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);

                    response->result(boost::beast::http::status::internal_server_error);
                    response->set(boost::beast::http::field::content_type, "text/plain");
                    boost::beast::ostream(response->body()) << "(!camera_data->ok)\r\n";
                    response->content_length(response->body().size());
                    write_response(response);
                });
                return;
            }

//            BOOST_LOG_TRIVIAL(info) << "create_get_response_image cmd_data->callbackRunner "
//                                    << camera_data->camera_id;

            // try to run immediately if now on the same strand, or run it later
            boost::asio::dispatch(socket_.get_executor(), [this, self = shared_from_this(), camera_data]() {


//                BOOST_LOG_TRIVIAL(info) << "create_get_response_image cmd_data->callbackRunner dispatch "
//                                        << camera_data->camera_id;

//              cv::Mat img{6, 6, CV_8UC3, cv::Scalar{0, 0, 0}};
                cv::Mat img = camera_data->image;
                camera_data->image.release();

                auto imageBuffer = std::make_shared<std::vector<uchar>>();
                cv::imencode(".jpg", img, *imageBuffer,
                             {cv::ImwriteFlags::IMWRITE_JPEG_QUALITY, 70});


                // https://www.boost.org/doc/libs/develop/libs/beast/example/doc/http_examples.hpp
                //      `send_cgi_response()`
                auto response = std::make_shared<boost::beast::http::response<boost::beast::http::buffer_body>>();
                response->version(request_.version());
                response->keep_alive(false);

                response->result(boost::beast::http::status::ok);
                response->set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);

                response->set(boost::beast::http::field::content_type, "image/jpeg");

                response->set("X-image-height", boost::lexical_cast<std::string>(img.rows));
                response->set("X-image-width", boost::lexical_cast<std::string>(img.cols));
                response->set("X-image-pixel-channel", boost::lexical_cast<std::string>(img.channels()));
                response->set("X-image-format", "jpg");

                img.release();

                response->body().data = imageBuffer->data();
                response->body().size = imageBuffer->size();


                boost::beast::http::async_write(
                        socket_,
                        *response,
                        [self, response, imageBuffer](boost::beast::error_code ec, std::size_t) {
                            self->socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
                            self->deadline_.cancel();
                        });

                return;


            });


        };

        p->sendMail(std::move(cmd_data));

    }

    void ImageServiceHttpConnect::create_get_response_set_camera_image_size() {

        if (!request_.target().starts_with("/set_camera_image_size=")) {
            // inner error
            auto response = std::make_shared<boost::beast::http::response<boost::beast::http::dynamic_body>>();
            response->version(request_.version());
            response->keep_alive(false);

            response->set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);

            response->result(boost::beast::http::status::internal_server_error);
            response->set(boost::beast::http::field::content_type, "text/plain");
            boost::beast::ostream(response->body())
                    << "(!request_.target().starts_with(\"/set_camera_image_size=\"))\r\n";
            response->content_length(response->body().size());
            write_response(response);
            return;
        }

        auto make_bad_request = [this](const std::string &r) {
            // bad request
            auto response = std::make_shared<boost::beast::http::response<boost::beast::http::dynamic_body>>();
            response->version(request_.version());
            response->keep_alive(false);

            response->set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);

            response->result(boost::beast::http::status::bad_request);
            response->set(boost::beast::http::field::content_type, "text/plain");
            boost::beast::ostream(response->body()) << r << "\r\n";
            response->content_length(response->body().size());
            write_response(response);
        };

        // "/set_camera_image_size?camera=1&x=1920&y=1080"
        auto q = OwlQueryPairsAnalyser::QueryPairsAnalyser(request_.target()).queryPairs;

        if (q.count("camera") != 1 || q.count("x") != 1 || q.count("y") != 1) {
            // bad request
            make_bad_request(
                    R"(create_get_response_set_camera_image_size (q.count("camera") != 1 || q.count("x") != 1 || q.count("y") != 1))"
            );
            return;
        }

        int camera_id;
        int x;
        int y;
        if (!boost::conversion::try_lexical_convert(q.find("camera")->second, camera_id) ||
            !boost::conversion::try_lexical_convert(q.find("x")->second, x) ||
            !boost::conversion::try_lexical_convert(q.find("y")->second, y) ||
            camera_id < 1 || x < 2 || y < 2) {
            // bad request
            make_bad_request("create_get_response_set_camera_image_size (try_lexical)");
            return;
        }

        OwlCameraConfig::CameraAddrType addr = OwlCameraConfig::CameraAddrType_1_Placeholder;
        if (q.contains("addr") && q.find("addr")->second.length() > 0) {
            OwlCameraConfig::CameraAddrType_1 addr2;
            if (boost::conversion::try_lexical_convert(q.find("addr")->second, addr2)) {
                addr = addr2;
            } else {
                addr = q.find("addr")->second;
            }
        }
        std::string apiType = OwlCameraConfig::Camera_VideoCaptureAPI_Placeholder;
        if (q.contains("api") && q.find("api")->second.length() > 0) {
            apiType = q.find("api")->second;
        }


        auto p = parents_.lock();
        if (!p) {
            // inner error
            auto response = std::make_shared<boost::beast::http::response<boost::beast::http::dynamic_body>>();
            response->version(request_.version());
            response->keep_alive(false);

            response->set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);

            response->result(boost::beast::http::status::internal_server_error);
            response->set(boost::beast::http::field::content_type, "text/plain");
            boost::beast::ostream(response->body()) << "(!parents_.lock())\r\n";
            response->content_length(response->body().size());
            write_response(response);
            return;
        }

        // TOD send cmd to let camera re-create
        OwlMailDefine::MailService2Camera cmd_data = std::make_shared<OwlMailDefine::Service2Camera>();
        cmd_data->camera_id = camera_id;
        cmd_data->cmd = OwlMailDefine::ControlCameraCmd::reset;
        cmd_data->cmdParams = {OwlCameraConfig::CameraInfoTuple{
                camera_id,
                addr,
                apiType,
                x,
                y,
        }};

        cmd_data->callbackRunner = [this, self = shared_from_this()](
                const OwlMailDefine::MailCamera2Service &camera_data
        ) {

            // now, send back
            if (!camera_data->ok) {

                // try to run immediately if now on the same strand, or run it later
                boost::asio::dispatch(socket_.get_executor(), [this, self = shared_from_this()]() {
                    auto response = std::make_shared<boost::beast::http::response<boost::beast::http::dynamic_body>>();
                    response->version(request_.version());
                    response->keep_alive(false);

                    response->set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);

                    response->result(boost::beast::http::status::internal_server_error);
                    response->set(boost::beast::http::field::content_type, "text/plain");
                    boost::beast::ostream(response->body()) << "(!camera reset->ok)\r\n";
                    response->content_length(response->body().size());
                    write_response(response);
                });
                return;
            }

            boost::asio::dispatch(socket_.get_executor(), [this, self = shared_from_this()]() {
                auto response = std::make_shared<boost::beast::http::response<boost::beast::http::dynamic_body>>();
                response->version(request_.version());
                response->keep_alive(false);

                response->set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);

                response->result(boost::beast::http::status::ok);
                response->set(boost::beast::http::field::content_type, "text/plain");
                boost::beast::ostream(response->body()) << "200\r\n";
                response->content_length(response->body().size());
                write_response(response);
            });
            return;

        };

        p->sendMail(std::move(cmd_data));

    }

    void ImageServiceHttpConnect::create_get_response() {

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
        if (request_.target() == "/down") {
            create_get_response_image(downCameraId_);
            return;
        }
        if (request_.target() == "/front") {
            create_get_response_image(frontCameraId_);
            return;
        }
        if (request_.target().starts_with("/set_camera_image_size?")) {
            create_get_response_set_camera_image_size();
            return;
        }

        auto response = std::make_shared<boost::beast::http::response<boost::beast::http::dynamic_body>>();
        response->version(request_.version());
        response->keep_alive(false);

        response->set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);

        if (request_.target() == "/downCameraId") {
            response->result(boost::beast::http::status::ok);
            response->set(boost::beast::http::field::content_type, "text/json");
            boost::beast::ostream(response->body()) << R"({"downCameraId": )" << downCameraId_ << R"( })";
            response->content_length(response->body().size());
            write_response(response);
            return;
        }

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

    void ImageServiceHttpConnect::process_request() {

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

    ImageServiceHttp::ImageServiceHttp(
            boost::asio::io_context &ioc,
            const boost::asio::ip::tcp::endpoint &endpoint,
            int downCameraId,
            int frontCameraId,
            OwlMailDefine::ServiceCameraMailbox &&mailbox
    ) : ioc_(ioc),
        acceptor_(boost::asio::make_strand(ioc)),
        mailbox_(std::move(mailbox)),
        downCameraId_(downCameraId),
        frontCameraId_(frontCameraId) {

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
} // OwlImageServiceHttp