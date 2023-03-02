// jeremie

#ifndef OWLACCESSTERMINAL_EMBEDWEBSERVER_H
#define OWLACCESSTERMINAL_EMBEDWEBSERVER_H

// https://www.boost.org/doc/libs/develop/libs/beast/example/http/server/async/http_server_async.cpp

#include "../../MemoryBoost.h"
#include <string>
#include <iostream>
#include <sstream>
#include <boost/asio/executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include "../WebCmdMail.h"

namespace OwlEmbedWebServer {

    //------------------------------------------------------------------------------

    // Accepts incoming connections and launches the sessions
    class EmbedWebServer : public boost::enable_shared_from_this<EmbedWebServer> {
        boost::asio::io_context &ioc_;
        OwlMailDefine::WebCmdMailbox mailbox_;
        boost::asio::ip::tcp::acceptor acceptor_;
        boost::shared_ptr<std::string const> doc_root_;
        boost::shared_ptr<std::string const> index_file_of_root;
        boost::shared_ptr<std::string const> backend_json_string;
        std::vector<std::string> allowFileExtList;

    public:
        EmbedWebServer(
                boost::asio::io_context &ioc,
                OwlMailDefine::WebCmdMailbox &&mailbox,
                const boost::asio::ip::tcp::endpoint &endpoint,
                boost::shared_ptr<std::string const> const &doc_root,
                boost::shared_ptr<std::string const> const &index_file_of_root,
                boost::shared_ptr<std::string const> const &backend_json_string,
                boost::shared_ptr<std::string const> const &allowFileExtList
        );

        ~EmbedWebServer() {
            mailbox_->receiveB2A(nullptr);
        }

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


    public:
        void receiveMail(OwlMailDefine::MailCmd2Web &&data) {
            data->runner(data);
        }

        void sendMail(OwlMailDefine::MailWeb2Cmd &&data) {
            // send cmd
            mailbox_->sendA2B(std::move(data));
        }

    };

}

#endif //OWLACCESSTERMINAL_EMBEDWEBSERVER_H
