// jeremie

#ifndef OWLACCESSTERMINAL_EMBEDWEBSERVERSESSION_H
#define OWLACCESSTERMINAL_EMBEDWEBSERVERSESSION_H

// https://www.boost.org/doc/libs/develop/libs/beast/example/http/server/async/http_server_async.cpp

#include "../../MemoryBoost.h"
#include <string>
#include <iostream>
#include <sstream>
#include <boost/asio/executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <utility>

#include "../WebCmdMail.h"
#include "EmbedWebServer.h"

namespace OwlEmbedWebServer {

    struct EmbedWebServerSessionHelperCmd;

    // Handles an HTTP server connection
    class EmbedWebServerSession : public boost::enable_shared_from_this<EmbedWebServerSession> {
        // This is the C++11 equivalent of a generic lambda.
        // The function object is used to send an HTTP message.
        struct send_lambda {
            EmbedWebServerSession &self_;

            explicit
            send_lambda(EmbedWebServerSession &self)
                    : self_(self) {
            }

            template<bool isRequest, class Body, class Fields>
            void
            operator()(boost::beast::http::message<isRequest, Body, Fields> &&msg) const;
        };

        boost::weak_ptr<EmbedWebServer> parentRef_;
        boost::beast::tcp_stream stream_;
        boost::beast::flat_buffer buffer_;
        boost::shared_ptr<std::string const> doc_root_;
        boost::shared_ptr<std::string const> index_file_of_root;
        boost::shared_ptr<std::string const> backend_json_string;
        std::vector<std::string> allowFileExtList;
        boost::beast::http::request<boost::beast::http::string_body> req_;
        boost::shared_ptr<void> res_;
        send_lambda lambda_;

        friend EmbedWebServerSessionHelperCmd;

    public:
        // Take ownership of the stream
        EmbedWebServerSession(
                boost::weak_ptr<EmbedWebServer> parentRef,
                boost::asio::ip::tcp::socket &&socket,
                boost::shared_ptr<std::string const> const &doc_root,
                boost::shared_ptr<std::string const> const &index_file_of_root,
                boost::shared_ptr<std::string const> const &backend_json_string,
                std::vector<std::string> const &allowFileExtList)
                : parentRef_(std::move(parentRef)),
                  stream_(std::move(socket)),
                  doc_root_(doc_root),
                  index_file_of_root(index_file_of_root),
                  backend_json_string(backend_json_string),
                  allowFileExtList(allowFileExtList),
                  lambda_(*this) {
        }

        // Start the asynchronous operation
        void
        run();

        void
        do_read();

        void
        on_read(
                boost::beast::error_code ec,
                std::size_t bytes_transferred);

        void
        on_write(
                bool close,
                boost::beast::error_code ec,
                std::size_t bytes_transferred);

        void
        do_close();


        void
        on_cmd();

        void server_error(std::string what);

        void bad_request(std::string what);

        void send_EmptyPairs_error();

        void send_json(boost::json::value &&o);
    };


} // OwlWebControlService

#endif //OWLACCESSTERMINAL_EMBEDWEBSERVERSESSION_H
