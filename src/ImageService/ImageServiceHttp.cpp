// jeremie

#include "ImageServiceHttp.h"
#include <boost/lexical_cast.hpp>
#include <boost/lexical_cast/try_lexical_convert.hpp>
#include <regex>
#include <utility>
#include <boost/url.hpp>
#include "../QueryPairsAnalyser/QueryPairsAnalyser.h"

namespace OwlImageServiceHttp {
    void ImageServiceHttpConnect::create_get_response_image(int camera_id) {

        auto p = parents_.lock();
        if (!p) {
            // inner error
            internal_server_error("(!parents_.lock())");
            return;
        }

        OwlMailDefine::MailService2Camera cmd_data = boost::make_shared<OwlMailDefine::Service2Camera>();
        cmd_data->camera_id = camera_id;

        cmd_data->callbackRunner = [this, self = shared_from_this()](
                const OwlMailDefine::MailCamera2Service &camera_data
        ) {
            auto p = parents_.lock();
            if (!p) {
                // inner error
                internal_server_error("(!parents_.lock())");
                return;
            }

            // now, send back
            if (!camera_data->ok) {

                // try to run immediately if now on the same strand, or run it later
                boost::asio::dispatch(socket_.get_executor(), [this, self = shared_from_this()]() {
                    internal_server_error("(!camera_data->ok)");
                });
                return;
            }

            OwlMailDefine::MailService2Time time_data = boost::make_shared<OwlMailDefine::Service2Time>();
            time_data->cmd = OwlMailDefine::TimeServiceCmd::getSyncClock;
            time_data->callbackRunner = [this, self = shared_from_this(), camera_data](
                    const OwlMailDefine::MailTime2Service &time_data_r
            ) {
                std::string time_string;
                if (!boost::conversion::try_lexical_convert<std::string>(time_data_r->clockTimestampMs, time_string)) {
                    BOOST_LOG_OWL(error)
                        << "ImageServiceHttpConnect::create_get_response_image try_lexical_convert error";
                    time_string = "0";
                    boost::asio::dispatch(socket_.get_executor(), [this, self = shared_from_this()]() {
                        internal_server_error(
                                "ImageServiceHttpConnect::create_get_response_image try_lexical_convert error");
                    });
                    return;
                }

                // try to run immediately if now on the same strand, or run it later
                boost::asio::dispatch(socket_.get_executor(), [
                        this, self = shared_from_this(), camera_data, time_string
                ]() {

//              cv::Mat img{6, 6, CV_8UC3, cv::Scalar{0, 0, 0}};
                    cv::Mat img = camera_data->image;
                    camera_data->image.release();

                    auto imageBuffer = boost::make_shared<std::vector<uchar>>();
                    cv::imencode(".jpg", img, *imageBuffer,
                                 {cv::ImwriteFlags::IMWRITE_JPEG_QUALITY, 70});


                    // https://www.boost.org/doc/libs/develop/libs/beast/example/doc/http_examples.hpp
                    //      `send_cgi_response()`
                    auto response = boost::make_shared<boost::beast::http::response<boost::beast::http::buffer_body>>();
                    response->version(request_.version());
                    response->keep_alive(false);

                    response->result(boost::beast::http::status::ok);
                    response->set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);

                    response->set(boost::beast::http::field::content_type, "image/jpeg");

                    response->set("X-image-height", boost::lexical_cast<std::string>(img.rows));
                    response->set("X-image-width", boost::lexical_cast<std::string>(img.cols));
                    response->set("X-image-pixel-channel", boost::lexical_cast<std::string>(img.channels()));
                    response->set("X-image-format", "jpg");
                    response->set("X-SteadyClockTimestampMs", time_string);

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

            p->sendMailTime(std::move(time_data));

        };

        p->sendMail(std::move(cmd_data));

    }

    void ImageServiceHttpConnect::create_get_response_set_camera_image_size() {

        if (!request_.target().starts_with("/set_camera_image_size?")) {
            // inner error
            internal_server_error("(!request_.target().starts_with(\"/set_camera_image_size?\"))");
            return;
        }

        // "/set_camera_image_size?camera=1&x=1920&y=1080"
        auto q = OwlQueryPairsAnalyser::QueryPairsAnalyser(request_.target()).queryPairs;

        if (q.count("camera") != 1 || q.count("x") != 1 || q.count("y") != 1) {
            // bad request
            bad_request(
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
            bad_request("create_get_response_set_camera_image_size (try_lexical)");
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
            internal_server_error("(!parents_.lock())");
            return;
        }

        // send cmd to let camera re-create
        OwlMailDefine::MailService2Camera cmd_data = boost::make_shared<OwlMailDefine::Service2Camera>();
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
                    internal_server_error("(!camera reset->ok)");
                });
                return;
            }

            boost::asio::dispatch(socket_.get_executor(), [this, self = shared_from_this()]() {
                auto response = boost::make_shared<boost::beast::http::response<boost::beast::http::dynamic_body>>();
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

    void ImageServiceHttpConnect::create_get_response_set_camera_direct() {

        if (!request_.target().starts_with("/set_camera_direct?")) {
            // inner error
            internal_server_error("(!request_.target().starts_with(\"/set_camera_image_size?\"))");
            return;
        }

        // "/set_camera_direct?camera=1&direct=front"
        auto q = OwlQueryPairsAnalyser::QueryPairsAnalyser(request_.target()).queryPairs;

        if (q.count("camera") != 1 || q.count("direct") != 1) {
            // bad request
            bad_request(
                    R"(create_get_response_set_camera_direct (q.count("camera") != 1 || q.count("direct") != 1))"
            );
            return;
        }

        int camera_id;
        std::string direct = q.find("camera")->second;
        if (!boost::conversion::try_lexical_convert(q.find("camera")->second, camera_id)
            || (direct != "down" && direct != "front")) {
            // bad request
            bad_request("create_get_response_set_camera_direct (try_lexical)");
            return;
        }

        auto p = parents_.lock();
        if (!p) {
            // inner error
            internal_server_error("(!parents_.lock())");
            return;
        }

        if (direct == "down") {
            config_->config().downCameraId.store(camera_id);
        }
        if (direct == "front") {
            config_->config().frontCameraId.store(camera_id);
        }


    }

    void ImageServiceHttpConnect::create_get_response_time() {

        if (!request_.target().starts_with("/time?")) {
            // inner error
            internal_server_error("(!request_.target().starts_with(\"/time?\"))");
            return;
        }

        // "/time?setTimestamp=123456"
        auto q = OwlQueryPairsAnalyser::QueryPairsAnalyser(request_.target()).queryPairs;

        if (q.count("setTimestamp") != 1) {
            // bad request
            bad_request(
                    R"(create_get_response_time (q.count("setTimestamp") != 1))"
            );
            return;
        }

        int64_t set_timestamp;
        if (!boost::conversion::try_lexical_convert(q.find("setTimestamp")->second, set_timestamp)) {
            // bad request
            bad_request("create_get_response_set_camera_direct (try_lexical)");
            return;
        }

        auto p = parents_.lock();
        if (!p) {
            // inner error
            internal_server_error("(!parents_.lock())");
            return;
        }

        auto data = boost::make_shared<OwlMailDefine::Service2Time>();
        data->cmd = OwlMailDefine::TimeServiceCmd::setNowClock;
        data->clockTimestampMs = set_timestamp;

        data->callbackRunner = [this, self = shared_from_this()](
                const OwlMailDefine::MailTime2Service &data_r
        ) {

            boost::asio::dispatch(socket_.get_executor(), [this, self = shared_from_this(), data_r]() {

                std::string ts;
                if (!boost::conversion::try_lexical_convert<std::string>(data_r->clockTimestampMs, ts)) {
                    BOOST_LOG_OWL(error)
                        << "ImageServiceHttpConnect::create_get_response_time try_lexical_convert error";
                    internal_server_error("try_lexical_convert error");
                    return;
                }

                auto js = std::string{R"({"steadyClockTimestampMs": <NUMBER>})"};
                boost::replace_all(js, "<NUMBER>", boost::lexical_cast<std::string>(data_r->clockTimestampMs));

                auto response = boost::make_shared<boost::beast::http::response<boost::beast::http::dynamic_body>>();
                response->version(request_.version());
                response->keep_alive(false);

                response->set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);

                response->result(boost::beast::http::status::ok);
                response->set(boost::beast::http::field::content_type, "text/json");
                boost::beast::ostream(response->body()) << js << "\r\n";
                response->content_length(response->body().size());
                write_response(response);
            });

        };

        p->sendMailTime(std::move(data));

    }

    void ImageServiceHttpConnect::create_get_response_time_now() {

        if (!request_.target().starts_with("/timeGet")) {
            // inner error
            internal_server_error("(!request_.target().starts_with(\"/timeGet?\"))");
            return;
        }

        auto p = parents_.lock();
        if (!p) {
            // inner error
            internal_server_error("(!parents_.lock())");
            return;
        }

        auto data = boost::make_shared<OwlMailDefine::Service2Time>();
        data->cmd = OwlMailDefine::TimeServiceCmd::getSyncClock;

        data->callbackRunner = [this, self = shared_from_this(), p](
                const OwlMailDefine::MailTime2Service &data_r
        ) {

            auto js = boost::json::serialize(
                    boost::json::value{
                            {"syncClock",   data_r->clockTimestampMs},
                            {"steadyClock", std::chrono::time_point_cast<std::chrono::milliseconds>(
                                    std::chrono::steady_clock::now()).time_since_epoch().count()},
                            {"systemClock", std::chrono::time_point_cast<std::chrono::milliseconds>(
                                    std::chrono::system_clock::now()).time_since_epoch().count()},
                    }
            );

            auto response = boost::make_shared<boost::beast::http::response<boost::beast::http::dynamic_body>>();
            response->version(request_.version());
            response->keep_alive(false);

            response->set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);

            response->result(boost::beast::http::status::ok);
            response->set(boost::beast::http::field::content_type, "text/json");
            boost::beast::ostream(response->body()) << js << "\r\n";
            response->content_length(response->body().size());
            write_response(response);

        };

        p->sendMailTime(std::move(data));

    }

    void ImageServiceHttpConnect::create_get_response() {

        auto u = boost::urls::parse_uri_reference(request_.target());

        if (u.has_error()) {
            auto response = boost::make_shared<boost::beast::http::response<boost::beast::http::dynamic_body>>();
            response->version(request_.version());
            response->keep_alive(false);

            response->set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);

            response->result(boost::beast::http::status::bad_request);
            response->set(boost::beast::http::field::content_type, "text/plain");
            boost::beast::ostream(response->body()) << "urls decode fail\r\n";
            response->content_length(response->body().size());
            write_response(response);
            return;
        }

//        BOOST_LOG_OWL(trace) << "ok";
        auto v = u.value();
//        BOOST_LOG_OWL(trace) << "v.path():[" << v.path() << "]";
//        BOOST_LOG_OWL(trace) << "v.query():[" << v.query() << "]";

        if (v.path() == "/1") {
            create_get_response_image(1);
            return;
        }
        if (v.path() == "/2") {
            create_get_response_image(2);
            return;
        }
        if (v.path() == "/3") {
            create_get_response_image(3);
            return;
        }
        if (v.path() == "/down") {
            create_get_response_image(config_->config().downCameraId.load());
            return;
        }
        if (v.path() == "/front") {
            create_get_response_image(config_->config().frontCameraId.load());
            return;
        }
        if (request_.target().starts_with("/set_camera_image_size?")) {
            create_get_response_set_camera_image_size();
            return;
        }
        if (request_.target().starts_with("/set_camera_image_direct?")) {
            create_get_response_set_camera_direct();
            return;
        }
        if (request_.target().starts_with("/time?")) {
            create_get_response_time();
            return;
        }
        if (v.path().starts_with("/timeGet")) {
            create_get_response_time_now();
            return;
        }

        auto response = boost::make_shared<boost::beast::http::response<boost::beast::http::dynamic_body>>();
        response->version(request_.version());
        response->keep_alive(false);

        response->set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);

        if (request_.target() == "/downCameraId") {
            response->result(boost::beast::http::status::ok);
            response->set(boost::beast::http::field::content_type, "text/json");
            boost::beast::ostream(response->body())
                    << R"({"downCameraId": )"
                    << config_->config().downCameraId.load()
                    << R"( })";
            response->content_length(response->body().size());
            write_response(response);
            return;
        }
        if (request_.target() == "/frontCameraId") {
            response->result(boost::beast::http::status::ok);
            response->set(boost::beast::http::field::content_type, "text/json");
            boost::beast::ostream(response->body())
                    << R"({"frontCameraId": )"
                    << config_->config().frontCameraId.load()
                    << R"( })";
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

            case boost::beast::http::verb::post:
                create_get_response();
                break;

            default: {
                auto response = boost::make_shared<boost::beast::http::response<boost::beast::http::dynamic_body>>();
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

    void ImageServiceHttpConnect::bad_request(const std::string &r) {
        // bad request
        auto response = boost::make_shared<boost::beast::http::response<boost::beast::http::dynamic_body>>();
        response->version(request_.version());
        response->keep_alive(false);

        response->set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);

        response->result(boost::beast::http::status::bad_request);
        response->set(boost::beast::http::field::content_type, "text/plain");
        boost::beast::ostream(response->body()) << r << "\r\n";
        response->content_length(response->body().size());
        write_response(response);
    }

    void ImageServiceHttpConnect::internal_server_error(const std::string &r) {
        auto response = boost::make_shared<boost::beast::http::response<boost::beast::http::dynamic_body>>();
        response->version(request_.version());
        response->keep_alive(false);

        response->set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);

        response->result(boost::beast::http::status::internal_server_error);
        response->set(boost::beast::http::field::content_type, "text/plain");
        boost::beast::ostream(response->body()) << r << "\r\n";
        response->content_length(response->body().size());
        write_response(response);
    }

    ImageServiceHttp::ImageServiceHttp(
            boost::asio::io_context &ioc,
            const boost::asio::ip::tcp::endpoint &endpoint,
            boost::shared_ptr<OwlConfigLoader::ConfigLoader> config,
            OwlMailDefine::ServiceCameraMailbox &&mailbox,
            OwlMailDefine::ServiceTimeMailbox &&mailbox_time
    ) : ioc_(ioc),
        acceptor_(boost::asio::make_strand(ioc)),
        config_(std::move(config)),
        mailbox_(std::move(mailbox)),
        mailbox_time_(std::move(mailbox_time)) {

        mailbox_->receiveB2A([this](OwlMailDefine::MailCamera2Service &&data) {
            receiveMail(std::move(data));
        });
        mailbox_time_->receiveB2A([this](OwlMailDefine::MailTime2Service &&data) {
            data->runner(data);
        });

        boost::beast::error_code ec;

        // Open the acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if (ec) {
            BOOST_LOG_OWL(error) << "open" << " : " << ec.message();
            return;
        }

        // Allow address reuse
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
        if (ec) {
            BOOST_LOG_OWL(error) << "set_option" << " : " << ec.message();
            return;
        }

        // Bind to the server address
        acceptor_.bind(endpoint, ec);
        if (ec) {
            BOOST_LOG_OWL(error) << "bind" << " : " << ec.message();
            return;
        }

        // Start listening for connections
        acceptor_.listen(
                boost::asio::socket_base::max_listen_connections, ec);
        if (ec) {
            BOOST_LOG_OWL(error) << "listen" << " : " << ec.message();
            return;
        }
    }
} // OwlImageServiceHttp