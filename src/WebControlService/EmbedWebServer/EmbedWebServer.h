// jeremie

#ifndef OWLACCESSTERMINAL_EMBEDWEBSERVER_H
#define OWLACCESSTERMINAL_EMBEDWEBSERVER_H

// https://www.boost.org/doc/libs/develop/libs/beast/example/http/server/async/http_server_async.cpp

#include <memory>
#include <string>
#include <iostream>
#include <sstream>
#include <boost/asio/executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

namespace OwlEmbedWebServer {

    //------------------------------------------------------------------------------

    // Accepts incoming connections and launches the sessions
    class EmbedWebServer : public std::enable_shared_from_this<EmbedWebServer> {
        boost::asio::io_context &ioc_;
        boost::asio::ip::tcp::acceptor acceptor_;
        std::shared_ptr<std::string const> doc_root_;
        std::shared_ptr<std::string const> index_file_of_root;
        std::shared_ptr<std::string const> backend_json_string;
        std::vector<std::string> allowFileExtList;

    public:
        EmbedWebServer(
                boost::asio::io_context &ioc,
                boost::asio::ip::tcp::endpoint endpoint,
                std::shared_ptr<std::string const> const &doc_root,
                std::shared_ptr<std::string const> const &index_file_of_root,
                std::shared_ptr<std::string const> const &backend_json_string,
                std::shared_ptr<std::string const> const &allowFileExtList
        );

        // Start accepting incoming connections
        void
        start() {
            do_accept();
        }

    private:
        void
        do_accept();

        void
        on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket);
    };

}

#endif //OWLACCESSTERMINAL_EMBEDWEBSERVER_H
