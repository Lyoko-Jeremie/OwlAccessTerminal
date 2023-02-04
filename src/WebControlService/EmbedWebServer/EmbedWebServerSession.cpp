// jeremie

#include "EmbedWebServerSession.h"
#include "EmbedWebServerTools.h"
#include "../../QueryPairsAnalyser/QueryPairsAnalyser.h"

#include <filesystem>
#include <boost/algorithm/string.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/beast/version.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/url.hpp>
#include <utility>

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
        boost::ignore_unused(self_.res_);

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
        // stream_.expires_after(std::chrono::seconds(30));
        stream_.expires_after(std::chrono::seconds(120));

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

        if (req_.method() == boost::beast::http::verb::get ||
            req_.method() == boost::beast::http::verb::post) {
            //
            if (std::string{req_.target()}.starts_with(R"(/cmd/)")) {
                return on_cmd();
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


    // ==============================================================================================================

    /**
     * the hole class are thead safe
     */
    struct EmbedWebServerSessionHelperCmd {
        struct CmdLookupTableVType {
            OwlMailDefine::WifiCmd cmd;

            using ConfigFunctionType = std::function<
                    bool(
                            OwlMailDefine::MailWeb2Cmd &data,
                            const OwlQueryPairsAnalyser::QueryPairsAnalyser::QueryPairsType &queryPairs,
                            EmbedWebServerSession &ref
                    )>;

            std::vector<std::string> paramsCheckList{};

            ConfigFunctionType configFunction = nullptr;
        };

        using CmdLookupTableType = std::map<std::string, CmdLookupTableVType>;

        CmdLookupTableType::const_iterator operator()(const std::string &s) const {
            return CmdLookupTable.find(s);
        }

        CmdLookupTableType::const_iterator operator()() const {
            return CmdLookupTable.cend();
        }

        [[nodiscard]] CmdLookupTableType::const_iterator cend() const {
            return CmdLookupTable.cend();
        }

        static bool checkParamCheckListHelper(
                const CmdLookupTableVType &cmdLookupTableV,
                const OwlQueryPairsAnalyser::QueryPairsAnalyser::QueryPairsType &queryPairs,
                EmbedWebServerSession &ref
        ) {
            if (cmdLookupTableV.paramsCheckList.empty()) {
                return true;
            }
            if (queryPairs.empty()) {
                ref.send_EmptyPairs_error();
                return false;
            }
            // check every params IN the queryPairs AND all not empty
            auto n = std::find_if(
                    cmdLookupTableV.paramsCheckList.begin(),
                    cmdLookupTableV.paramsCheckList.end(),
                    [&](const auto &item) {
                        return !(queryPairs.count(item) == 1 && !queryPairs.find(item)->second.empty());
                    });
            if (n != cmdLookupTableV.paramsCheckList.end()) {
                ref.bad_request(R"((queryPairs.count(")" + *n + R"(") != 1))");
                return false;
            }
            return true;
        };

        // get return type of `EmbedWebServerSession::parentRef_.lock()`
        //      use `decltype(std::declval<decltype(EmbedWebServerSession::parentRef_)>().lock())`
        //      from https://www.appsloveworld.com/cplus/100/12/get-return-type-of-member-function-without-an-object
        static bool processCmd(const std::shared_ptr<EmbedWebServerSession> &ref,
                               decltype(std::declval<decltype(EmbedWebServerSession::parentRef_)>().lock()) &p,
                               const EmbedWebServerSessionHelperCmd &helperCmdLookupTable) {

            auto queryPairs = std::move(
                    OwlQueryPairsAnalyser::QueryPairsAnalyser{ref->req_.target()}.queryPairs);

            auto url = boost::urls::parse_uri_reference(ref->req_.target()).value();
            // url.params();

            auto t = helperCmdLookupTable(url.path());
            if (t != helperCmdLookupTable.cend()) {
                OwlMailDefine::MailWeb2Cmd data = std::make_shared<OwlMailDefine::Web2Cmd>();
                data->cmd = t->second.cmd;
                if (!t->second.paramsCheckList.empty()) {
                    if (!checkParamCheckListHelper(t->second, queryPairs, *ref)) {
                        // need call `ref.bad_request("");` in checkParamCheckListHelper if failed.
                        // fail, ignore
                        return false;
                    }
                }
                if (t->second.configFunction) {
                    if (!t->second.configFunction(data, queryPairs, *ref)) {
                        // need call `ref.bad_request("");` in configFunction if failed.
                        // fail, ignore
                        return false;
                    }
                }
                data->callbackRunner = [ref](
                        const std::shared_ptr<OwlMailDefine::Cmd2Web> &data_r
                ) {
                    if (!data_r->ok) {
                        return ref->send_json(
                                boost::json::value{
                                        {"result",      false},
                                        {"msg",         "error"},
                                        {"error",       "(!data_r->ok)"},
                                        {"result_code", data_r->result},
                                        {"s_err",       data_r->s_err},
                                        {"s_out",       data_r->s_out},
                                }
                        );
                    }
                    return ref->send_json(
                            boost::json::value{
                                    {"result",      true},
                                    {"result_code", data_r->result},
                                    {"s_err",       data_r->s_err},
                                    {"s_out",       data_r->s_out},
                            }
                    );
                };
                p->sendMail(std::move(data));
                return true;
            }
            return false;
        }

        const CmdLookupTableType CmdLookupTable{
                {R"(/cmd/listWlanDevice)",      {OwlMailDefine::WifiCmd::listWlanDevice}},
                {R"(/cmd/enable)",              {OwlMailDefine::WifiCmd::enable}},
                {R"(/cmd/scan)",                {OwlMailDefine::WifiCmd::scan}},
                {R"(/cmd/showHotspotPassword)", {OwlMailDefine::WifiCmd::showHotspotPassword,
                                                        {"DEVICE_NAME",},
                                                        [](
                                                                OwlMailDefine::MailWeb2Cmd &data,
                                                                const OwlQueryPairsAnalyser::QueryPairsAnalyser::QueryPairsType &queryPairs,
                                                                EmbedWebServerSession &ref
                                                        ) -> bool {
                                                            boost::ignore_unused(ref);
                                                            data->DEVICE_NAME = queryPairs.find("DEVICE_NAME")->second;
                                                            return true;
                                                        }}},
                {R"(/cmd/ap)",                  {OwlMailDefine::WifiCmd::ap,
                                                        {
                                                         "apEnable", "SSID", "PASSWORD",
                                                        },
                                                        [](
                                                                OwlMailDefine::MailWeb2Cmd &data,
                                                                const OwlQueryPairsAnalyser::QueryPairsAnalyser::QueryPairsType &queryPairs,
                                                                EmbedWebServerSession &ref
                                                        ) -> bool {
                                                            boost::ignore_unused(ref);
                                                            data->enableAp = queryPairs.find("apEnable")->second == "1";
                                                            data->SSID = queryPairs.find("SSID")->second;
                                                            data->PASSWORD = queryPairs.find("PASSWORD")->second;
                                                            return true;
                                                        }}},
                {R"(/cmd/connect)",             {OwlMailDefine::WifiCmd::connect,
                                                        {"SSID",     "PASSWORD",},
                                                        [](
                                                                OwlMailDefine::MailWeb2Cmd &data,
                                                                const OwlQueryPairsAnalyser::QueryPairsAnalyser::QueryPairsType &queryPairs,
                                                                EmbedWebServerSession &ref
                                                        ) -> bool {
                                                            boost::ignore_unused(ref);
                                                            data->SSID = queryPairs.find("SSID")->second;
                                                            data->PASSWORD = queryPairs.find("PASSWORD")->second;
                                                            return true;
                                                        }}},
                {R"(/cmd/getWlanDeviceState)",  {OwlMailDefine::WifiCmd::getWlanDeviceState,
                                                        {"DEVICE_NAME",},
                                                        [](
                                                                OwlMailDefine::MailWeb2Cmd &data,
                                                                const OwlQueryPairsAnalyser::QueryPairsAnalyser::QueryPairsType &queryPairs,
                                                                EmbedWebServerSession &ref
                                                        ) -> bool {
                                                            boost::ignore_unused(ref);
                                                            data->DEVICE_NAME = queryPairs.find("DEVICE_NAME")->second;
                                                            return true;
                                                        }}},
        };
    };

    const EmbedWebServerSessionHelperCmd helperCmdLookupTable;

    // ==============================================================================================================

    void EmbedWebServerSession::send_EmptyPairs_error() {
        boost::beast::http::response<boost::beast::http::string_body> res{
                boost::beast::http::status::not_found,
                req_.version()};
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, "text/html");
        res.keep_alive(req_.keep_alive());
        res.body() = "The cmd queryPairs empty.";
        res.prepare_payload();
        return lambda_(std::move(res));
    };

    void EmbedWebServerSession::on_cmd() {

        auto p = parentRef_.lock();
        if (!p) {
            return server_error("(!parentRef_.lock())");
        }

        OwlEmbedWebServer::EmbedWebServerSessionHelperCmd::processCmd(
                shared_from_this(),
                p,
                helperCmdLookupTable
        );

    }

    void EmbedWebServerSession::server_error(std::string what) {
        boost::beast::http::response<boost::beast::http::string_body> res{
                boost::beast::http::status::internal_server_error,
                req_.version()};
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, "text/html");
        res.keep_alive(req_.keep_alive());
        res.body() = std::move(what);
        res.prepare_payload();
        return lambda_(std::move(res));
    }

    void EmbedWebServerSession::bad_request(std::string what) {
        boost::beast::http::response<boost::beast::http::string_body> res{
                boost::beast::http::status::bad_request,
                req_.version()};
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, "text/html");
        res.keep_alive(req_.keep_alive());
        res.body() = std::move(what);
        res.prepare_payload();
        return lambda_(std::move(res));
    }

    void EmbedWebServerSession::send_json(boost::json::value &&o) {
        boost::beast::http::response<boost::beast::http::string_body> res{
                boost::beast::http::status::ok,
                req_.version()};
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, "text/json");
        res.keep_alive(req_.keep_alive());
        res.body() = boost::json::serialize(o);
        res.prepare_payload();
        return lambda_(std::move(res));
    }


} // OwlWebControlService