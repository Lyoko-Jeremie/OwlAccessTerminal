// jeremie

#include "CmdServiceHttp.h"

namespace OwlCommandServiceHttp {

#ifdef DEBUG_TAG_INFO
    constexpr bool SHOW_DEBUG_TAG_INFO = true;
#else
    constexpr bool SHOW_DEBUG_TAG_INFO = false;
#endif // DEBUG_TAG_INFO

    void CmdServiceHttpConnect::sendMail(OwlMailDefine::MailCmd2Serial &&data) {
        // send cmd to serial
        auto p = getParentRef();
        if (p) {
            p->sendMail(std::move(data));
        }
    }

    void CmdServiceHttpConnect::process_request() {

        switch (request_.method()) {
            case boost::beast::http::verb::get:
                create_get_response();
                break;

            case boost::beast::http::verb::post:
                create_post_response();
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

    void CmdServiceHttpConnect::process_tag_info() {
//        BOOST_LOG_TRIVIAL(trace) << "process_tag_info";
//        BOOST_LOG_TRIVIAL(trace) << request_.target();
        auto u = boost::urls::parse_uri_reference(request_.target());
        if (u.has_error()) {
            auto response = std::make_shared<boost::beast::http::response<boost::beast::http::dynamic_body>>();
            response->version(request_.version());
            response->keep_alive(false);
            response->set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
            response->result(boost::beast::http::status::bad_request);
            response->set(boost::beast::http::field::content_type, "text/plain");
            boost::beast::ostream(response->body())
                    << "invalid post request"
                    << " " << u.error()
                    << " " << u.error().what()
                    << "\r\n";
            response->content_length(response->body().size());
            write_response(response);
            return;
        }
        // auto q = u.value().params();
        // q.find("");

        std::string jsonS = boost::beast::buffers_to_string(request_.body().data());
        // BOOST_LOG_TRIVIAL(trace) << jsonS;

        // resize json space
        json_storage_ = std::make_unique<decltype(json_storage_)::element_type>(JSON_Package_Max_Size_TagInfo, 0);
        json_storage_resource_ = std::make_unique<decltype(json_storage_resource_)::element_type>
                (json_storage_->data(),
                 json_storage_->size());

        try {

            boost::system::error_code ec;
            // auto jsv = boost::string_view{receive_buffer_.data(), bytes_transferred};
            boost::json::value json_v = boost::json::parse(
                    jsonS,
                    ec,
                    &*json_storage_resource_,
                    json_parse_options_
            );

            if (ec) {
                // ignore
                // std::cerr << ec.what() << "\n";
                BOOST_LOG_TRIVIAL(error) << "process_tag_info " << ec.what();
                send_back_json({
                                       {"msg",    "error"},
                                       {"error",  "boost::json::parse failed : " + ec.what()},
                                       {"result", false},
                               });
                return;
            }

            auto json_o = json_v.as_object();
            if (!json_o.contains("tagList") || !json_o.at("tagList").is_array()) {
                BOOST_LOG_TRIVIAL(warning) << "contains fail (tagList)" << jsonS;
                send_back_json(
                        boost::json::value{
                                {"msg",    "error"},
                                {"error",  "(tagList) not find || !is_array()"},
                                {"result", false},
                        }
                );
                return;
            }
            auto tl = json_o.at("tagList").as_array();

//            if (tl.empty() && !json_o.contains("center")) {
//                // ignore
//                send_back_json(
//                        boost::json::value{
//                                {"result", true},
//                        }
//                );
//                return;
//            }

            std::shared_ptr<OwlMailDefine::AprilTagCmd::AprilTagListType::element_type> aprilTagInfoList{};
            if (!tl.empty()) {
                aprilTagInfoList =
                        std::make_shared<OwlMailDefine::AprilTagCmd::AprilTagListType::element_type>();
                aprilTagInfoList->reserve(tl.size());

                for (const auto &a: tl) {
                    auto b = a.as_object();
                    if (!(
                            b.contains("id") &&
                            b.contains("ham") &&
                            b.contains("dm") &&
                            b.contains("cX") &&
                            b.contains("cY") &&
                            b.contains("cRTy") &&
                            b.contains("cRBx") &&
                            b.contains("cRBy") &&
                            b.contains("cLBx") &&
                            b.contains("cLBy")
                    )) {
                        BOOST_LOG_TRIVIAL(warning) << "invalid tagList items" << jsonS;
                        send_back_json(
                                boost::json::value{
                                        {"msg",    "error"},
                                        {"error",  "invalid tagList items"},
                                        {"result", false},
                                }
                        );
                        return;
                    }

                    aprilTagInfoList->emplace_back(
                            OwlMailDefine::AprilTagInfo{
                                    .id=              static_cast<int>(b.at("id").get_int64()),
                                    .hamming=         static_cast<int>(b.at("ham").get_int64()),
                                    .decision_margin= static_cast<float>(b.at("dm").get_double()),
                                    .centerX=         b.at("cX").get_double(),
                                    .centerY=         b.at("cY").get_double(),
                                    .cornerLTx=       b.at("cLTx").get_double(),
                                    .cornerLTy=       b.at("cLTy").get_double(),
                                    .cornerRTx=       b.at("cRTx").get_double(),
                                    .cornerRTy=       b.at("cRTy").get_double(),
                                    .cornerRBx=       b.at("cRBx").get_double(),
                                    .cornerRBy=       b.at("cRBy").get_double(),
                                    .cornerLBx=       b.at("cLBx").get_double(),
                                    .cornerLBy=       b.at("cLBy").get_double(),
                            }
                    );
                }
            }

            OwlMailDefine::AprilTagCmd::AprilTagCenterType aprilTagInfoCenter{};
            if (json_o.contains("center")) {
                if (!json_o.at("center").is_object()) {
                    BOOST_LOG_TRIVIAL(warning) << "(!json_o.at(\"center\").is_object())" << jsonS;
                    send_back_json(
                            boost::json::value{
                                    {"msg",    "error"},
                                    {"error",  "(!json_o.at(\"center\").is_object())"},
                                    {"result", false},
                            }
                    );
                    return;
                }
                auto b = json_o.at("center").as_object();
                aprilTagInfoCenter = std::make_shared<OwlMailDefine::AprilTagCmd::AprilTagCenterType::element_type>(
                        OwlMailDefine::AprilTagInfo{
                                .id=              static_cast<int>(b.at("id").get_int64()),
                                .hamming=         static_cast<int>(b.at("ham").get_int64()),
                                .decision_margin= static_cast<float>(b.at("dm").get_double()),
                                .centerX=         b.at("cX").get_double(),
                                .centerY=         b.at("cY").get_double(),
                                .cornerLTx=       b.at("cLTx").get_double(),
                                .cornerLTy=       b.at("cLTy").get_double(),
                                .cornerRTx=       b.at("cRTx").get_double(),
                                .cornerRTy=       b.at("cRTy").get_double(),
                                .cornerRBx=       b.at("cRBx").get_double(),
                                .cornerRBy=       b.at("cRBy").get_double(),
                                .cornerLBx=       b.at("cLBx").get_double(),
                                .cornerLBy=       b.at("cLBy").get_double(),
                        }
                );
            }

            if constexpr (SHOW_DEBUG_TAG_INFO) {
                std::stringstream ss;
                ss << "DEBUG_TAG_INFO ";
                if (!aprilTagInfoCenter) {
                    ss << " NO_CENTER";
                } else {
                    ss << "\ncenter:"
                       << " id:" << aprilTagInfoCenter->id
                       << " x:" << aprilTagInfoCenter->centerX
                       << " y:" << aprilTagInfoCenter->centerX
                       << " [" << aprilTagInfoCenter->cornerLTx
                       << " ," << aprilTagInfoCenter->cornerLTy
                       << " ;" << aprilTagInfoCenter->cornerRTx
                       << " ," << aprilTagInfoCenter->cornerRTy
                       << " ;" << aprilTagInfoCenter->cornerRBx
                       << " ," << aprilTagInfoCenter->cornerRBy
                       << " ;" << aprilTagInfoCenter->cornerLBx
                       << " ," << aprilTagInfoCenter->cornerLBy
                       << " ]";
                }
                if (!aprilTagInfoList) {
                    if (aprilTagInfoCenter) {
                        ss << "\nNO_LIST";
                    } else {
                        ss << " NO_LIST";
                    }
                } else {
                    if (aprilTagInfoList->empty()) {
                        ss << " EMPTY_LIST";
                    } else {
                        ss << "\ncenter:" << aprilTagInfoList->size();
                        for (size_t i = 0; i <= aprilTagInfoList->size(); ++i) {
                            ss << "\n\t"
                               << " id:" << aprilTagInfoCenter->id
                               << " x:" << aprilTagInfoCenter->centerX
                               << " y:" << aprilTagInfoCenter->centerX
                               << " [" << aprilTagInfoCenter->cornerLTx
                               << " ," << aprilTagInfoCenter->cornerLTy
                               << " ;" << aprilTagInfoCenter->cornerRTx
                               << " ," << aprilTagInfoCenter->cornerRTy
                               << " ;" << aprilTagInfoCenter->cornerRBx
                               << " ," << aprilTagInfoCenter->cornerRBy
                               << " ;" << aprilTagInfoCenter->cornerLBx
                               << " ," << aprilTagInfoCenter->cornerLBy
                               << " ]";
                        }
                    }
                }
                auto s = ss.str();
                BOOST_LOG_TRIVIAL(trace) << s;
            }

            if (!aprilTagInfoList && !aprilTagInfoCenter) {
                // ignore
                send_back_json(
                        boost::json::value{
                                {"result", true},
                        }
                );
                return;
            }

            auto aprilTagCmd = std::make_shared<OwlMailDefine::AprilTagCmd>();
            aprilTagCmd->aprilTagList = aprilTagInfoList;
            aprilTagCmd->aprilTagCenter = aprilTagInfoCenter;

            auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
            m->additionCmd = OwlMailDefine::AdditionCmd::AprilTag;
            m->aprilTagCmdPtr = aprilTagCmd;
            m->callbackRunner = [this, self = shared_from_this()](
                    OwlMailDefine::MailSerial2Cmd data
            ) {
                send_back_json(
                        boost::json::value{
                                {"msg",       "AprilTag"},
                                {"result",    data->ok},
                                {"openError", data->openError},
                        }
                );
            };
            sendMail(std::move(m));
            return;

        } catch (std::exception &e) {
            BOOST_LOG_TRIVIAL(error) << "CmdServiceHttpConnect::process_tag_info " << e.what();
            // ignore
//                send_back_json(
//                        boost::json::value{
//                                {"msg",    "exception"},
//                                {"error",  e.what()},
//                                {"result", false},
//                        }
//                );
            auto response = std::make_shared<boost::beast::http::response<boost::beast::http::dynamic_body>>();
            response->version(request_.version());
            response->keep_alive(false);
            response->set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
            response->result(boost::beast::http::status::bad_request);
            response->set(boost::beast::http::field::content_type, "text/plain");
            boost::beast::ostream(response->body())
                    << "CmdServiceHttpConnect::process_tag_info "
                    << " " << e.what()
                    << "\r\n";
            response->content_length(response->body().size());
            write_response(response);
            return;
        } catch (...) {
            // ignore
            send_back_json(
                    boost::json::value{
                            {"msg",    "exception"},
                            {"error",  "(...)"},
                            {"result", false},
                    }
            );
            return;
        }
    }

    void CmdServiceHttpConnect::create_post_response() {
//        BOOST_LOG_TRIVIAL(trace) << "create_post_response";

        if (request_.body().size() > 0) {

            if (request_.target().starts_with("/cmd")) {

                auto qp = OwlQueryPairsAnalyser::QueryPairsAnalyser{request_.target()};

                std::string jsonS = boost::beast::buffers_to_string(request_.body().data());
                // this will call process_json_message->sendMail->send_back_json->send_back
                create_post_response_cmd(jsonS);

                return;
            }

            if (request_.target().starts_with("/tagInfo")) {
                process_tag_info();
                return;
            }

        }
        BOOST_LOG_TRIVIAL(error) << "create_post_response invalid post request";

        auto response = std::make_shared<boost::beast::http::response<boost::beast::http::dynamic_body>>();
        response->version(request_.version());
        response->keep_alive(false);

        response->set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);

        response->result(boost::beast::http::status::bad_request);
        response->set(boost::beast::http::field::content_type, "text/plain");
        boost::beast::ostream(response->body()) << "invalid post request\r\n";
        response->content_length(response->body().size());
        write_response(response);
        return;


    }

    void CmdServiceHttpConnect::create_get_response() {

        auto response = std::make_shared<boost::beast::http::response<boost::beast::http::dynamic_body>>();
        response->version(request_.version());
        response->keep_alive(false);

        response->set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);

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

    void CmdServiceHttpConnect::send_back(std::string &&json_string) {
        auto response = std::make_shared<boost::beast::http::response<boost::beast::http::dynamic_body>>();
        response->version(request_.version());
        response->keep_alive(false);

        response->set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);

        response->result(boost::beast::http::status::ok);
        response->set(boost::beast::http::field::content_type, "application/json");
        boost::beast::ostream(response->body()) << json_string;
        response->content_length(response->body().size());
        write_response(response);
    }

    std::shared_ptr<CmdServiceHttp> CmdServiceHttpConnect::getParentRef() {
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
            return nullptr;
        }
        return p;
    }

    CmdServiceHttp::CmdServiceHttp(boost::asio::io_context &ioc, const boost::asio::ip::tcp::endpoint &endpoint,
                                   OwlMailDefine::CmdSerialMailbox &&mailbox) : ioc_(ioc),
                                                                                acceptor_(
                                                                                        boost::asio::make_strand(ioc)),
                                                                                mailbox_(std::move(mailbox)) {

        mailbox_->receiveB2A = [this](OwlMailDefine::MailSerial2Cmd &&data) {
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
}