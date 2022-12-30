// jeremie

#include "ImageServiceHttp.h"

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
                OwlMailDefine::MailCamera2Service camera_data
        ) {

            // now, send back
            if (!camera_data->ok) {

                auto response = std::make_shared<boost::beast::http::response<boost::beast::http::dynamic_body>>();
                response->version(request_.version());
                response->keep_alive(false);

                response->set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);

                response->result(boost::beast::http::status::internal_server_error);
                response->set(boost::beast::http::field::content_type, "text/plain");
                boost::beast::ostream(response->body()) << "(!camera_data->ok)\r\n";
                response->content_length(response->body().size());
                write_response(response);
                return;
            }


            // try to run immediately if now on this ioc, or run it later
            boost::asio::dispatch(ioc_, [this, self = shared_from_this(), camera_data]() {

//              cv::Mat img{6, 6, CV_8UC3, cv::Scalar{0, 0, 0}};
                cv::Mat img = camera_data->image;
                camera_data->image.release();

                auto imageBuffer = std::make_shared<std::vector<uchar>>();
                cv::imencode(".jpg", img, *imageBuffer,
                             {cv::ImwriteFlags::IMWRITE_JPEG_QUALITY, 70});
                img.release();

                // https://www.boost.org/doc/libs/develop/libs/beast/example/doc/http_examples.hpp
                //      `send_cgi_response()`
                auto response = std::make_shared<boost::beast::http::response<boost::beast::http::buffer_body>>();
                response->version(request_.version());
                response->keep_alive(false);

                response->result(boost::beast::http::status::ok);
                response->set(boost::beast::http::field::content_type, "image/jpeg");

                response->body().data = imageBuffer->data();
                response->body().size = imageBuffer->size();

                boost::beast::http::async_write(
                        socket_,
                        *response,
                        [self, response, imageBuffer](boost::beast::error_code ec, std::size_t) { ;
                            self->socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
                            self->deadline_.cancel();
                        });

                return;


            });


        };

        p->sendMail(std::move(cmd_data));

    }
} // OwlImageServiceHttp