// jeremie

#include "EmbedWebServerSession.h"
#include "EmbedWebServerTools.h"

#include <filesystem>
#include <boost/algorithm/string.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/beast/version.hpp>

namespace OwlEmbedWebServer {

    template<bool isRequest, class Body, class Fields>
    void
    EmbedWebServerSession::send_lambda::operator()(boost::beast::http::message<isRequest, Body, Fields> &&msg) const {
        // The lifetime of the message has to extend
        // for the duration of the async operation so
        // we use a shared_ptr to manage it.
        auto sp = std::make_shared<
                boost::beast::http::message<isRequest, Body, Fields>>(std::move(msg));

        // Store a type-erased version of the shared
        // pointer in the class to keep it alive.
        self_.res_ = sp;

        // Write the response
        boost::beast::http::async_write(
                self_.stream_,
                *sp,
                boost::beast::bind_front_handler(
                        &EmbedWebServerSession::on_write,
                        self_.shared_from_this(),
                        sp->need_eof()));
    }

//------------------------------------------------------------------------------

    void EmbedWebServerSession::run() {
        // We need to be executing within a strand to perform async operations
        // on the I/O objects in this session. Although not strictly necessary
        // for single-threaded contexts, this example code is written to be
        // thread-safe by default.
        boost::asio::dispatch(stream_.get_executor(),
                              boost::beast::bind_front_handler(
                                      &EmbedWebServerSession::do_read,
                                      shared_from_this()));
    }

    void EmbedWebServerSession::do_read() {
        // Make the request empty before reading,
        // otherwise the operation behavior is undefined.
        req_ = {};

        // Set the timeout.
        stream_.expires_after(std::chrono::seconds(30));

        // Read a request
        boost::beast::http::async_read(stream_, buffer_, req_,
                                       boost::beast::bind_front_handler(
                                               &EmbedWebServerSession::on_read,
                                               shared_from_this()));
    }

    void EmbedWebServerSession::on_read(boost::beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        // This means they closed the connection
        if (ec == boost::beast::http::error::end_of_stream)
            return do_close();

        if (ec)
            return fail(ec, "read");


        std::cout << "req_.target():" << req_.target() << std::endl;
        if (req_.method() == boost::beast::http::verb::get) {
            // answer backend json
            if (std::string{req_.target()} == std::string{"/backend"}) {
                boost::beast::http::response<boost::beast::http::string_body> res{
                        boost::beast::http::status::ok,
                        req_.version()};
                res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(boost::beast::http::field::content_type, "text/json");
                res.keep_alive(req_.keep_alive());
                res.body() = std::string(*backend_json_string);
                res.prepare_payload();
                return lambda_(std::move(res));
            }
        }

        // Send the response
        handle_request(*doc_root_, std::move(req_), lambda_, index_file_of_root, allowFileExtList);
    }

    void EmbedWebServerSession::on_write(bool close, boost::beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        if (ec)
            return fail(ec, "write");

        if (close) {
            // This means we should close the connection, usually because
            // the response indicated the "Connection: close" semantic.
            return do_close();
        }

        // We're done with the response so delete it
        res_ = nullptr;

        // Read another request
        do_read();
    }

    void EmbedWebServerSession::do_close() {
        // Send a TCP shutdown
        boost::beast::error_code ec;
        stream_.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);

        // At this point the connection is closed gracefully
    }


} // OwlWebControlService