// jeremie

#include "CmdServiceHttp.h"
#include "./AirplaneState.h"
#include "../MapCalc/MapCalcMail.h"
#include "../MapCalc/MapCalcPlaneInfoType.h"
#include <boost/exception/diagnostic_information.hpp>

#include <boost/asio/co_spawn.hpp>

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
            BOOST_LOG_OWL(error) << "CmdServiceHttpConnect::sendMail() " << "(!p)";
        }
    }

    void CmdServiceHttpConnect::sendMail_map(OwlMailDefine::MailService2MapCalc &&data) {
        // send cmd to serial
        auto p = getParentRef();
        if (p) {
            p->mailbox_map_->sendA2B(std::move(data));
        } else {
            BOOST_LOG_OWL(error) << "CmdServiceHttpConnect::sendMail() " << "(!p)";
        }
    }


    OwlMailDefine::CmdSerialMailbox CmdServiceHttpConnect::getMailBoxSerial() {
        auto p = getParentRef();
        if (p) {
            return p->mailbox_;
        } else {
            BOOST_LOG_OWL(error) << "CmdServiceHttpConnect::getMailBoxSerial() " << "(!p)";
            return {};
        }
    }

    OwlMailDefine::ServiceMapCalcMailbox CmdServiceHttpConnect::getMailBoxMap() {
        auto p = getParentRef();
        if (p) {
            return p->mailbox_map_;
        } else {
            BOOST_LOG_OWL(error) << "CmdServiceHttpConnect::getMailBoxMap() " << "(!p)";
            return {};
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
                auto response = boost::make_shared<boost::beast::http::response<boost::beast::http::dynamic_body>>();
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


    using boost::asio::use_awaitable;
#if defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)
# define use_awaitable \
  boost::asio::use_awaitable_t(__FILE__, __LINE__, __PRETTY_FUNCTION__)
#endif

    struct CmdServiceHttpConnectCoImpl : public boost::enable_shared_from_this<CmdServiceHttpConnectCoImpl> {
        explicit CmdServiceHttpConnectCoImpl(boost::shared_ptr<CmdServiceHttpConnect> parentPtr)
                : parentPtr_(parentPtr) {
        }

        boost::shared_ptr<OwlMailDefine::AprilTagCmd> aprilTagCmd{};
        boost::shared_ptr<OwlMailDefine::Cmd2Serial> getASm{};
        boost::shared_ptr<OwlMailDefine::Service2MapCalc> mc{};
        boost::shared_ptr<OwlMailDefine::Cmd2Serial> m{};

        boost::shared_ptr<CmdServiceHttpConnect> parentPtr_;

        boost::system::error_code ec_{};

        boost::asio::awaitable<bool> co_process_tag_info(boost::shared_ptr<CmdServiceHttpConnectCoImpl> self) {
            boost::ignore_unused(self);

            BOOST_LOG_OWL(trace_cmd_tag) << "co_process_tag_info start";

            try {
                auto executor = co_await boost::asio::this_coro::executor;

//                boost::shared_ptr<OwlMailDefine::AprilTagCmd> aprilTagCmd{};
                try {

                    std::string jsonS = boost::beast::buffers_to_string(parentPtr_->request_.body().data());
                    // resize json space
                    parentPtr_->json_storage_ =
                            std::make_unique<decltype(parentPtr_->json_storage_)::element_type>(
                                    JSON_Package_Max_Size_TagInfo, 0);
                    parentPtr_->json_storage_resource_ =
                            std::make_unique<decltype(parentPtr_->json_storage_resource_)::element_type>
                                    (parentPtr_->json_storage_->data(),
                                     parentPtr_->json_storage_->size());


                    BOOST_LOG_OWL(trace_cmd_tag) << "co_process_tag_info do [analysis tag from json request]";
                    // ================================ analysis tag from json request ================================

                    boost::system::error_code ec;
                    // auto jsv = boost::string_view{receive_buffer_.data(), bytes_transferred};
                    boost::json::value json_v = boost::json::parse(
                            jsonS,
                            ec,
                            &*parentPtr_->json_storage_resource_,
                            parentPtr_->json_parse_options_
                    );

                    if (ec) {
                        // ignore
                        // std::cerr << ec.what() << "\n";
                        BOOST_LOG_OWL(error) << "process_tag_info " << ec.what();
                        parentPtr_->send_back_json({
                                                           {"msg",    "error"},
                                                           {"error",  "boost::json::parse failed : " + ec.what()},
                                                           {"result", false},
                                                   });
                        co_return false;
                    }

                    auto json_o = json_v.as_object();
                    if (!json_o.contains("imageX") || !json_o.at("imageX").is_int64()) {
                        BOOST_LOG_OWL(warning) << "contains fail (imageCenterX)" << jsonS;
                        parentPtr_->send_back_json(
                                boost::json::value{
                                        {"msg",    "error"},
                                        {"error",  "(imageX) not find || !is_int64()"},
                                        {"result", false},
                                }
                        );
                        co_return false;
                    }
                    if (!json_o.contains("imageY") || !json_o.at("imageY").is_int64()) {
                        BOOST_LOG_OWL(warning) << "contains fail (imageY)" << jsonS;
                        parentPtr_->send_back_json(
                                boost::json::value{
                                        {"msg",    "error"},
                                        {"error",  "(imageY) not find || !is_int64()"},
                                        {"result", false},
                                }
                        );
                        co_return false;
                    }
                    auto imageX = json_o.at("imageX").get_int64();
                    auto imageY = json_o.at("imageY").get_int64();

                    if (!json_o.contains("tagList") || !json_o.at("tagList").is_array()) {
                        BOOST_LOG_OWL(warning) << "contains fail (tagList)" << jsonS;
                        parentPtr_->send_back_json(
                                boost::json::value{
                                        {"msg",    "error"},
                                        {"error",  "(tagList) not find || !is_array()"},
                                        {"result", false},
                                }
                        );
                        co_return false;
                    }
                    auto tl = json_o.at("tagList").as_array();

//            if (tl.empty() && !json_o.contains("center")) {
//                // ignore
//                parentPtr_->send_back_json(
//                        boost::json::value{
//                                {"result", true},
//                        }
//                );
//                co_return false;
//            }

                    boost::shared_ptr<OwlMailDefine::AprilTagCmd::AprilTagListType::element_type> aprilTagInfoList{};
                    if (!tl.empty()) {
                        aprilTagInfoList =
                                boost::make_shared<OwlMailDefine::AprilTagCmd::AprilTagListType::element_type>();
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
                                BOOST_LOG_OWL(warning) << "invalid tagList items" << jsonS;
                                parentPtr_->send_back_json(
                                        boost::json::value{
                                                {"msg",    "error"},
                                                {"error",  "invalid tagList items"},
                                                {"result", false},
                                        }
                                );
                                co_return false;
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
                            BOOST_LOG_OWL(warning) << "(!json_o.at(\"center\").is_object())" << jsonS;
                            parentPtr_->send_back_json(
                                    boost::json::value{
                                            {"msg",    "error"},
                                            {"error",  "(!json_o.at(\"center\").is_object())"},
                                            {"result", false},
                                    }
                            );
                            co_return false;
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
                            BOOST_LOG_OWL(warning) << "invalid centerTag items" << jsonS;
                            parentPtr_->send_back_json(
                                    boost::json::value{
                                            {"msg",    "error"},
                                            {"error",  "invalid centerTag items"},
                                            {"result", false},
                                    }
                            );
                            co_return false;
                        }
                        aprilTagInfoCenter = boost::make_shared<OwlMailDefine::AprilTagCmd::AprilTagCenterType::element_type>(
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
//                BOOST_LOG_OWL(trace) << jsonS;
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
                        BOOST_LOG_OWL(trace) << s;
                    }

                    if (!aprilTagInfoList && !aprilTagInfoCenter) {
                        // ignore
                        BOOST_LOG_OWL(trace_cmd_tag)
                            << "co_process_tag_info (!aprilTagInfoList && !aprilTagInfoCenter)";
                        parentPtr_->send_back_json(
                                boost::json::value{
                                        {"result", true},
                                }
                        );
                        co_return false;
                    }


                    BOOST_LOG_OWL(trace_cmd_tag) << "co_process_tag_info do [prepare aprilTagCmd]";
                    // ================================ prepare aprilTagCmd ================================
                    aprilTagCmd = boost::make_shared<OwlMailDefine::AprilTagCmd>();
                    aprilTagCmd->aprilTagList = aprilTagInfoList;
                    aprilTagCmd->aprilTagCenter = aprilTagInfoCenter;
                    aprilTagCmd->imageX = imageX;
                    aprilTagCmd->imageY = imageY;

                } catch (const std::exception &e) {
                    BOOST_LOG_OWL(error) << "co_process_tag_info json catch (const std::exception &e)" << e.what();
                    co_return false;
                }
                BOOST_ASSERT(self);
                BOOST_ASSERT(parentPtr_);

                BOOST_LOG_OWL(trace_cmd_tag) << "co_process_tag_info do [to get newestAirplaneState]";
                // ================================ to get newestAirplaneState ================================
                // get newestAirplaneState
//                auto getASm = boost::make_shared<OwlMailDefine::Cmd2Serial>();
                getASm = boost::make_shared<OwlMailDefine::Cmd2Serial>();
                getASm->additionCmd = OwlMailDefine::AdditionCmd::getAirplaneState;
                BOOST_LOG_OWL(trace_cmd_tag) << "co_process_tag_info to getASm";
                boost::shared_ptr<OwlSerialController::AirplaneState> newestAirplaneState{};
                {
                    auto box = parentPtr_->getMailBoxSerial();
                    if (box) {
                        auto data = co_await asyncSendMail2B(
                                box->shared_from_this(), getASm,
                                boost::asio::bind_executor(
                                        executor,
                                        boost::asio::redirect_error(use_awaitable, ec_)));
                        BOOST_ASSERT(data);
                        BOOST_ASSERT(self);
                        BOOST_ASSERT(parentPtr_);
                        co_await boost::asio::dispatch(executor, use_awaitable);
                        BOOST_LOG_OWL(trace_cmd_tag) << "co_process_tag_info back getASm";
                        BOOST_ASSERT(data);
                        BOOST_ASSERT(self);
                        BOOST_ASSERT(parentPtr_);
                        if (ec_) {
                            BOOST_LOG_OWL(error) << "co_process_tag_info asyncSendMail2B MailBoxSerial error : "
                                                 << ec_.what();
                            co_return false;
                        }

                        if (!data->ok) {
                            BOOST_LOG_OWL(trace_cmd_tag) << "co_process_tag_info getAirplaneState (!data->ok)";
                            // ignore
                            parentPtr_->send_back_json(
                                    boost::json::value{
                                            {"msg",       "getAirplaneState"},
                                            {"result",    data->ok},
                                            {"openError", data->openError},
                                    }
                            );
                            co_return false;
                        }
                        if (!data->newestAirplaneState) {
                            // TODO
                        }
                        newestAirplaneState = data->newestAirplaneState;
                    } else {
                        BOOST_LOG_OWL(error) << "co_process_tag_info !parentPtr_->getMailBoxSerial()";
                        co_return false;
                    }
                }
                BOOST_LOG_OWL(trace_cmd_tag) << "co_process_tag_info do [to calc map]";
                // ================================ to calc map ================================
//                auto mc = boost::make_shared<OwlMailDefine::Service2MapCalc>();
                mc = boost::make_shared<OwlMailDefine::Service2MapCalc>();
                mc->airplaneState = newestAirplaneState;
                mc->tagInfo = aprilTagCmd->shared_from_this();
                BOOST_LOG_OWL(trace_cmd_tag) << "co_process_tag_info to mc newestAirplaneState";
                {
                    auto box = parentPtr_->getMailBoxMap();
                    if (box) {
                        auto data = co_await asyncSendMail2B(
                                box->shared_from_this(), mc,
                                boost::asio::bind_executor(
                                        executor,
                                        boost::asio::redirect_error(use_awaitable, ec_)));
                        BOOST_ASSERT(data);
                        BOOST_ASSERT(self);
                        BOOST_ASSERT(parentPtr_);
                        co_await boost::asio::dispatch(executor, use_awaitable);
                        BOOST_LOG_OWL(trace_cmd_tag) << "co_process_tag_info back mc newestAirplaneState";
                        BOOST_ASSERT(data);
                        BOOST_ASSERT(self);
                        BOOST_ASSERT(parentPtr_);
                        if (ec_) {
                            BOOST_LOG_OWL(error) << "co_process_tag_info asyncSendMail2B MailBoxMap error : "
                                                 << ec_.what();
                            co_return false;
                        }

                        if (!data->ok) {
                            BOOST_LOG_OWL(trace_cmd_tag) << "co_process_tag_info Service2MapCalc (!data->ok)";
                            // ignore
                            parentPtr_->send_back_json(
                                    boost::json::value{
                                            {"msg",       "MapCalc"},
                                            {"result",    data->ok},
                                            {"openError", false},
                                    }
                            );
                            co_return false;
                        }
                        BOOST_ASSERT(aprilTagCmd);
                        aprilTagCmd->mapCalcPlaneInfoType = data->info;
                        BOOST_ASSERT(aprilTagCmd->aprilTagList);
                        BOOST_ASSERT(aprilTagCmd->aprilTagCenter);

                    } else {
                        BOOST_LOG_OWL(error) << "co_process_tag_info !parentPtr_->getMailBoxMap()";
                        co_return false;
                    }
                }
                BOOST_LOG_OWL(trace_cmd_tag) << "co_process_tag_info do [send tag to serial]";
                // ================================ send tag to serial ================================
//                auto m = boost::make_shared<OwlMailDefine::Cmd2Serial>();
                m = boost::make_shared<OwlMailDefine::Cmd2Serial>();
                m->additionCmd = OwlMailDefine::AdditionCmd::AprilTag;
                m->aprilTagCmdPtr = aprilTagCmd;
                BOOST_LOG_OWL(trace_cmd_tag) << "co_process_tag_info to m AprilTag";
                {
                    auto box = parentPtr_->getMailBoxSerial();
                    if (box) {
                        auto data = co_await asyncSendMail2B(
                                box->shared_from_this(), m,
                                boost::asio::bind_executor(
                                        executor,
                                        boost::asio::redirect_error(use_awaitable, ec_)));
                        BOOST_LOG_OWL(trace_cmd_tag) << "co_process_tag_info back m AprilTag";
                        BOOST_ASSERT(data);
                        BOOST_ASSERT(self);
                        BOOST_ASSERT(parentPtr_);
                        co_await boost::asio::dispatch(executor, use_awaitable);
                        BOOST_LOG_OWL(trace_cmd_tag) << "co_process_tag_info back m AprilTag dispatch";
                        BOOST_ASSERT(data);
                        BOOST_ASSERT(self);
                        BOOST_ASSERT(parentPtr_);
                        if (ec_) {
                            BOOST_LOG_OWL(error) << "co_process_tag_info asyncSendMail2B MailBoxSerial error : "
                                                 << ec_.what();
                            co_return false;
                        }

                        if (!data->ok) {
                            BOOST_LOG_OWL(trace_cmd_tag) << "co_process_tag_info AprilTag (!data->ok)";
                            // ignore
                            parentPtr_->send_back_json(
                                    boost::json::value{
                                            {"msg",       "getAirplaneState"},
                                            {"result",    data->ok},
                                            {"openError", data->openError},
                                    }
                            );
                            co_return false;
                        }

                        BOOST_LOG_OWL(trace_cmd_tag) << "co_process_tag_info do [tag process end]";
                        // ================================ tag process end ================================
                        parentPtr_->send_back_json(
                                boost::json::value{
                                        {"msg",       "AprilTag"},
                                        {"result",    data->ok},
                                        {"openError", data->openError},
                                }
                        );
                        BOOST_LOG_OWL(trace_cmd_tag) << "co_process_tag_info back m AprilTag send_back_json end";
                    } else {
                        BOOST_LOG_OWL(error) << "co_process_tag_info !parentPtr_->getMailBoxSerial()";
                        co_return false;
                    }
                }
                // ================================ ............ ================================
                // ================================ ............ ================================
                // ================================ ............ ================================
                // ================================ ............ ================================
                // ================================ ............ ================================


            } catch (const std::exception &e) {
                BOOST_LOG_OWL(error) << "co_process_tag_info catch (const std::exception &e)" << e.what();
                throw;
//                co_return false;
            }

            boost::ignore_unused(self);
            co_return true;
        }

        void process_tag_info() {
            auto u = boost::urls::parse_uri_reference(parentPtr_->request_.target());
            if (u.has_error()) {
                auto response = boost::make_shared<boost::beast::http::response<boost::beast::http::dynamic_body>>();
                response->version(parentPtr_->request_.version());
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
                parentPtr_->write_response(response);
                return;
            }

            boost::asio::co_spawn(
                    parentPtr_->ioc_,
                    [this, self = shared_from_this()]() {
                        return co_process_tag_info(self);
                    },
                    [this, self = shared_from_this()](std::exception_ptr e, bool r) {

                        if (!e) {
                            if (r) {
                                BOOST_LOG_OWL(trace_cmd_tag) << "CmdServiceHttpConnectCoImpl run() ok";
                                return;
                            } else {
                                BOOST_LOG_OWL(trace_cmd_tag) << "CmdServiceHttpConnectCoImpl run() error";
                                return;
                            }
                        } else {
                            std::string what;
                            // https://stackoverflow.com/questions/14232814/how-do-i-make-a-call-to-what-on-stdexception-ptr
                            try { std::rethrow_exception(std::move(e)); }
                            catch (const std::exception &e) {
                                BOOST_LOG_OWL(error) << "CmdServiceHttpConnectCoImpl co_spawn catch std::exception "
                                                     << e.what();
                                what = e.what();
                            }
                            catch (const std::string &e) {
                                BOOST_LOG_OWL(error) << "CmdServiceHttpConnectCoImpl co_spawn catch std::string " << e;
                                what = e;
                            }
                            catch (const char *e) {
                                BOOST_LOG_OWL(error) << "CmdServiceHttpConnectCoImpl co_spawn catch char *e " << e;
                                what = std::string{e};
                            }
                            catch (...) {
                                BOOST_LOG_OWL(error) << "CmdServiceHttpConnectCoImpl co_spawn catch (...)"
                                                     << "\n" << boost::current_exception_diagnostic_information();
                                what = boost::current_exception_diagnostic_information();
                            }
                            BOOST_LOG_OWL(error) << "CmdServiceHttpConnectCoImpl::process_tag_info " << what;
                            auto response = boost::make_shared<boost::beast::http::response<boost::beast::http::dynamic_body>>();
                            response->version(parentPtr_->request_.version());
                            response->keep_alive(false);
                            response->set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
                            response->result(boost::beast::http::status::bad_request);
                            response->set(boost::beast::http::field::content_type, "text/plain");
                            boost::beast::ostream(response->body())
                                    << "CmdServiceHttpConnectCoImpl::process_tag_info "
                                    << " " << what
                                    << "\r\n";
                            response->content_length(response->body().size());
                            parentPtr_->write_response(response);
                        }

                    });

        }
    };


    void CmdServiceHttpConnect::process_tag_info() {

        {
            BOOST_LOG_OWL(trace_cmd_tag) << "CmdServiceHttpConnect init CmdServiceHttpConnectCoImpl";
            auto c = boost::make_shared<CmdServiceHttpConnectCoImpl>(shared_from_this());

            BOOST_LOG_OWL(trace_cmd_tag) << "CmdServiceHttpConnect init CmdServiceHttpConnectCoImpl process_tag_info()";
            c->process_tag_info();
            BOOST_LOG_OWL(trace_cmd_tag) << "CmdServiceHttpConnect end CmdServiceHttpConnectCoImpl process_tag_info()";
        }

        return;

    }

    void CmdServiceHttpConnect::create_post_response() {
//        BOOST_LOG_OWL(trace) << "create_post_response";

        if (request_.body().size() > 0) {

            if (request_.target().starts_with("/cmd")) {

                auto qp = OwlQueryPairsAnalyser::QueryPairsAnalyser{request_.target()};

                std::string jsonS = boost::beast::buffers_to_string(request_.body().data());
                // this will call process_json_message->sendMail->send_back_json->send_back
                create_post_response_cmd(jsonS);

                return;
            }

            if (request_.target().starts_with("/tagInfo")) {
                BOOST_LOG_OWL(trace_cmd_tag) << "create_post_response request_.target().starts_with(/tagInfo)";
                process_tag_info();
                return;
            }

        }
        BOOST_LOG_OWL(error) << "create_post_response invalid post request";

        auto response = boost::make_shared<boost::beast::http::response<boost::beast::http::dynamic_body>>();
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

        auto m = boost::make_shared<OwlMailDefine::Cmd2Serial>();
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
                    boost::shared_ptr<OwlMapCalc::MapCalcPlaneInfoType> tags, bool ok
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
            auto mm = boost::make_shared<OwlMailDefine::Service2MapCalc>();
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

        auto response = boost::make_shared<boost::beast::http::response<boost::beast::http::dynamic_body>>();
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
        BOOST_LOG_OWL(trace_cmd_http) << "CmdServiceHttpConnect::send_back json_string:" << json_string;
        auto response = boost::make_shared<boost::beast::http::response<boost::beast::http::dynamic_body>>();
        response->version(request_.version());
        response->keep_alive(false);

        response->set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);

        response->result(boost::beast::http::status::ok);
        response->set(boost::beast::http::field::content_type, "application/json");
        boost::beast::ostream(response->body()) << json_string;
        response->content_length(response->body().size());
        write_response(response);
        BOOST_LOG_OWL(trace_cmd_http) << "CmdServiceHttpConnect::send_back return";
    }

    boost::shared_ptr<CmdServiceHttp> CmdServiceHttpConnect::getParentRef() {
        auto p = parents_.lock();
        if (!p) {
            BOOST_LOG_OWL(error) << "CmdServiceHttpConnect::getParentRef() " << "(!p)";
            // inner error
            auto response = boost::make_shared<boost::beast::http::response<boost::beast::http::dynamic_body>>();
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
            BOOST_LOG_OWL(trace_cmd_http) << "CmdServiceHttp::receiveMail mailbox_map_->receiveB2A";
            boost::asio::dispatch(ioc_, [self = shared_from_this(), data]() {
                BOOST_LOG_OWL(trace_cmd_http) << "CmdServiceHttp::receiveMail mailbox_map_->receiveB2A dispatch";
                data->runner(data);
            });
        });

        boost::beast::error_code ec;

        // Open the acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if (ec) {
            BOOST_LOG_OWL(error) << "open" << " : " << ec.message();
            return;
        }

        // Allow address reuse
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
        if (ec) {
            BOOST_LOG_OWL(error) << "set_option" << " : " << ec.message();
            return;
        }

        // Bind to the server address
        acceptor_.bind(endpoint, ec);
        if (ec) {
            BOOST_LOG_OWL(error) << "bind" << " : " << ec.message();
            return;
        }

        // Start listening for connections
        acceptor_.listen(
                boost::asio::socket_base::max_listen_connections, ec);
        if (ec) {
            BOOST_LOG_OWL(error) << "listen" << " : " << ec.message();
            return;
        }
    }

}