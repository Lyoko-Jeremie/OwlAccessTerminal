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
#include <boost/assert.hpp>
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


    template<typename SelfType = ProcessJsonMessageSelfTypeInterface, typename SelfPtrType = std::shared_ptr<SelfType> >
    void analysisJoyConGyro(
            SelfPtrType self,
            boost::json::object &joyConGyro,
            int cmdId,
            int packageId
    ) {
        try {
            auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
            m->additionCmd = OwlMailDefine::AdditionCmd::JoyConGyro;
            m->joyConGyroPtr = std::make_shared<OwlMailDefine::JoyConGyro>();
            bool good = true;
            m->joyConGyroPtr->x = getFromJsonObject<int16_t>(joyConGyro, "x", good);
            m->joyConGyroPtr->y = getFromJsonObject<int16_t>(joyConGyro, "y", good);
            m->joyConGyroPtr->z = getFromJsonObject<int16_t>(joyConGyro, "z", good);
            m->joyConGyroPtr->a = getFromJsonObject<int16_t>(joyConGyro, "a", good);
            m->joyConGyroPtr->b = getFromJsonObject<int16_t>(joyConGyro, "b", good);
            m->joyConGyroPtr->c = getFromJsonObject<int16_t>(joyConGyro, "c", good);
            m->joyConGyroPtr->d = getFromJsonObject<int16_t>(joyConGyro, "d", good);
            if (!good) {
                BOOST_LOG_TRIVIAL(warning) << "analysisJoyConGyro getFromJsonObject fail " << joyConGyro;
                self->send_back_json(
                        boost::json::value{
                                {"msg",    "error"},
                                {"error",  "analysisJoyConGyro getFromJsonObject fail"},
                                {"result", false},
                        }
                );
                return;
            }
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
                                {"openError", data->openError},
                        }
                );
            };
            self->sendMail(std::move(m));
        } catch (std::exception &e) {
            BOOST_LOG_TRIVIAL(error) << "CommandService::analysisJoyConSimple";
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

    template<typename SelfType = ProcessJsonMessageSelfTypeInterface, typename SelfPtrType = std::shared_ptr<SelfType> >
    void analysisJoyConSimple(
            SelfPtrType self,
            boost::json::object &joyConSimple,
            int cmdId,
            int packageId
    ) {
        try {
            auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
            m->additionCmd = OwlMailDefine::AdditionCmd::JoyConSimple;
            m->joyConPtr = std::make_shared<OwlMailDefine::JoyCon>();
            bool good = true;
            m->joyConPtr->leftRockerX = getFromJsonObject<int16_t>(joyConSimple, "leftRockerX", good);
            m->joyConPtr->leftRockerY = getFromJsonObject<int16_t>(joyConSimple, "leftRockerY", good);
            m->joyConPtr->rightRockerX = getFromJsonObject<int16_t>(joyConSimple, "rightRockerX", good);
            m->joyConPtr->rightRockerY = getFromJsonObject<int16_t>(joyConSimple, "rightRockerY", good);
            m->joyConPtr->leftBackTop = getFromJsonObject<int8_t>(joyConSimple, "leftBackTop", good);
            m->joyConPtr->leftBackBottom = getFromJsonObject<int8_t>(joyConSimple, "leftBackBottom", good);
            m->joyConPtr->rightBackTop = getFromJsonObject<int8_t>(joyConSimple, "rightBackTop", good);
            m->joyConPtr->rightBackBottom = getFromJsonObject<int8_t>(joyConSimple, "rightBackBottom", good);
            m->joyConPtr->buttonAdd = getFromJsonObject<int8_t>(joyConSimple, "buttonAdd", good);
            m->joyConPtr->buttonReduce = getFromJsonObject<int8_t>(joyConSimple, "buttonReduce", good);
            if (!good) {
                BOOST_LOG_TRIVIAL(warning) << "analysisJoyConSimple getFromJsonObject fail " << joyConSimple;
                self->send_back_json(
                        boost::json::value{
                                {"msg",    "error"},
                                {"error",  "analysisJoyConSimple getFromJsonObject fail"},
                                {"result", false},
                        }
                );
                return;
            }
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
                                {"openError", data->openError},
                        }
                );
            };
            self->sendMail(std::move(m));
        } catch (std::exception &e) {
            BOOST_LOG_TRIVIAL(error) << "CommandService::analysisJoyConSimple";
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

    template<typename SelfType = ProcessJsonMessageSelfTypeInterface, typename SelfPtrType = std::shared_ptr<SelfType> >
    void analysisJoyCon(
            SelfPtrType self,
            boost::json::object &joyCon,
            int cmdId,
            int packageId
    ) {
        try {
            auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
            m->additionCmd = OwlMailDefine::AdditionCmd::JoyCon;
            m->joyConPtr = std::make_shared<OwlMailDefine::JoyCon>();
            bool good = true;
            m->joyConPtr->leftRockerX = getFromJsonObject<int16_t>(joyCon, "leftRockerX", good);
            m->joyConPtr->leftRockerY = getFromJsonObject<int16_t>(joyCon, "leftRockerY", good);
            m->joyConPtr->rightRockerX = getFromJsonObject<int16_t>(joyCon, "rightRockerX", good);
            m->joyConPtr->rightRockerY = getFromJsonObject<int16_t>(joyCon, "rightRockerY", good);
            m->joyConPtr->leftBackTop = getFromJsonObject<int8_t>(joyCon, "leftBackTop", good);
            m->joyConPtr->leftBackBottom = getFromJsonObject<int8_t>(joyCon, "leftBackBottom", good);
            m->joyConPtr->rightBackTop = getFromJsonObject<int8_t>(joyCon, "rightBackTop", good);
            m->joyConPtr->rightBackBottom = getFromJsonObject<int8_t>(joyCon, "rightBackBottom", good);
            m->joyConPtr->CrossUp = getFromJsonObject<int8_t>(joyCon, "CrossUp", good);
            m->joyConPtr->CrossDown = getFromJsonObject<int8_t>(joyCon, "CrossDown", good);
            m->joyConPtr->CrossLeft = getFromJsonObject<int8_t>(joyCon, "CrossLeft", good);
            m->joyConPtr->CrossRight = getFromJsonObject<int8_t>(joyCon, "CrossRight", good);
            m->joyConPtr->A = getFromJsonObject<int8_t>(joyCon, "A", good);
            m->joyConPtr->B = getFromJsonObject<int8_t>(joyCon, "B", good);
            m->joyConPtr->X = getFromJsonObject<int8_t>(joyCon, "X", good);
            m->joyConPtr->Y = getFromJsonObject<int8_t>(joyCon, "Y", good);
            m->joyConPtr->buttonAdd = getFromJsonObject<int8_t>(joyCon, "buttonAdd", good);
            m->joyConPtr->buttonReduce = getFromJsonObject<int8_t>(joyCon, "buttonReduce", good);
            if (!good) {
                BOOST_LOG_TRIVIAL(warning) << "analysisJoyCon getFromJsonObject fail " << joyCon;
                self->send_back_json(
                        boost::json::value{
                                {"msg",    "error"},
                                {"error",  "analysisJoyCon getFromJsonObject fail"},
                                {"result", false},
                        }
                );
                return;
            }
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
                                {"openError", data->openError},
                        }
                );
            };
            self->sendMail(std::move(m));
        } catch (std::exception &e) {
            BOOST_LOG_TRIVIAL(error) << "CommandService::analysisJoyCon";
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

    enum class OwlCmdEnum {
        ping = 0,
        emergencyStop = 120,
        unlock = 92,

        calibrate = 90,
        breakCmd = 10,
        takeoff = 11,
        land = 12,
        move = 13,
        rotate = 14,
        keep = 15,
        gotoCmd = 16,

        led = 17,
        high = 18,
        speed = 19,
        flyMode = 20,

        joyCon = 60,
        joyConSimple = 61,
        joyConGyro = 62,

    };
    enum class OwlCmdRotateEnum {
        cw = 1,
        ccw = 2,
    };
    enum class OwlCmdMoveEnum {
        up = 1,
        down = 2,
        left = 3,
        right = 4,
        forward = 5,
        back = 6,
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
//            const std::shared_ptr<ProcessJsonMessageSelfTypeInterface> &&self
            SelfPtrType &&self
    ) {
        BOOST_ASSERT(self);
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
                self->send_back_json(
                        boost::json::value{
                                {"msg",    "error"},
                                {"error",  "boost::json::parse failed : " + ec.what()},
                                {"result", false},
                        }
                );
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
                case static_cast<int>(OwlCmdEnum::ping):
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
                case static_cast<int>(OwlCmdEnum::calibrate): {
                    // calibrate
                    BOOST_LOG_TRIVIAL(info) << "calibrate";
                    auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
                    m->additionCmd = OwlMailDefine::AdditionCmd::calibrate;
                    m->callbackRunner = [self, cmdId, packageId](
                            const OwlMailDefine::MailSerial2Cmd &data
                    ) {
                        self->send_back_json(
                                boost::json::value{
                                        {"cmdId",     cmdId},
                                        {"packageId", packageId},
                                        {"msg",       "calibrate"},
                                        // {"result",    true},
                                        {"result",    data->ok},
                                        {"openError", data->openError},
                                }
                        );
                    };
                    self->sendMail(std::move(m));
                    break;
                }
                case static_cast<int>(OwlCmdEnum::emergencyStop): {
                    // emergencyStop
                    BOOST_LOG_TRIVIAL(info) << "emergencyStop";
                    auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
                    m->additionCmd = OwlMailDefine::AdditionCmd::emergencyStop;
                    m->callbackRunner = [self, cmdId, packageId](
                            const OwlMailDefine::MailSerial2Cmd &data
                    ) {
                        self->send_back_json(
                                boost::json::value{
                                        {"cmdId",     cmdId},
                                        {"packageId", packageId},
                                        {"msg",       "calibrate"},
                                        // {"result",    true},
                                        {"result",    data->ok},
                                        {"openError", data->openError},
                                }
                        );
                    };
                    self->sendMail(std::move(m));
                    break;
                }
                case static_cast<int>(OwlCmdEnum::unlock): {
                    // unlock
                    BOOST_LOG_TRIVIAL(info) << "unlock";
                    auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
                    m->additionCmd = OwlMailDefine::AdditionCmd::unlock;
                    m->callbackRunner = [self, cmdId, packageId](
                            const OwlMailDefine::MailSerial2Cmd &data
                    ) {
                        self->send_back_json(
                                boost::json::value{
                                        {"cmdId",     cmdId},
                                        {"packageId", packageId},
                                        {"msg",       "calibrate"},
                                        // {"result",    true},
                                        {"result",    data->ok},
                                        {"openError", data->openError},
                                }
                        );
                    };
                    self->sendMail(std::move(m));
                    break;
                }
                case static_cast<int>(OwlCmdEnum::breakCmd): {
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
                                        {"openError", data->openError},
                                }
                        );
                    };
                    self->sendMail(std::move(m));
                    break;
                }
                case static_cast<int>(OwlCmdEnum::takeoff): {
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
                    BOOST_LOG_TRIVIAL(info) << "takeoff distance check ok";
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
                    BOOST_LOG_TRIVIAL(info) << "takeoff moveStepDistance get ok , :" << moveStepDistance;
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
                    m->moveCmdPtr = std::make_shared<OwlMailDefine::MoveCmd>();
                    m->moveCmdPtr->y = int16_t(moveStepDistance);
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
                                        {"openError", data->openError},
                                }
                        );
                    };
                    self->sendMail(std::move(m));
                    break;
                }
                case static_cast<int>(OwlCmdEnum::land): {
                    // land
                    BOOST_LOG_TRIVIAL(info) << "land";
                    auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
                    m->additionCmd = OwlMailDefine::AdditionCmd::land;
                    m->moveCmdPtr = std::make_shared<OwlMailDefine::MoveCmd>();
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
                                        {"openError", data->openError},
                                }
                        );
                    };
                    self->sendMail(std::move(m));
                    break;
                }
                case static_cast<int>(OwlCmdEnum::move):
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
                            case static_cast<int>(OwlCmdMoveEnum::up): {
                                // up
                                BOOST_LOG_TRIVIAL(info) << "move up " << moveStepDistance;
                                auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
                                m->additionCmd = OwlMailDefine::AdditionCmd::move;
                                m->moveCmdPtr = std::make_shared<OwlMailDefine::MoveCmd>();
                                m->moveCmdPtr->y = int16_t(moveStepDistance);
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
                                                    {"openError", data->openError},
                                            }
                                    );
                                };
                                self->sendMail(std::move(m));
                                break;
                            }
                            case static_cast<int>(OwlCmdMoveEnum::down): {
                                // down
                                BOOST_LOG_TRIVIAL(info) << "move down " << moveStepDistance;
                                auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
                                m->additionCmd = OwlMailDefine::AdditionCmd::move;
                                m->moveCmdPtr = std::make_shared<OwlMailDefine::MoveCmd>();
                                m->moveCmdPtr->y = int16_t(-moveStepDistance);
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
                                                    {"openError", data->openError},
                                            }
                                    );
                                };
                                self->sendMail(std::move(m));
                                break;
                            }
                            case static_cast<int>(OwlCmdMoveEnum::left): {
                                // left
                                BOOST_LOG_TRIVIAL(info) << "move left " << moveStepDistance;
                                auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
                                m->additionCmd = OwlMailDefine::AdditionCmd::move;
                                m->moveCmdPtr = std::make_shared<OwlMailDefine::MoveCmd>();
                                m->moveCmdPtr->z = int16_t(-moveStepDistance);
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
                                                    {"openError", data->openError},
                                            }
                                    );
                                };
                                self->sendMail(std::move(m));
                                break;
                            }
                            case static_cast<int>(OwlCmdMoveEnum::right): {
                                // right
                                BOOST_LOG_TRIVIAL(info) << "move right " << moveStepDistance;
                                auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
                                m->additionCmd = OwlMailDefine::AdditionCmd::move;
                                m->moveCmdPtr = std::make_shared<OwlMailDefine::MoveCmd>();
                                m->moveCmdPtr->z = int16_t(moveStepDistance);
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
                                                    {"openError", data->openError},
                                            }
                                    );
                                };
                                self->sendMail(std::move(m));
                                break;
                            }
                            case static_cast<int>(OwlCmdMoveEnum::forward): {
                                // forward
                                BOOST_LOG_TRIVIAL(info) << "move forward " << moveStepDistance;
                                auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
                                m->additionCmd = OwlMailDefine::AdditionCmd::move;
                                m->moveCmdPtr = std::make_shared<OwlMailDefine::MoveCmd>();
                                m->moveCmdPtr->x = int16_t(moveStepDistance);
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
                                                    {"openError", data->openError},
                                            }
                                    );
                                };
                                self->sendMail(std::move(m));
                                break;
                            }
                            case static_cast<int>(OwlCmdMoveEnum::back): {
                                // back
                                BOOST_LOG_TRIVIAL(info) << "move back " << moveStepDistance;
                                auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
                                m->additionCmd = OwlMailDefine::AdditionCmd::move;
                                m->moveCmdPtr = std::make_shared<OwlMailDefine::MoveCmd>();
                                m->moveCmdPtr->x = int16_t(-moveStepDistance);
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
                                                    {"openError", data->openError},
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
                case static_cast<int>(OwlCmdEnum::rotate):
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
                            case static_cast<int>(OwlCmdRotateEnum::cw): {
                                // cw
                                BOOST_LOG_TRIVIAL(info) << "rotate cw " << rote;
                                auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
                                m->additionCmd = OwlMailDefine::AdditionCmd::rotate;
                                m->moveCmdPtr = std::make_shared<OwlMailDefine::MoveCmd>();
                                m->moveCmdPtr->cw = int16_t(rote);
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
                                                    {"openError", data->openError},
                                            }
                                    );
                                };
                                self->sendMail(std::move(m));
                                break;
                            }
                            case static_cast<int>(OwlCmdRotateEnum::ccw): {
                                // ccw
                                BOOST_LOG_TRIVIAL(info) << "rotate ccw " << rote;
                                auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
                                m->additionCmd = OwlMailDefine::AdditionCmd::rotate;
                                m->moveCmdPtr = std::make_shared<OwlMailDefine::MoveCmd>();
                                m->moveCmdPtr->cw = int16_t(-rote);
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
                                                    {"openError", data->openError},
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
                case static_cast<int>(OwlCmdEnum::keep): {
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
                                        {"openError", data->openError},
                                }
                        );
                    };
                    self->sendMail(std::move(m));
                    break;
                }
                case static_cast<int>(OwlCmdEnum::gotoCmd): {
                    // goto position
                    BOOST_LOG_TRIVIAL(info) << "gotoPosition";
                    if (!json_o.contains("x") && !json_o.contains("y") && !json_o.contains("h")) {
                        BOOST_LOG_TRIVIAL(warning) << "gotoPosition contains fail " << jsv;
                        self->send_back_json(
                                boost::json::value{
                                        {"cmdId",     cmdId},
                                        {"packageId", packageId},
                                        {"msg",       "error"},
                                        {"error",     "gotoPosition (x||y||h) not find"},
                                        {"result",    false},
                                }
                        );
                        return;
                    }
                    bool good = true;
                    auto x = getFromJsonObject<int32_t>(json_o, "x", good);
                    auto y = getFromJsonObject<int32_t>(json_o, "y", good);
                    auto h = getFromJsonObject<int32_t>(json_o, "h", good);
                    if (!good) {
                        BOOST_LOG_TRIVIAL(warning) << "gotoPosition getFromJsonObject fail" << jsv;
                        self->send_back_json(
                                boost::json::value{
                                        {"msg",    "error"},
                                        {"error",  "(x||y||h) getFromJsonObject fail"},
                                        {"result", false},
                                }
                        );
                        return;
                    }
                    if (x < 0 || y < 0 || h < 0) {
                        BOOST_LOG_TRIVIAL(warning) << "(x < 0 || y < 0 || h < 0)" << jsv;
                        self->send_back_json(
                                boost::json::value{
                                        {"msg",    "error"},
                                        {"error",  "(x < 0 || y < 0 || h < 0)"},
                                        {"result", false},
                                }
                        );
                        return;
                    }
                    if (x > 32767 || y > 32767 || h > 32767) {
                        BOOST_LOG_TRIVIAL(warning) << "(x > 32767 || y > 32767 || h > 32767)" << jsv;
                        self->send_back_json(
                                boost::json::value{
                                        {"msg",    "error"},
                                        {"error",  "(x > 32767 || y > 32767 || h > 32767)"},
                                        {"result", false},
                                }
                        );
                        return;
                    }
                    auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
                    m->additionCmd = OwlMailDefine::AdditionCmd::gotoPosition;
                    m->moveCmdPtr = std::make_shared<OwlMailDefine::MoveCmd>();
                    m->moveCmdPtr->x = static_cast<int16_t>(y);
                    m->moveCmdPtr->z = static_cast<int16_t>(x);
                    m->moveCmdPtr->y = static_cast<int16_t>(h);
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
                                        {"openError", data->openError},
                                }
                        );
                    };
                    self->sendMail(std::move(m));
                    break;
                }
                case static_cast<int>(OwlCmdEnum::led): {
                    // led
                    BOOST_LOG_TRIVIAL(info) << "led";
                    if (!json_o.contains("ledMode")
                        && !json_o.contains("b") && !json_o.contains("g") && !json_o.contains("r")) {
                        BOOST_LOG_TRIVIAL(warning) << "led contains fail " << jsv;
                        self->send_back_json(
                                boost::json::value{
                                        {"cmdId",     cmdId},
                                        {"packageId", packageId},
                                        {"msg",       "error"},
                                        {"error",     "led (ledMode||x||y||h) not find"},
                                        {"result",    false},
                                }
                        );
                        return;
                    }
                    bool good = true;
                    // 1:static , 2:Breathing , 3:rainbow(sin)
                    auto ledMode = getFromJsonObject<int32_t>(json_o, "ledMode", good);
                    auto b = getFromJsonObject<int32_t>(json_o, "b", good);
                    auto g = getFromJsonObject<int32_t>(json_o, "g", good);
                    auto r = getFromJsonObject<int32_t>(json_o, "r", good);
                    if (!good) {
                        BOOST_LOG_TRIVIAL(warning) << "led getFromJsonObject fail" << jsv;
                        self->send_back_json(
                                boost::json::value{
                                        {"msg",    "error"},
                                        {"error",  "(ledMode||x||y||h) getFromJsonObject fail"},
                                        {"result", false},
                                }
                        );
                        return;
                    }
                    if (b < 0 || g < 0 || r < 0 || b > 255 || g > 255 || r > 255) {
                        BOOST_LOG_TRIVIAL(warning) << "(b < 0 || g < 0 || r < 0 || b > 255 || g > 255 || r > 255)"
                                                   << jsv;
                        self->send_back_json(
                                boost::json::value{
                                        {"msg",    "error"},
                                        {"error",  "(b < 0 || g < 0 || r < 0 || b > 255 || g > 255 || r > 255)"},
                                        {"result", false},
                                }
                        );
                        return;
                    }
                    auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
                    m->additionCmd = OwlMailDefine::AdditionCmd::led;
                    m->moveCmdPtr = std::make_shared<OwlMailDefine::MoveCmd>();
                    m->moveCmdPtr->x = static_cast<int16_t>(b);
                    m->moveCmdPtr->y = static_cast<int16_t>(g);
                    m->moveCmdPtr->z = static_cast<int16_t>(r);
                    m->moveCmdPtr->cw = static_cast<int16_t>(ledMode);
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
                                        {"openError", data->openError},
                                }
                        );
                    };
                    self->sendMail(std::move(m));
                    break;
                }
                case static_cast<int>(OwlCmdEnum::high): {
                    // high
                    BOOST_LOG_TRIVIAL(info) << "high";
                    if (!json_o.contains("high")) {
                        BOOST_LOG_TRIVIAL(warning) << "high contains fail " << jsv;
                        self->send_back_json(
                                boost::json::value{
                                        {"cmdId",     cmdId},
                                        {"packageId", packageId},
                                        {"msg",       "error"},
                                        {"error",     "high (high) not find"},
                                        {"result",    false},
                                }
                        );
                        return;
                    }
                    bool good = true;
                    auto high = getFromJsonObject<int32_t>(json_o, "high", good);
                    if (!good) {
                        BOOST_LOG_TRIVIAL(warning) << "high getFromJsonObject fail" << jsv;
                        self->send_back_json(
                                boost::json::value{
                                        {"msg",    "error"},
                                        {"error",  "(high) high fail"},
                                        {"result", false},
                                }
                        );
                        return;
                    }
                    if (high < 0 || high > 32767) {
                        BOOST_LOG_TRIVIAL(warning) << "(high < 0 || high > 32767)" << jsv;
                        self->send_back_json(
                                boost::json::value{
                                        {"msg",    "error"},
                                        {"error",  "(high < 0 || high > 32767)"},
                                        {"result", false},
                                }
                        );
                        return;
                    }
                    auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
                    m->additionCmd = OwlMailDefine::AdditionCmd::high;
                    m->moveCmdPtr = std::make_shared<OwlMailDefine::MoveCmd>();
                    m->moveCmdPtr->y = static_cast<int16_t>(high);
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
                                        {"openError", data->openError},
                                }
                        );
                    };
                    self->sendMail(std::move(m));
                    break;
                }
                case static_cast<int>(OwlCmdEnum::speed): {
                    // speed
                    BOOST_LOG_TRIVIAL(info) << "speed";
                    if (!json_o.contains("speed")) {
                        BOOST_LOG_TRIVIAL(warning) << "speed contains fail " << jsv;
                        self->send_back_json(
                                boost::json::value{
                                        {"cmdId",     cmdId},
                                        {"packageId", packageId},
                                        {"msg",       "error"},
                                        {"error",     "speed (speed) not find"},
                                        {"result",    false},
                                }
                        );
                        return;
                    }
                    bool good = true;
                    auto speed = getFromJsonObject<int32_t>(json_o, "speed", good);
                    if (!good) {
                        BOOST_LOG_TRIVIAL(warning) << "speed getFromJsonObject fail" << jsv;
                        self->send_back_json(
                                boost::json::value{
                                        {"msg",    "error"},
                                        {"error",  "(speed) speed fail"},
                                        {"result", false},
                                }
                        );
                        return;
                    }
                    if (speed <= 0 || speed > 32767) {
                        BOOST_LOG_TRIVIAL(warning) << "(speed <= 0 || speed > 32767)" << jsv;
                        self->send_back_json(
                                boost::json::value{
                                        {"msg",    "error"},
                                        {"error",  "(speed <= 0 || speed > 32767)"},
                                        {"result", false},
                                }
                        );
                        return;
                    }
                    auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
                    m->additionCmd = OwlMailDefine::AdditionCmd::speed;
                    m->moveCmdPtr = std::make_shared<OwlMailDefine::MoveCmd>();
                    m->moveCmdPtr->x = static_cast<int16_t>(speed);
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
                                        {"openError", data->openError},
                                }
                        );
                    };
                    self->sendMail(std::move(m));
                    break;
                }
                case static_cast<int>(OwlCmdEnum::flyMode): {
                    // flyMode
                    BOOST_LOG_TRIVIAL(info) << "flyMode";
                    if (!json_o.contains("flyMode")) {
                        BOOST_LOG_TRIVIAL(warning) << "flyMode contains fail " << jsv;
                        self->send_back_json(
                                boost::json::value{
                                        {"cmdId",     cmdId},
                                        {"packageId", packageId},
                                        {"msg",       "error"},
                                        {"error",     "flyMode (flyMode) not find"},
                                        {"result",    false},
                                }
                        );
                        return;
                    }
                    bool good = true;
                    auto flyMode = getFromJsonObject<int32_t>(json_o, "flyMode", good);
                    if (!good) {
                        BOOST_LOG_TRIVIAL(warning) << "flyMode getFromJsonObject fail" << jsv;
                        self->send_back_json(
                                boost::json::value{
                                        {"msg",    "error"},
                                        {"error",  "(flyMode) flyMode fail"},
                                        {"result", false},
                                }
                        );
                        return;
                    }
                    auto m = std::make_shared<OwlMailDefine::Cmd2Serial>();
                    m->additionCmd = OwlMailDefine::AdditionCmd::flyMode;
                    m->moveCmdPtr = std::make_shared<OwlMailDefine::MoveCmd>();
                    m->moveCmdPtr->x = static_cast<int16_t>(flyMode);
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
                                        {"openError", data->openError},
                                }
                        );
                    };
                    self->sendMail(std::move(m));
                    break;
                }
                case static_cast<int>(OwlCmdEnum::joyCon): {
                    // joystick
                    BOOST_LOG_TRIVIAL(info) << "JoyCon";
                    if (!json_o.contains("JoyCon")) {
                        BOOST_LOG_TRIVIAL(warning) << "JoyCon contains fail " << jsv;
                        self->send_back_json(
                                boost::json::value{
                                        {"cmdId",     cmdId},
                                        {"packageId", packageId},
                                        {"msg",       "error"},
                                        {"error",     "JoyCon (JoyCon) not find"},
                                        {"result",    false},
                                }
                        );
                        return;
                    }
                    bool good = true;
                    auto joyConObj = json_o.at("JoyCon");
                    if (!joyConObj.is_object()) {
                        BOOST_LOG_TRIVIAL(warning) << "joyConObj (!JoyCon.is_object())" << jsv;
                        self->send_back_json(
                                boost::json::value{
                                        {"msg",    "error"},
                                        {"error",  "(!JoyCon.is_object())"},
                                        {"result", false},
                                }
                        );
                        return;
                    }
                    analysisJoyCon(self, joyConObj.as_object(), cmdId, packageId);
                    break;
                }
                case static_cast<int>(OwlCmdEnum::joyConSimple): {
                    // joystick
                    BOOST_LOG_TRIVIAL(info) << "JoyConSimple";
                    if (!json_o.contains("JoyConSimple")) {
                        BOOST_LOG_TRIVIAL(warning) << "JoyConSimple contains fail " << jsv;
                        self->send_back_json(
                                boost::json::value{
                                        {"cmdId",     cmdId},
                                        {"packageId", packageId},
                                        {"msg",       "error"},
                                        {"error",     "JoyConSimple (JoyConSimple) not find"},
                                        {"result",    false},
                                }
                        );
                        return;
                    }
                    bool good = true;
                    auto joyConSimpleObj = json_o.at("JoyConSimple");
                    if (!joyConSimpleObj.is_object()) {
                        BOOST_LOG_TRIVIAL(warning) << "joyConSimpleObj (!JoyConSimple.is_object())" << jsv;
                        self->send_back_json(
                                boost::json::value{
                                        {"msg",    "error"},
                                        {"error",  "(!JoyConSimple.is_object())"},
                                        {"result", false},
                                }
                        );
                        return;
                    }
                    analysisJoyConSimple(self, joyConSimpleObj.as_object(), cmdId, packageId);
                    break;
                }
                case static_cast<int>(OwlCmdEnum::joyConGyro): {
                    // joystick
                    BOOST_LOG_TRIVIAL(info) << "JoyConGyro";
                    if (!json_o.contains("JoyConGyro")) {
                        BOOST_LOG_TRIVIAL(warning) << "JoyConGyro contains fail " << jsv;
                        self->send_back_json(
                                boost::json::value{
                                        {"cmdId",     cmdId},
                                        {"packageId", packageId},
                                        {"msg",       "error"},
                                        {"error",     "JoyConGyro (JoyConGyro) not find"},
                                        {"result",    false},
                                }
                        );
                        return;
                    }
                    bool good = true;
                    auto joyConGyroObj = json_o.at("JoyConGyro");
                    if (!joyConGyroObj.is_object()) {
                        BOOST_LOG_TRIVIAL(warning) << "joyConGyroObj (!JoyConGyro.is_object())" << jsv;
                        self->send_back_json(
                                boost::json::value{
                                        {"msg",    "error"},
                                        {"error",  "(!JoyConGyro.is_object())"},
                                        {"result", false},
                                }
                        );
                        return;
                    }
                    analysisJoyConGyro(self, joyConGyroObj.as_object(), cmdId, packageId);
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
            BOOST_LOG_TRIVIAL(error) << "CommandService::process_message";
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
