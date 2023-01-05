// jeremie

#ifndef OWLACCESSTERMINAL_PROCESSJSONMESSAGE_H
#define OWLACCESSTERMINAL_PROCESSJSONMESSAGE_H

#include <memory>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/json.hpp>
#include <boost/system/error_code.hpp>
#include <boost/log/trivial.hpp>
#include <boost/utility/string_view.hpp>
#include "CmdSerialMail.h"

namespace OwlProcessJsonMessage {

    template<typename T>
    T getFromJsonObject(const boost::json::object &v, boost::string_view key, bool &good) {
        try {
            T r = boost::json::value_to<T>(v.at(key));
            return r;
        } catch (std::exception &e) {
            good = false;
            return T{};
        }
    }

    class ProcessJsonMessageSelfTypeInterface {
    public:
        ProcessJsonMessageSelfTypeInterface() = default;

        virtual ~ProcessJsonMessageSelfTypeInterface() = default;

        virtual void send_back_json(const boost::json::value &json_value) = 0;

        virtual void sendMail(OwlMailDefine::MailCmd2Serial &&data) = 0;
    };

    /**
     *
     * @tparam SelfType : must impl `send_back_json(const boost::json::value &json_value)`
     *                          and `sendMail(OwlMailDefine::MailCmd2Serial &&data)`
     * @tparam SelfPtrType = std::shared_ptr<SelfType>
     * @param jsv   : json string_view
     * @param json_parse_options_
     * @param json_storage_resource_
     * @param self = SelfPtrType
     */
    template<typename SelfType = ProcessJsonMessageSelfTypeInterface, typename SelfPtrType = std::shared_ptr<SelfType> >
    void process_json_message(
            boost::string_view jsv,
            boost::json::static_resource &json_storage_resource_,
            boost::json::parse_options &json_parse_options_,
//            const std::shared_ptr<ProcessJsonMessageSelfTypeInterface> &self
            const SelfPtrType &self
    ) {
        try {
            boost::system::error_code ec;
            // auto jsv = boost::string_view{receive_buffer_.data(), bytes_transferred};
            boost::json::value json_v = boost::json::parse(
                    jsv,
                    ec,
                    &json_storage_resource_,
                    json_parse_options_
            );
            if (ec) {
                // ignore
                // std::cerr << ec.what() << "\n";
                BOOST_LOG_TRIVIAL(error) << ec.what();
                return;
            }
            //  std::cout << boost::json::serialize(json_v) << "\n";
            BOOST_LOG_TRIVIAL(info) << boost::json::serialize(json_v);
            auto json_o = json_v.as_object();
            if (!json_o.contains("cmdId") && !json_o.contains("packageId")) {
                BOOST_LOG_TRIVIAL(warning) << "contains fail " << jsv;
                self->send_back_json(
                        boost::json::value{
                                {"msg",    "error"},
                                {"error",  "(cmdId||packageId) not find"},
                                {"result", false},
                        }
                );
                return;
            }
            bool good = true;
            auto cmdId = getFromJsonObject<int32_t>(json_o, "cmdId", good);
            auto packageId = getFromJsonObject<int32_t>(json_o, "packageId", good);
            if (!good) {
                BOOST_LOG_TRIVIAL(warning) << "getFromJsonObject fail " << jsv;
                self->send_back_json(
                        boost::json::value{
                                {"msg",    "error"},
                                {"error",  "(cmdId||packageId) getFromJsonObject fail"},
                                {"result", false},
                        }
                );
                return;
            }
            switch (cmdId) {
                case 0:
                    // ping-pong
                    BOOST_LOG_TRIVIAL(info) << "ping-pong";
                    self->send_back_json(
                            boost::json::value{
                                    {"cmdId",     cmdId},
                                    {"packageId", packageId},
                                    {"msg",       "pong"},
                                    {"result",    true},
                            }
                    );
                    break;
                case 10: {
                    // break
                    BOOST_LOG_TRIVIAL(info) << "break";
                    auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
                    m->additionCmd = OwlMailDefine::AdditionCmd::stop;
                    m->callbackRunner = [self, cmdId, packageId](
                            const OwlMailDefine::MailSerial2Cmd &data
                    ) {
                        self->send_back_json(
                                boost::json::value{
                                        {"cmdId",     cmdId},
                                        {"packageId", packageId},
                                        {"msg",       "break"},
                                        // {"result",    true},
                                        {"result",    data->ok},
                                }
                        );
                    };
                    self->sendMail(std::move(m));
                    break;
                }
                case 11: {
                    // takeoff
                    BOOST_LOG_TRIVIAL(info) << "takeoff";
                    if (!json_o.contains("distance")) {
                        BOOST_LOG_TRIVIAL(warning) << "move step contains fail " << jsv;
                        self->send_back_json(
                                boost::json::value{
                                        {"cmdId",     cmdId},
                                        {"packageId", packageId},
                                        {"msg",       "error"},
                                        {"error",     "takeoff (distance) not find"},
                                        {"result",    false},
                                }
                        );
                        return;
                    }
                    bool good = true;
                    auto moveStepDistance = getFromJsonObject<int32_t>(json_o, "distance", good);
                    if (!good) {
                        BOOST_LOG_TRIVIAL(warning) << "takeoff getFromJsonObject fail" << jsv;
                        self->send_back_json(
                                boost::json::value{
                                        {"msg",    "error"},
                                        {"error",  "(distance) getFromJsonObject fail"},
                                        {"result", false},
                                }
                        );
                        return;
                    }
                    if (moveStepDistance > 32767 || moveStepDistance < 0) {
                        BOOST_LOG_TRIVIAL(warning) << "(moveStepDistance > 32767 || moveStepDistance < 0)" << jsv;
                        self->send_back_json(
                                boost::json::value{
                                        {"msg",    "error"},
                                        {"error",  "(moveStepDistance > 32767 || moveStepDistance < 0)"},
                                        {"result", false},
                                }
                        );
                        return;
                    }
                    auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
                    m->additionCmd = OwlMailDefine::AdditionCmd::takeoff;
                    m->y = int16_t(moveStepDistance);
                    m->callbackRunner = [self, cmdId, packageId](
                            const OwlMailDefine::MailSerial2Cmd &data
                    ) {
                        self->send_back_json(
                                boost::json::value{
                                        {"cmdId",     cmdId},
                                        {"packageId", packageId},
                                        {"msg",       "takeoff"},
                                        // {"result",    true},
                                        {"result",    data->ok},
                                }
                        );
                    };
                    self->sendMail(std::move(m));
                    break;
                }
                case 12: {
                    // land
                    BOOST_LOG_TRIVIAL(info) << "land";
                    auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
                    m->additionCmd = OwlMailDefine::AdditionCmd::land;
                    m->callbackRunner = [self, cmdId, packageId](
                            const OwlMailDefine::MailSerial2Cmd &data
                    ) {
                        self->send_back_json(
                                boost::json::value{
                                        {"cmdId",     cmdId},
                                        {"packageId", packageId},
                                        {"msg",       "land"},
                                        // {"result",    true},
                                        {"result",    data->ok},
                                }
                        );
                    };
                    self->sendMail(std::move(m));
                    break;
                }
                case 13:
                    // move step
                    BOOST_LOG_TRIVIAL(info) << "move step";
                    {
                        if (!json_o.contains("forward") && !json_o.contains("distance")) {
                            BOOST_LOG_TRIVIAL(warning) << "move step contains fail " << jsv;
                            self->send_back_json(
                                    boost::json::value{
                                            {"cmdId",     cmdId},
                                            {"packageId", packageId},
                                            {"msg",       "error"},
                                            {"error",     "move step (forward||distance) not find"},
                                            {"result",    false},
                                    }
                            );
                            return;
                        }
                        bool good = true;
                        auto moveStepForward = getFromJsonObject<int32_t>(json_o, "forward", good);
                        auto moveStepDistance = getFromJsonObject<int32_t>(json_o, "distance", good);
                        if (!good) {
                            BOOST_LOG_TRIVIAL(warning) << "move step getFromJsonObject fail" << jsv;
                            self->send_back_json(
                                    boost::json::value{
                                            {"msg",    "error"},
                                            {"error",  "(forward||distance) getFromJsonObject fail"},
                                            {"result", false},
                                    }
                            );
                            return;
                        }
                        if (moveStepDistance > 32767 || moveStepDistance < 0) {
                            BOOST_LOG_TRIVIAL(warning) << "(moveStepDistance > 32767 || moveStepDistance < 0)" << jsv;
                            self->send_back_json(
                                    boost::json::value{
                                            {"msg",    "error"},
                                            {"error",  "(moveStepDistance > 32767 || moveStepDistance < 0)"},
                                            {"result", false},
                                    }
                            );
                            return;
                        }
                        switch (moveStepForward) {
                            case 1: {
                                // up
                                BOOST_LOG_TRIVIAL(info) << "move up " << moveStepDistance;
                                auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
                                m->additionCmd = OwlMailDefine::AdditionCmd::move;
                                m->y = int16_t(moveStepDistance);
                                m->callbackRunner = [self, cmdId, packageId](
                                        const OwlMailDefine::MailSerial2Cmd &data
                                ) {
                                    self->send_back_json(
                                            boost::json::value{
                                                    {"cmdId",     cmdId},
                                                    {"packageId", packageId},
                                                    {"msg",       "up"},
                                                    // {"result",    true},
                                                    {"result",    data->ok},
                                            }
                                    );
                                };
                                self->sendMail(std::move(m));
                                break;
                            }
                            case 2: {
                                // down
                                BOOST_LOG_TRIVIAL(info) << "move down " << moveStepDistance;
                                auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
                                m->additionCmd = OwlMailDefine::AdditionCmd::move;
                                m->y = int16_t(-moveStepDistance);
                                m->callbackRunner = [self, cmdId, packageId](
                                        const OwlMailDefine::MailSerial2Cmd &data
                                ) {
                                    self->send_back_json(
                                            boost::json::value{
                                                    {"cmdId",     cmdId},
                                                    {"packageId", packageId},
                                                    {"msg",       "down"},
                                                    // {"result",    true},
                                                    {"result",    data->ok},
                                            }
                                    );
                                };
                                self->sendMail(std::move(m));
                                break;
                            }
                            case 3: {
                                // left
                                BOOST_LOG_TRIVIAL(info) << "move left " << moveStepDistance;
                                auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
                                m->additionCmd = OwlMailDefine::AdditionCmd::move;
                                m->z = int16_t(-moveStepDistance);
                                m->callbackRunner = [self, cmdId, packageId](
                                        const OwlMailDefine::MailSerial2Cmd &data
                                ) {
                                    self->send_back_json(
                                            boost::json::value{
                                                    {"cmdId",     cmdId},
                                                    {"packageId", packageId},
                                                    {"msg",       "left"},
                                                    // {"result",    true},
                                                    {"result",    data->ok},
                                            }
                                    );
                                };
                                self->sendMail(std::move(m));
                                break;
                            }
                            case 4: {
                                // right
                                BOOST_LOG_TRIVIAL(info) << "move right " << moveStepDistance;
                                auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
                                m->additionCmd = OwlMailDefine::AdditionCmd::move;
                                m->z = int16_t(moveStepDistance);
                                m->callbackRunner = [self, cmdId, packageId](
                                        const OwlMailDefine::MailSerial2Cmd &data
                                ) {
                                    self->send_back_json(
                                            boost::json::value{
                                                    {"cmdId",     cmdId},
                                                    {"packageId", packageId},
                                                    {"msg",       "right"},
                                                    // {"result",    true},
                                                    {"result",    data->ok},
                                            }
                                    );
                                };
                                self->sendMail(std::move(m));
                                break;
                            }
                            case 5: {
                                // forward
                                BOOST_LOG_TRIVIAL(info) << "move forward " << moveStepDistance;
                                auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
                                m->additionCmd = OwlMailDefine::AdditionCmd::move;
                                m->x = int16_t(moveStepDistance);
                                m->callbackRunner = [self, cmdId, packageId](
                                        const OwlMailDefine::MailSerial2Cmd &data
                                ) {
                                    self->send_back_json(
                                            boost::json::value{
                                                    {"cmdId",     cmdId},
                                                    {"packageId", packageId},
                                                    {"msg",       "forward"},
                                                    // {"result",    true},
                                                    {"result",    data->ok},
                                            }
                                    );
                                };
                                self->sendMail(std::move(m));
                                break;
                            }
                            case 6: {
                                // back
                                BOOST_LOG_TRIVIAL(info) << "move back " << moveStepDistance;
                                auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
                                m->additionCmd = OwlMailDefine::AdditionCmd::move;
                                m->x = int16_t(-moveStepDistance);
                                m->callbackRunner = [self, cmdId, packageId](
                                        const OwlMailDefine::MailSerial2Cmd &data
                                ) {
                                    self->send_back_json(
                                            boost::json::value{
                                                    {"cmdId",     cmdId},
                                                    {"packageId", packageId},
                                                    {"msg",       "back"},
                                                    // {"result",    true},
                                                    {"result",    data->ok},
                                            }
                                    );
                                };
                                self->sendMail(std::move(m));
                                break;
                            }
                            default:
                                // ignore
                                BOOST_LOG_TRIVIAL(warning) << "move ignore " << jsv;
                                self->send_back_json(
                                        boost::json::value{
                                                {"cmdId",     cmdId},
                                                {"packageId", packageId},
                                                {"msg",       "move ignore"},
                                                {"result",    false},
                                        }
                                );
                                break;
                        }
                    }
                    break;
                case 14:
                    // rotate
                    BOOST_LOG_TRIVIAL(info) << "rotate ";
                    {
                        if (!json_o.contains("rotate") && !json_o.contains("rote")) {
                            BOOST_LOG_TRIVIAL(warning) << "rotate contains fail " << jsv;
                            self->send_back_json(
                                    boost::json::value{
                                            {"cmdId",     cmdId},
                                            {"packageId", packageId},
                                            {"msg",       "error"},
                                            {"error",     "rotate (rotate||rote) not find"},
                                            {"result",    false},
                                    }
                            );
                            return;
                        }
                        bool good = true;
                        auto rotate = getFromJsonObject<int32_t>(json_o, "rotate", good);
                        auto rote = getFromJsonObject<int32_t>(json_o, "rote", good);
                        if (!good) {
                            BOOST_LOG_TRIVIAL(warning) << "rotate getFromJsonObject fail" << jsv;
                            self->send_back_json(
                                    boost::json::value{
                                            {"msg",    "error"},
                                            {"error",  "(rotate||rote) getFromJsonObject fail"},
                                            {"result", false},
                                    }
                            );
                            return;
                        }
                        if (rote > 360 || rote < 0) {
                            BOOST_LOG_TRIVIAL(warning) << "(rote > 360 || rote < 0)" << jsv;
                            self->send_back_json(
                                    boost::json::value{
                                            {"msg",    "error"},
                                            {"error",  "(rote > 360 || rote < 0)"},
                                            {"result", false},
                                    }
                            );
                            return;
                        }
                        switch (rotate) {
                            case 1: {
                                // cw
                                BOOST_LOG_TRIVIAL(info) << "rotate cw " << rote;
                                auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
                                m->additionCmd = OwlMailDefine::AdditionCmd::rotate;
                                m->cw = int16_t(rote);
                                m->callbackRunner = [self, cmdId, packageId](
                                        const OwlMailDefine::MailSerial2Cmd &data
                                ) {
                                    self->send_back_json(
                                            boost::json::value{
                                                    {"cmdId",     cmdId},
                                                    {"packageId", packageId},
                                                    {"msg",       "rotate cw"},
                                                    // {"result",    true},
                                                    {"result",    data->ok},
                                            }
                                    );
                                };
                                self->sendMail(std::move(m));
                                break;
                            }
                            case 2: {
                                // ccw
                                BOOST_LOG_TRIVIAL(info) << "rotate ccw " << rote;
                                auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
                                m->additionCmd = OwlMailDefine::AdditionCmd::rotate;
                                m->cw = int16_t(-rote);
                                m->callbackRunner = [self, cmdId, packageId](
                                        OwlMailDefine::MailSerial2Cmd data
                                ) {
                                    self->send_back_json(
                                            boost::json::value{
                                                    {"cmdId",     cmdId},
                                                    {"packageId", packageId},
                                                    {"msg",       "rotate ccw"},
                                                    // {"result",    true},
                                                    {"result",    data->ok},
                                            }
                                    );
                                };
                                self->sendMail(std::move(m));
                                break;
                            }
                            default:
                                // ignore
                                BOOST_LOG_TRIVIAL(info) << "rotate ignore " << jsv;
                                self->send_back_json(
                                        boost::json::value{
                                                {"cmdId",     cmdId},
                                                {"packageId", packageId},
                                                {"msg",       "rotate ignore"},
                                                {"result",    false},
                                        }
                                );
                                break;
                        }
                    }
                    break;
                case 15: {
                    // keep position
                    BOOST_LOG_TRIVIAL(info) << "keep";
                    auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
                    m->additionCmd = OwlMailDefine::AdditionCmd::keep;
                    m->callbackRunner = [self, cmdId, packageId](
                            OwlMailDefine::MailSerial2Cmd data
                    ) {
                        self->send_back_json(
                                boost::json::value{
                                        {"cmdId",     cmdId},
                                        {"packageId", packageId},
                                        {"msg",       "keep"},
                                        // {"result",    true},
                                        {"result",    data->ok},
                                }
                        );
                    };
                    self->sendMail(std::move(m));
                    break;
                }
                default:
                    // ignore
                    BOOST_LOG_TRIVIAL(warning) << "ignore " << jsv;
                    self->send_back_json(
                            boost::json::value{
                                    {"cmdId",     cmdId},
                                    {"packageId", packageId},
                                    {"msg",       "ignore"},
                                    {"result",    false},
                            }
                    );
                    break;
            }
            return;
        } catch (std::exception &e) {
            std::cerr << "CommandService::process_message \n" << e.what();
            BOOST_LOG_TRIVIAL(error) << "CommandService::process_message \n" << e.what();
            // ignore
            self->send_back_json(
                    boost::json::value{
                            {"msg",    "exception"},
                            {"error",  e.what()},
                            {"result", false},
                    }
            );
            return;
        } catch (...) {
            // ignore
            self->send_back_json(
                    boost::json::value{
                            {"msg",    "exception"},
                            {"error",  "(...)"},
                            {"result", false},
                    }
            );
            return;
        }
    }
}

#endif //OWLACCESSTERMINAL_PROCESSJSONMESSAGE_H
