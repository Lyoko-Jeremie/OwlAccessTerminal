// jeremie

#include "EmbedWebServer.h"
#include "EmbedWebServerSession.h"
#include "EmbedWebServerTools.h"

#include <filesystem>
#include <boost/algorithm/string.hpp>
#include <boost/asio/strand.hpp>

namespace OwlEmbedWebServer {


//------------------------------------------------------------------------------

    EmbedWebServer::EmbedWebServer(boost::asio::io_context &ioc,
                                   OwlMailDefine::WebCmdMailbox &&mailbox,
                                   const boost::asio::ip::tcp::endpoint &endpoint,
                                   const boost::shared_ptr<const std::string> &doc_root,
                                   boost::shared_ptr<std::string const> const &index_file_of_root,
                                   boost::shared_ptr<std::string const> const &backend_json_string,
                                   boost::shared_ptr<std::string const> const &_allowFileExtList)
            : ioc_(ioc),
              mailbox_(std::move(mailbox)),
              acceptor_(boost::asio::make_strand(ioc)),
              doc_root_(doc_root),
              index_file_of_root(index_file_of_root),
              backend_json_string(backend_json_string) {

        if (_allowFileExtList) {
            std::vector<std::string> extList;
            boost::split(extList, *_allowFileExtList, boost::is_any_of(" "));
            allowFileExtList = extList;
        }

        mailbox_->receiveB2A = [this](OwlMailDefine::MailCmd2Web &&data) {
            receiveMail(std::move(data));
        };

        boost::beast::error_code ec;

        // Open the acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if (ec) {
            fail(ec, "open");
            return;
        }

        // Allow address reuse
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
        if (ec) {
            fail(ec, "set_option");
            return;
        }

        // Bind to the server address
        acceptor_.bind(endpoint, ec);
        if (ec) {
            fail(ec, "bind");
            return;
        }

        // Start listening for connections
        acceptor_.listen(
                boost::asio::socket_base::max_listen_connections, ec);
        if (ec) {
            fail(ec, "listen");
            return;
        }
    }

    void EmbedWebServer::do_accept() {
        // The new connection gets its own strand
        acceptor_.async_accept(
                boost::asio::make_strand(ioc_),
                boost::beast::bind_front_handler(
                        &EmbedWebServer::on_accept,
                        shared_from_this()));
    }

    void EmbedWebServer::on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket) {
        if (ec) {
            fail(ec, "accept");
        } else {
            // Create the session and run it
            boost::make_shared<EmbedWebServerSession>(
                    weak_from_this(),
                    std::move(socket),
                    doc_root_,
                    index_file_of_root,
                    backend_json_string,
                    allowFileExtList
            )->run();
        }

        // Accept another connection
        do_accept();
    }


}
