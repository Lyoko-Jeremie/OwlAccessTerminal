// jeremie

#ifndef OWLACCESSTERMINAL_CMDSERVICEHTTP_H
#define OWLACCESSTERMINAL_CMDSERVICEHTTP_H

#include "../MemoryBoost.h"
#include <vector>
#include <map>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <boost/url.hpp>
#include "../OwlLog/OwlLog.h"
#include "CmdSerialMail.h"
#include "ProcessJsonMessage.h"
#include "../QueryPairsAnalyser/QueryPairsAnalyser.h"
#include "../MapCalc/MapCalcMail.h"

namespace OwlCommandServiceHttp {

    enum {
        JSON_Package_Max_Size = (1024 * 1024 * 1), // 1
        JSON_Package_Max_Size_TagInfo = (1024 * 1024 * 6), // 6M
    };

    class CmdServiceHttp;

    struct CmdServiceHttpConnectCoImpl;

    class CmdServiceHttpConnect
            : public boost::enable_shared_from_this<CmdServiceHttpConnect>,
              public OwlProcessJsonMessage::ProcessJsonMessageSelfTypeInterface {
    public:
        CmdServiceHttpConnect(
                boost::asio::io_context &ioc,
                boost::asio::ip::tcp::socket &&socket,
                boost::weak_ptr<CmdServiceHttp> &&parents
        ) : ioc_(ioc),
            socket_(std::move(socket)),
            parents_(std::move(parents)),
            json_storage_(std::make_unique<decltype(json_storage_)::element_type>(JSON_Package_Max_Size, 0)),
            json_storage_resource_(std::make_unique<decltype(json_storage_resource_)::element_type>
                                           (json_storage_->data(),
                                            json_storage_->size())) {

            json_parse_options_.allow_comments = true;
            json_parse_options_.allow_trailing_commas = true;
            json_parse_options_.max_depth = 5;

        }

        ~CmdServiceHttpConnect() override {
            BOOST_LOG_OWL(trace_dtor) << "CmdServiceHttpConnect::~CmdServiceHttpConnect()";
        }

        // Initiate the asynchronous operations associated with the connection.
        void
        start() {
            read_request();
            check_deadline();
        }

    private:
        boost::asio::io_context &ioc_;

        // The socket for the currently connected client.
        boost::asio::ip::tcp::socket socket_;

        boost::weak_ptr<CmdServiceHttp> parents_;

        // The buffer for performing reads.
        boost::beast::flat_buffer buffer_{JSON_Package_Max_Size};

        // The request message.
        boost::beast::http::request<boost::beast::http::dynamic_body> request_;

        // The timer for putting a deadline on connection processing.
        boost::asio::steady_timer deadline_{
                socket_.get_executor(), std::chrono::seconds(60)};


        boost::json::parse_options json_parse_options_;
        std::unique_ptr<std::vector<unsigned char>> json_storage_;
        std::unique_ptr<boost::json::static_resource> json_storage_resource_;

    private:

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
                            BOOST_LOG_OWL(warning) << "CmdServiceHttpConnect read_request error: " << ec.what();
                            return;
                        }
                        self->process_request();
                    });
        }

        // Determine what needs to be done with the request message.
        void
        process_request();

        void create_post_response_cmd(const std::string &jsonS) {

            auto p = getParentRef();
            if (!p) {
                return;
            }

            OwlProcessJsonMessage::process_json_message(
                    jsonS,
                    *json_storage_resource_,
                    json_parse_options_,
                    shared_from_this()
            );
        }

        void process_tag_info();

        void create_post_response();

        // Construct a response message based on the program state.
        void create_get_response();

        void create_get_airplane_state();

        // Asynchronously transmit the response message.
        template<typename BodyType =boost::beast::http::dynamic_body>
        void write_response(
                boost::shared_ptr<boost::beast::http::response<BodyType>> response
        ) {
            auto self = shared_from_this();

            BOOST_LOG_OWL(trace_cmd_http) << "CmdServiceHttpConnect::write_response";
            boost::beast::http::async_write(
                    socket_,
                    *response,
                    [self, response](boost::beast::error_code ec, std::size_t) {
                        BOOST_LOG_OWL(trace_cmd_http) << "CmdServiceHttpConnect::write_response end";
                        self->socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
                        self->deadline_.cancel();
                        BOOST_LOG_OWL(trace_cmd_http) << "CmdServiceHttpConnect::write_response close";
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
                            BOOST_LOG_OWL(warning) << "CmdServiceHttpConnect check_deadline : " << ec.what();
                            return;
                        }
                        // Close socket to cancel any outstanding operation.
                        self->socket_.close(ec);
                    });
        }

    private:

        void send_back(std::string &&json_string);

        friend struct CmdServiceHttpConnectCoImpl;

        OwlMailDefine::CmdSerialMailbox getMailBoxSerial();

        OwlMailDefine::ServiceMapCalcMailbox getMailBoxMap();

    private:
        // https://en.cppreference.com/w/cpp/language/friend
        template<typename SelfType, typename SelfPtrType>
        friend void OwlProcessJsonMessage::process_json_message(
                boost::string_view,
                boost::json::static_resource &,
                boost::json::parse_options &,
                // const boost::shared_ptr<ProcessJsonMessageSelfTypeInterface> &&self
                SelfPtrType &&
        );

        template<typename SelfType, typename SelfPtrType>
        friend void OwlProcessJsonMessage::analysisJoyCon(
                SelfPtrType self,
                boost::json::object &joyCon,
                int cmdId,
                int packageId,
                int32_t clientId
        );

        template<typename SelfType, typename SelfPtrType>
        friend void OwlProcessJsonMessage::analysisJoyConSimple(
                SelfPtrType self,
                boost::json::object &joyCon,
                int cmdId,
                int packageId,
                int32_t clientId
        );

        template<typename SelfType, typename SelfPtrType>
        friend void OwlProcessJsonMessage::analysisJoyConGyro(
                SelfPtrType self,
                boost::json::object &joyCon,
                int cmdId,
                int packageId,
                int32_t clientId
        );

        void send_back_json(const boost::json::value &json_value) override {
            BOOST_LOG_OWL(trace_cmd_http) << "CmdServiceHttpConnect::send_back_json";
            send_back(boost::json::serialize(json_value));
        }

        void sendMail(OwlMailDefine::MailCmd2Serial &&data) override;

        void sendMail_map(OwlMailDefine::MailService2MapCalc &&data);

        boost::shared_ptr<CmdServiceHttp> getParentRef();

    };


    class CmdServiceHttp : public boost::enable_shared_from_this<CmdServiceHttp> {
    public:
        CmdServiceHttp(
                boost::asio::io_context &ioc,
                const boost::asio::ip::tcp::endpoint &endpoint,
                OwlMailDefine::CmdSerialMailbox &&mailbox,
                OwlMailDefine::ServiceMapCalcMailbox &&mailbox_map
        );

        ~CmdServiceHttp() {
            BOOST_LOG_OWL(trace_dtor) << "~CmdServiceHttp()";
            mailbox_->receiveB2A(nullptr);
        }

    private:
        boost::asio::io_context &ioc_;
        boost::asio::ip::tcp::acceptor acceptor_;
        OwlMailDefine::CmdSerialMailbox mailbox_;
        OwlMailDefine::ServiceMapCalcMailbox mailbox_map_;
    public:
        // Start accepting incoming connections
        void
        start() {
            do_accept();
        }

    private:
        friend CmdServiceHttpConnect;

        void receiveMail(OwlMailDefine::MailSerial2Cmd &&data) {
            // get callback from data and call it to send back image result
            data->runner(data);
        }

        // clientId-CmdPackageIdentify
        std::map<size_t, OwlMailDefine::CmdPackageIdentify> cmdPackageIdentify;

        void sendMail(OwlMailDefine::MailCmd2Serial &&data) {
            // send cmd to serial
            if (data->clientId > 0) {
                // TODO filter joystick
                auto it = cmdPackageIdentify.find(data->clientId);
                if (it == cmdPackageIdentify.end()) {
                    cmdPackageIdentify.try_emplace(
                            data->clientId,
                            data->packageId,
                            data->cmdId,
                            data->clientId
                    );
                    // a new package with new client, go it
                    mailbox_->sendA2B(std::move(data));
                    return;
                } else {
                    auto a = it->second;
                    if (a.packageId == data->packageId &&
                        a.cmdId == data->cmdId &&
                        a.clientId == data->clientId) {
                        // a old package, ignore it
                        return;
                    } else {
                        a.packageId = data->packageId;
                        a.cmdId = data->cmdId;
                        // a new package, go it
                        mailbox_->sendA2B(std::move(data));
                        return;
                    }
                }
                return;
            }
            mailbox_->sendA2B(std::move(data));
        }

    private:
        void
        do_accept() {
            // The new connection gets its own strand
            acceptor_.async_accept(
                    boost::asio::make_strand(ioc_),
                    boost::beast::bind_front_handler(
                            &CmdServiceHttp::on_accept,
                            shared_from_this()));
        }

        void
        on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket) {

            if (ec) {
                BOOST_LOG_OWL(error) << "accept" << " : " << ec.message();
            } else {
                // Create the session and run it
                boost::make_shared<CmdServiceHttpConnect>(
                        ioc_,
                        std::move(socket),
                        weak_from_this()
                )->start();
            }

            // Accept another connection
            do_accept();

        }

    };

}


#endif //OWLACCESSTERMINAL_CMDSERVICEHTTP_H
