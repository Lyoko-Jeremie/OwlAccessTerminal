// jeremie

#include "CmdServiceHttp.h"
#include "./AirplaneState.h"
#include "../MapCalc/MapCalcMail.h"
#include "../MapCalc/MapCalcPlaneInfoType.h"
#include <boost/exception/diagnostic_information.hpp>

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
        } else {
            BOOST_LOG_TRIVIAL(error) << "CmdServiceHttpConnect::sendMail() " << "(!p)";
        }
    }

    void CmdServiceHttpConnect::sendMail_map(OwlMailDefine::MailService2MapCalc &&data) {
        // send cmd to serial
        auto p = getParentRef();
        if (p) {
            p->mailbox_map_->sendA2B(std::move(data));
        } else {
            BOOST_LOG_TRIVIAL(error) << "CmdServiceHttpConnect::sendMail() " << "(!p)";
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
            if (!json_o.contains("imageX") || !json_o.at("imageX").is_int64()) {
                BOOST_LOG_TRIVIAL(warning) << "contains fail (imageCenterX)" << jsonS;
                send_back_json(
                        boost::json::value{
                                {"msg",    "error"},
                                {"error",  "(imageX) not find || !is_int64()"},
                                {"result", false},
                        }
                );
                return;
            }
            if (!json_o.contains("imageY") || !json_o.at("imageY").is_int64()) {
                BOOST_LOG_TRIVIAL(warning) << "contains fail (imageY)" << jsonS;
                send_back_json(
                        boost::json::value{
                                {"msg",    "error"},
                                {"error",  "(imageY) not find || !is_int64()"},
                                {"result", false},
                        }
                );
                return;
            }
            auto imageX = json_o.at("imageX").get_int64();
            auto imageY = json_o.at("imageY").get_int64();

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
                            b.contains("cLTx") &&
                            b.contains("cLTy") &&
                            b.contains("cRTx") &&
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
            if (json_o.contains("centerTag")) {
                if (!json_o.at("centerTag").is_object()) {
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
                auto b = json_o.at("centerTag").as_object();
                if (!(
                        b.contains("id") &&
                        b.contains("ham") &&
                        b.contains("dm") &&
                        b.contains("cX") &&
                        b.contains("cY") &&
                        b.contains("cLTx") &&
                        b.contains("cLTy") &&
                        b.contains("cRTx") &&
                        b.contains("cRTy") &&
                        b.contains("cRBx") &&
                        b.contains("cRBy") &&
                        b.contains("cLBx") &&
                        b.contains("cLBy")
                )) {
                    BOOST_LOG_TRIVIAL(warning) << "invalid centerTag items" << jsonS;
                    send_back_json(
                            boost::json::value{
                                    {"msg",    "error"},
                                    {"error",  "invalid centerTag items"},
                                    {"result", false},
                            }
                    );
                    return;
                }
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
//                BOOST_LOG_TRIVIAL(trace) << jsonS;
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
                        ss << "\naprilTagInfoList.size():" << aprilTagInfoList->size();
                        for (size_t i = 0; i < aprilTagInfoList->size(); ++i) {
                            auto a = aprilTagInfoList->at(i);
                            ss << "\n\t"
                               << " id:" << a.id
                               << " x:" << a.centerX
                               << " y:" << a.centerX
                               << " [" << a.cornerLTx
                               << " ," << a.cornerLTy
                               << " ;" << a.cornerRTx
                               << " ," << a.cornerRTy
                               << " ;" << a.cornerRBx
                               << " ," << a.cornerRBy
                               << " ;" << a.cornerLBx
                               << " ," << a.cornerLBy
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
            aprilTagCmd->imageX = imageX;
            aprilTagCmd->imageY = imageY;

            // get newestAirplaneState
            auto getASm = std::make_shared<OwlMailDefine::Cmd2Serial>();
            getASm->additionCmd = OwlMailDefine::AdditionCmd::getAirplaneState;
            BOOST_LOG_TRIVIAL(trace) << "CmdServiceHttpConnect::process_tag_info to getASm";
            getASm->callbackRunner = [
                    this, self = shared_from_this(),
                    aprilTagCmd
            ](
                    OwlMailDefine::MailSerial2Cmd data
            ) {
                BOOST_LOG_TRIVIAL(trace) << "CmdServiceHttpConnect::process_tag_info back getASm";
                BOOST_ASSERT(data);
                BOOST_ASSERT(self);
                BOOST_ASSERT(self.use_count() > 0);
                if (!data->ok) {
                    // ignore
                    send_back_json(
                            boost::json::value{
                                    {"msg",       "getAirplaneState"},
                                    {"result",    data->ok},
                                    {"openError", data->openError},
                            }
                    );
                    return;
                }
                if (!data->newestAirplaneState) {
                    // TODO
                }

                auto mc = std::make_shared<OwlMailDefine::Service2MapCalc>();
                mc->airplaneState = data->newestAirplaneState;
                mc->tagInfo = aprilTagCmd->shared_from_this();
                BOOST_LOG_TRIVIAL(trace) << "CmdServiceHttpConnect::process_tag_info to mc newestAirplaneState";
                mc->callbackRunner = [
                        this, self = shared_from_this(),
                        aprilTagCmd
                ](
                        OwlMailDefine::MailMapCalc2Service data
                ) {
                    BOOST_LOG_TRIVIAL(trace) << "CmdServiceHttpConnect::process_tag_info back mc newestAirplaneState";
                    BOOST_ASSERT(data);
                    BOOST_ASSERT(self);
                    BOOST_ASSERT(self.use_count() > 0);
                    if (!data->ok) {
                        // ignore
                        send_back_json(
                                boost::json::value{
                                        {"msg",       "MapCalc"},
                                        {"result",    data->ok},
                                        {"openError", false},
                                }
                        );
                        return;
                    }
                    BOOST_ASSERT(aprilTagCmd);
                    aprilTagCmd->mapCalcPlaneInfoType = data->info;
                    BOOST_ASSERT(aprilTagCmd->aprilTagList);
                    BOOST_ASSERT(aprilTagCmd->aprilTagCenter);

                    // TODO debug
                    send_back_json(
                            boost::json::value{
                                    {"msg",       "MapCalc"},
                                    {"result",    data->ok},
                                    {"openError", false},
                            }
                    );
                    return;

                    auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
                    m->additionCmd = OwlMailDefine::AdditionCmd::AprilTag;
                    m->aprilTagCmdPtr = aprilTagCmd;
                    BOOST_LOG_TRIVIAL(trace) << "CmdServiceHttpConnect::process_tag_info to m AprilTag";
                    m->callbackRunner = [
                            this, self = shared_from_this(),
                            aprilTagCmd
                    ](
                            OwlMailDefine::MailSerial2Cmd data
                    ) {
                        BOOST_LOG_TRIVIAL(trace) << "CmdServiceHttpConnect::process_tag_info back m AprilTag";
                        BOOST_ASSERT(aprilTagCmd);
                        BOOST_ASSERT(aprilTagCmd.use_count() > 0);
                        BOOST_LOG_TRIVIAL(trace) << "CmdServiceHttpConnect::process_tag_info aprilTagCmd.use_count() "
                                                 << aprilTagCmd.use_count();
                        BOOST_ASSERT(data);
                        BOOST_ASSERT(self);
                        BOOST_ASSERT(self.use_count() > 0);
                        send_back_json(
                                boost::json::value{
                                        {"msg",       "AprilTag"},
                                        {"result",    data->ok},
                                        {"openError", data->openError},
                                }
                        );
                        BOOST_LOG_TRIVIAL(trace)
                            << "CmdServiceHttpConnect::process_tag_info back m AprilTag send_back_json end";
                    };
                    sendMail(std::move(m));
                };
                sendMail_map(std::move(mc));
            };
            sendMail(std::move(getASm));
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
            BOOST_LOG_TRIVIAL(error) << "CmdServiceHttpConnect::process_tag_info catch (...) exception"
                                     << "\n" << boost::current_exception_diagnostic_information();
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

    void CmdServiceHttpConnect::create_get_airplane_state() {

        auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
        m->additionCmd = OwlMailDefine::AdditionCmd::getAirplaneState;
        m->callbackRunner = [this, self = shared_from_this()](
                OwlMailDefine::MailSerial2Cmd data
        ) {
            BOOST_ASSERT(self);
            BOOST_ASSERT(self.use_count() > 0);
            auto state = data->newestAirplaneState;
            auto tags = data->aprilTagCmdData;
            if (!state || !tags) {
                send_back_json(
                        boost::json::value{
                                {"msg",    "newestAirplaneState || aprilTagCmdData nullptr"},
                                {"error",  "nullptr"},
                                {"result", false},
                        }
                );
                return;
            }
            auto send_back_data = [
                    this, self = shared_from_this(), state
            ](
                    std::shared_ptr<OwlMapCalc::MapCalcPlaneInfoType> tags, bool ok
            ) {
                send_back_json(
                        boost::json::value{
                                {"result",        true},
                                {"state",
                                                  {
                                                          {"timestamp", state->timestamp},
                                                          {"stateFly", state->stateFly},
                                                          {"pitch", state->pitch},
                                                          {"roll", state->roll},
                                                          {"yaw", state->yaw},
                                                          {"vx", state->vx},
                                                          {"vy", state->vy},
                                                          {"vz", state->vz},
                                                          {"high", state->high},
                                                          {"voltage", state->voltage},
                                                  }
                                },
                                {"tag",
                                                  {
                                                          {"ok",        ok},
                                                          {"info",     OwlMapCalc::
                                                                       MapCalcPlaneInfoType2JsonObject(tags)},
                                                  }
                                },
                                {"nowTimestamp",  std::chrono::time_point_cast<std::chrono::milliseconds>(
                                        std::chrono::steady_clock::now()).time_since_epoch().count()},
                                {"nowTimestampC", std::chrono::time_point_cast<std::chrono::milliseconds>(
                                        std::chrono::system_clock::now()).time_since_epoch().count()},
                        }
                );
            };
            if (tags->mapCalcPlaneInfoType) {
                send_back_data(tags->mapCalcPlaneInfoType, true);
                return;
            }
            auto mm = std::make_shared<OwlMailDefine::Service2MapCalc>();
            mm->airplaneState = state;
            mm->tagInfo = tags;
            mm->callbackRunner = [
                    this, self = shared_from_this(),
                    state, tags,
                    data_s = data,
                    send_back_data = std::move(send_back_data)
            ](
                    OwlMailDefine::MailMapCalc2Service data_m
            ) {
                if (!data_m->ok) {
                    // TODO
                }
                send_back_data(data_m->info, data_m->ok);
                return;
            };
            sendMail_map(std::move(mm));
        };
        sendMail(std::move(m));

    }

    void CmdServiceHttpConnect::create_get_response() {

        auto response = std::make_shared<boost::beast::http::response<boost::beast::http::dynamic_body>>();
        response->version(request_.version());
        response->keep_alive(false);

        response->set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);

        if (request_.target() == "/nowTimestamp") {
            response->result(boost::beast::http::status::ok);
            response->set(boost::beast::http::field::content_type, "text/html");
            boost::beast::ostream(response->body())
                    << std::chrono::time_point_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now()).time_since_epoch().count() << "\r\n";
            response->content_length(response->body().size());
            write_response(response);
            return;
        }
        if (request_.target() == "/AirplaneState") {
            create_get_airplane_state();
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

    void CmdServiceHttpConnect::send_back(std::string &&json_string) {
        BOOST_LOG_TRIVIAL(trace) << "CmdServiceHttpConnect::send_back json_string:" << json_string;
        auto response = std::make_shared<boost::beast::http::response<boost::beast::http::dynamic_body>>();
        response->version(request_.version());
        response->keep_alive(false);

        response->set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);

        response->result(boost::beast::http::status::ok);
        response->set(boost::beast::http::field::content_type, "application/json");
        boost::beast::ostream(response->body()) << json_string;
        response->content_length(response->body().size());
        write_response(response);
        BOOST_LOG_TRIVIAL(trace) << "CmdServiceHttpConnect::send_back return";
    }

    std::shared_ptr<CmdServiceHttp> CmdServiceHttpConnect::getParentRef() {
        auto p = parents_.lock();
        if (!p) {
            BOOST_LOG_TRIVIAL(error) << "CmdServiceHttpConnect::getParentRef() " << "(!p)";
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

    CmdServiceHttp::CmdServiceHttp(boost::asio::io_context &ioc,
                                   const boost::asio::ip::tcp::endpoint &endpoint,
                                   OwlMailDefine::CmdSerialMailbox &&mailbox,
                                   OwlMailDefine::ServiceMapCalcMailbox &&mailbox_map
    ) : ioc_(ioc),
        acceptor_(boost::asio::make_strand(ioc)),
        mailbox_(std::move(mailbox)),
        mailbox_map_(mailbox_map) {

        mailbox_->receiveB2A([this](OwlMailDefine::MailSerial2Cmd &&data) {
            receiveMail(std::move(data));
        });
        mailbox_map_->receiveB2A([this](OwlMailDefine::MailMapCalc2Service &&data) {
            BOOST_LOG_TRIVIAL(trace) << "CmdServiceHttp::receiveMail mailbox_map_->receiveB2A";
            boost::asio::dispatch(ioc_, [self = shared_from_this(), data]() {
                BOOST_LOG_TRIVIAL(trace) << "CmdServiceHttp::receiveMail mailbox_map_->receiveB2A dispatch";
                data->runner(data);
            });
        });

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