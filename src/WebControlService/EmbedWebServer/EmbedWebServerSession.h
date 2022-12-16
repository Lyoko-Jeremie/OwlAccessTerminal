// jeremie

#ifndef OWLACCESSTERMINAL_EMBEDWEBSERVERSESSION_H
#define OWLACCESSTERMINAL_EMBEDWEBSERVERSESSION_H

// https://www.boost.org/doc/libs/develop/libs/beast/example/http/server/async/http_server_async.cpp

#include <memory>
#include <string>
#include <iostream>
#include <sstream>
#include <boost/asio/executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include "../WebCmdMail.h"

namespace OwlEmbedWebServer {

    // Handles an HTTP server connection
    class EmbedWebServerSession : public std::enable_shared_from_this<EmbedWebServerSession> {
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

        OwlMailDefine::WebCmdMailbox mailbox_;
        boost::beast::tcp_stream stream_;
        boost::beast::flat_buffer buffer_;
        std::shared_ptr<std::string const> doc_root_;
        std::shared_ptr<std::string const> index_file_of_root;
        std::shared_ptr<std::string const> backend_json_string;
        std::vector<std::string> allowFileExtList;
        boost::beast::http::request<boost::beast::http::string_body> req_;
        std::shared_ptr<void> res_;
        send_lambda lambda_;

    public:
        // Take ownership of the stream
        EmbedWebServerSession(
                OwlMailDefine::WebCmdMailbox &&mailbox,
                boost::asio::ip::tcp::socket &&socket,
                std::shared_ptr<std::string const> const &doc_root,
                std::shared_ptr<std::string const> const &index_file_of_root,
                std::shared_ptr<std::string const> const &backend_json_string,
                std::vector<std::string> const &allowFileExtList)
                : mailbox_(std::move(mailbox)),
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
    };


} // OwlWebControlService

#endif //OWLACCESSTERMINAL_EMBEDWEBSERVERSESSION_H
