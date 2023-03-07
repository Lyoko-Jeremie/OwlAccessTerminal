// jeremie

#include "SerialController.h"
#include "StateReader/StateReader.h"
#include <boost/asio/read_until.hpp>
#include <boost/assert.hpp>
#include <array>
#include <utility>
#include <type_traits>

namespace OwlSerialController {

#ifdef DEBUG_IF_CHECK_POINT
    constexpr bool flag_DEBUG_IF_CHECK_POINT = true;
#else
    constexpr bool flag_DEBUG_IF_CHECK_POINT = false;
#endif // DEBUG_IF_CHECK_POINT

    bool SerialController::initPort() {
        BOOST_LOG_OWL(trace) << "SerialController::initPort";
        if constexpr (flag_DEBUG_IF_CHECK_POINT) {
            BOOST_ASSERT(!weak_from_this().expired());
        }
        BOOST_ASSERT(airplanePortController);
        BOOST_LOG_OWL(trace) << "SerialController::initPort close";
        airplanePortController->close();
        // set and open the airplanePortController
        BOOST_LOG_OWL(trace) << "SerialController::initPort open";
        BOOST_LOG_OWL(trace) << "config_->config().airplane_fly_serial_addr "
                             << config_->config().airplane_fly_serial_addr;
        if constexpr (flag_DEBUG_IF_CHECK_POINT) {
            BOOST_ASSERT(!weak_from_this().expired());
            BOOST_LOG_OWL(trace) << "SerialController::initPort weak_from_this().use_count(): "
                                 << weak_from_this().use_count();
            BOOST_LOG_OWL(trace) << "SerialController::initPort shared_from_this().use_count(): "
                                 << this->shared_from_this().use_count();
        }
        initOk = airplanePortController->open(config_->config().airplane_fly_serial_addr);
        BOOST_LOG_OWL(trace) << "SerialController::initPort open " << initOk;
        if (initOk) {
            initOk = airplanePortController->set_option(
                    boost::asio::serial_port::baud_rate(config_->config().airplane_fly_serial_baud_rate)
            );
            BOOST_LOG_OWL(trace) << "SerialController::initPort set_option " << initOk;
        }
        BOOST_LOG_OWL(trace) << "SerialController::initPort initOk " << initOk;
        if (!initOk) {
            return false;
        }
        return true;
    }

    void SerialController::receiveMailGetAirplaneState(
            OwlMailDefine::MailCmd2Serial &&data,
            OwlMailDefine::CmdSerialMailbox &mailbox) {
        switch (data->additionCmd) {
            case OwlMailDefine::AdditionCmd::getAirplaneState: {
                auto pn = atomic_load(&newestAirplaneState);
                auto pa = atomic_load(&aprilTagCmdData);
                if (!pn || !pa) {
                    auto data_r = boost::make_shared<OwlMailDefine::Serial2Cmd>();
                    data_r->runner = data->callbackRunner;
                    data_r->openError = true;
                    data_r->ok = false;
                    sendMail(std::move(data_r), mailbox);
                    return;
                }
                auto data_r = boost::make_shared<OwlMailDefine::Serial2Cmd>();
                data_r->runner = data->callbackRunner;
                data_r->ok = pn && pa;
                data_r->newestAirplaneState = pn->shared_from_this();
                data_r->aprilTagCmdData = pa->shared_from_this();
                sendMail(std::move(data_r), mailbox);
                return;
            }
            default: {
                BOOST_LOG_OWL(error) << "SerialController"
                                     << " receiveMailGetAirplaneState"
                                     << " switch (data->additionCmd) default";
                auto data_r = boost::make_shared<OwlMailDefine::Serial2Cmd>();
                data_r->runner = data->callbackRunner;
                data_r->openError = true;
                data_r->ok = false;
                sendMail(std::move(data_r), mailbox);
                return;
            }
        }
    }

    template<uint8_t packageSize>
    void sendADataBuffer(
            boost::shared_ptr<SerialController> selfPtr,
            boost::shared_ptr<std::array<uint8_t, packageSize>> sendDataBuffer,
            OwlMailDefine::MailCmd2Serial data,
            OwlMailDefine::CmdSerialMailbox &mailbox
    ) {
        BOOST_ASSERT(selfPtr);
        BOOST_ASSERT(sendDataBuffer);
        BOOST_ASSERT(data);
        // send it
        boost::asio::async_write(
                *(selfPtr->airplanePortController->sp_),
                boost::asio::buffer(*sendDataBuffer),
                boost::asio::transfer_exactly(sendDataBuffer->size()),
                [selfPtr, sendDataBuffer, data, &mailbox](
                        const boost::system::error_code &ec,
                        size_t bytes_transferred
                ) {
                    BOOST_ASSERT(data);
                    boost::ignore_unused(bytes_transferred);
                    // make cmd result
                    auto data_r = boost::make_shared<OwlMailDefine::Serial2Cmd>();
                    data_r->runner = data->callbackRunner;
                    if (ec) {
                        // error
                        BOOST_LOG_OWL(error) << "SerialController"
                                             << " receiveMail"
                                             << " async_write error: "
                                             << ec.what();
                        data_r->ok = false;
                        selfPtr->sendMail(std::move(data_r), mailbox);
                        return;
                    }
                    data_r->ok = true;
                    selfPtr->sendMail(std::move(data_r), mailbox);
                }
        );
    }

    template<uint8_t packageSize>
    void makeADataBuffer(
            std::array<uint8_t, packageSize> &&dataInitList,
            boost::shared_ptr<SerialController> selfPtr,
            OwlMailDefine::MailCmd2Serial data,
            OwlMailDefine::CmdSerialMailbox &mailbox
    ) {
        BOOST_ASSERT(selfPtr);
        BOOST_ASSERT(data);
        auto sendDataBuffer = boost::make_shared<std::array<uint8_t, packageSize>>(std::move(dataInitList));
        // add xor checksum
        if (sendDataBuffer->size() > 5) {
            for (size_t i = 3; i != packageSize - 2; ++i) {
                sendDataBuffer->at(packageSize - 2) ^= sendDataBuffer->at(i);
            }
        }
        // send it
        sendADataBuffer<packageSize>(
                selfPtr,
                sendDataBuffer,
                data,
                mailbox
        );
    }

    // https://stackoverflow.com/questions/8357240/how-to-automatically-convert-strongly-typed-enum-into-int
    template<typename E>
    constexpr auto to_underlying(E e) noexcept {
        return static_cast<std::underlying_type_t<E>>(e);
    }


    void SerialController::receiveMailRepeat(OwlMailDefine::MailCmd2Serial &&data,
                                             OwlMailDefine::CmdSerialMailbox &mailbox) {
        BOOST_ASSERT(data);
        if constexpr (flag_DEBUG_IF_CHECK_POINT) {
            BOOST_ASSERT(!weak_from_this().expired());
        }
        boost::asio::dispatch(ioc_, [
                this, self = shared_from_this(), data, &mailbox]() {
            if (!package_record_->mail()) {
                auto data_r = boost::make_shared<OwlMailDefine::Serial2Cmd>();
                data_r->runner = data->callbackRunner;
                data_r->openError = true;
                data_r->ok = false;
                sendMail(std::move(data_r), mailbox);
                return;
            }
            package_record_->mail()->callbackRunner = data->callbackRunner;
#ifdef DEBUG_ReceiveMailRepeat
            if (auto n = OwlMailDefine::AdditionCmdNameLookupTable.find(package_record_->mail()->additionCmd);
                    n != OwlMailDefine::AdditionCmdNameLookupTable.end()) {
                BOOST_LOG_OWL(trace) << "SerialController::receiveMailRepeat package_record "
                                         << " lastUpdateTime " << package_record_->lastUpdateTime
                                         << " id " << package_record_->id()
                                         << " additionCmd " << to_underlying(package_record_->mail()->additionCmd)
                                         << " additionCmd " << n->second;
            } else {
                BOOST_LOG_OWL(warning) << "SerialController::receiveMailRepeat package_record "
                                           << " lastUpdateTime " << package_record_->lastUpdateTime
                                           << " id " << package_record_->id()
                                           << " additionCmd unknown "
                                           << " additionCmd "
                                           << to_underlying(package_record_->mail()->additionCmd);
            }
#endif // DEBUG_ReceiveMailRepeat
            sendData2Serial(package_record_->mail(), mailbox, package_record_->id(), true, false);
        });
    }

    void SerialController::receiveMail(OwlMailDefine::MailCmd2Serial &&data, OwlMailDefine::CmdSerialMailbox &mailbox) {
        BOOST_ASSERT(data);
        if (auto n = OwlMailDefine::AdditionCmdNameLookupTable.find(data->additionCmd);
                n != OwlMailDefine::AdditionCmdNameLookupTable.end()) {
            if (data->additionCmd != OwlMailDefine::AdditionCmd::ping) {
                BOOST_LOG_OWL(debug_sp_w) << "SerialController::receiveMail "
                                          << to_underlying(data->additionCmd) << " additionCmd:"
                                          << n->second;
            }
        } else {
            BOOST_LOG_OWL(warning) << "SerialController::receiveMail unknown additionCmd : "
                                   << to_underlying(data->additionCmd);
        }
        if (data->additionCmd == OwlMailDefine::AdditionCmd::getAirplaneState) {
            receiveMailGetAirplaneState(std::move(data), mailbox);
            return;
        }
        if constexpr (flag_DEBUG_IF_CHECK_POINT) {
            BOOST_ASSERT(!weak_from_this().expired());
        }
        // BOOST_ASSERT(data->additionCmd != OwlMailDefine::AdditionCmd::ignore);
        boost::asio::dispatch(ioc_, [
                this, self = shared_from_this(), data, &mailbox]() {
            if (data->additionCmd != OwlMailDefine::AdditionCmd::ping) {
                BOOST_LOG_OWL(trace_cmd_sp_w) << "SerialController::receiveMail " << "dispatch initOk";
            }
            BOOST_ASSERT(data->additionCmd != OwlMailDefine::AdditionCmd::ignore);
            switch (data->additionCmd) {
                case OwlMailDefine::AdditionCmd::takeoff:
                case OwlMailDefine::AdditionCmd::land:
                case OwlMailDefine::AdditionCmd::stop:
                case OwlMailDefine::AdditionCmd::move:
                case OwlMailDefine::AdditionCmd::rotate:
                case OwlMailDefine::AdditionCmd::keep:
                case OwlMailDefine::AdditionCmd::high:
                case OwlMailDefine::AdditionCmd::speed:
                case OwlMailDefine::AdditionCmd::flyMode:
                case OwlMailDefine::AdditionCmd::gotoPosition:
                case OwlMailDefine::AdditionCmd::led:
                case OwlMailDefine::AdditionCmd::emergencyStop:
                case OwlMailDefine::AdditionCmd::unlock:
                case OwlMailDefine::AdditionCmd::calibrate:
                    // need repeat

                    // // clone a package without callback to recorder
                    // atomic_store(&package_record_->mail, data->repeat());
                    // // send package with origin callback
                    // sendData2Serial(data, mailbox, package_record_->nextId(), false);

                    // above logic moved to `sendData2Serial`
                    sendData2Serial(data, mailbox, 0, false, true);
                    break;
                case OwlMailDefine::AdditionCmd::ignore:
                case OwlMailDefine::AdditionCmd::ping:
                case OwlMailDefine::AdditionCmd::AprilTag:
                case OwlMailDefine::AdditionCmd::JoyCon:
                case OwlMailDefine::AdditionCmd::JoyConSimple:
                case OwlMailDefine::AdditionCmd::JoyConGyro:
                    // no repeat
                    sendData2Serial(data, mailbox, 0, false, false);
                    break;
                case OwlMailDefine::AdditionCmd::getAirplaneState:
                    BOOST_ASSERT_MSG(false, (std::string{"receiveMail boost::asio::dispatch"} +
                                             std::string{" case OwlMailDefine::AdditionCmd::getAirplaneState"} +
                                             std::string{" never go there!!!"}).c_str());
                    break;
                default:
                    // if we go there, ill program !!!
                    sendData2Serial(data, mailbox, 0, false, false);
                    break;
            }
        });
    }

    void SerialController::sendData2Serial(
            OwlMailDefine::MailCmd2Serial data,
            OwlMailDefine::CmdSerialMailbox &mailbox,
            uint16_t packageId,
            bool repeating,
            bool needRepeat) {
        // warning: this function must call inside `this.ioc_`
        //      and, must keep data alive

        BOOST_ASSERT(data);
        if (data->additionCmd != OwlMailDefine::AdditionCmd::ping && !repeating) {
            BOOST_LOG_OWL(trace_cmd_sp_w) << "SerialController::sendData2Serial " << "dispatch";
        }
        if constexpr (flag_DEBUG_IF_CHECK_POINT) {
            BOOST_ASSERT(!weak_from_this().expired());
            BOOST_ASSERT(this->weak_from_this().use_count() > 0);
        }
        if (!initOk && !initPort()) {
            BOOST_LOG_OWL(trace_cmd_sp_w) << "SerialController::sendData2Serial " << "dispatch initError";
            auto data_r = boost::make_shared<OwlMailDefine::Serial2Cmd>();
            data_r->runner = data->callbackRunner;
            data_r->openError = true;
            data_r->ok = true;
            sendMail(std::move(data_r), mailbox);
            return;
        }
        // BOOST_ASSERT(data->additionCmd != OwlMailDefine::AdditionCmd::ignore);
        switch (data->additionCmd) {
            case OwlMailDefine::AdditionCmd::ignore: {
                BOOST_LOG_OWL(error) << "SerialController"
                                     << " sendData2Serial"
                                     << " switch (data->additionCmd) OwlMailDefine::AdditionCmd::ignore";
                auto data_r = boost::make_shared<OwlMailDefine::Serial2Cmd>();
                data_r->runner = data->callbackRunner;
                data_r->openError = true;
                data_r->ok = false;
                sendMail(std::move(data_r), mailbox);
                return;
            }
            case OwlMailDefine::AdditionCmd::takeoff:
            case OwlMailDefine::AdditionCmd::land:
            case OwlMailDefine::AdditionCmd::stop:
            case OwlMailDefine::AdditionCmd::move:
            case OwlMailDefine::AdditionCmd::rotate:
            case OwlMailDefine::AdditionCmd::keep:
            case OwlMailDefine::AdditionCmd::high:
            case OwlMailDefine::AdditionCmd::speed:
            case OwlMailDefine::AdditionCmd::flyMode:
            case OwlMailDefine::AdditionCmd::gotoPosition:
            case OwlMailDefine::AdditionCmd::led: {
                if (!repeating)
                    BOOST_LOG_OWL(trace_cmd_sp_w)
                        << "SerialController"
                        << " sendData2Serial"
                        << " switch (data->additionCmd) OwlMailDefine::AdditionCmd::* move class";
                auto mcp = data->moveCmdPtr;
                if (!mcp) {
                    BOOST_LOG_OWL(error) << "SerialController"
                                         << " sendData2Serial"
                                         << " OwlMailDefine::AdditionCmd::* move class nullptr";
                    auto data_r = boost::make_shared<OwlMailDefine::Serial2Cmd>();
                    data_r->runner = data->callbackRunner;
                    data_r->openError = false;
                    data_r->ok = false;
                    sendMail(std::move(data_r), mailbox);
                    return;
                }
                if (needRepeat) {
                    // ok, this package is safe, now to remember this package

                    // clone a package without callback to recorder
                    packageId = package_record_->setNewMail(data->repeat());
                }
                constexpr uint8_t packageSize = 15;
                makeADataBuffer<packageSize>(
                        std::array<uint8_t, packageSize>{
                                // 0xAA
                                uint8_t(0xAA),

                                // AdditionCmd
                                uint8_t(to_underlying(data->additionCmd)),
                                // data size
                                uint8_t(packageSize - 5),

                                uint8_t(uint16_t(mcp->x) & 0xff),
                                uint8_t(uint16_t(mcp->x) >> 8),

                                uint8_t(uint16_t(mcp->y) & 0xff),
                                uint8_t(uint16_t(mcp->y) >> 8),

                                uint8_t(uint16_t(mcp->z) & 0xff),
                                uint8_t(uint16_t(mcp->z) >> 8),

                                uint8_t(uint16_t(mcp->cw) & 0xff),
                                uint8_t(uint16_t(mcp->cw) >> 8),

                                // serial ID
                                uint8_t(packageId & 0xff),
                                uint8_t(packageId >> 8),
                                // xor checksum byte
                                uint8_t(0),
                                // 0xBB
                                uint8_t(0xBB),
                        },
                        shared_from_this(),
                        data,
                        mailbox
                );
                return;
            }
            case OwlMailDefine::AdditionCmd::AprilTag: {
                if (!repeating)
                    BOOST_LOG_OWL(trace_cmd_sp_w) << "SerialController"
                                                  << " sendData2Serial"
                                                  << " switch (data->additionCmd) OwlMailDefine::AdditionCmd::AprilTag";

                if (!data->aprilTagCmdPtr) {
                    BOOST_LOG_OWL(error) << "SerialController"
                                         << " sendData2Serial"
                                         << " switch (data->additionCmd) OwlMailDefine::AdditionCmd::AprilTag"
                                         << " (!data->aprilTagCmdPtr)";
                    auto data_r = boost::make_shared<OwlMailDefine::Serial2Cmd>();
                    data_r->runner = data->callbackRunner;
                    data_r->openError = false;
                    data_r->ok = false;
                    sendMail(std::move(data_r), mailbox);
                    return;
                }
                BOOST_ASSERT(data->aprilTagCmdPtr);

                // auto pcx = static_cast<uint16_t>(data->aprilTagCmdPtr->imageX);
                // auto pcy = static_cast<uint16_t>(data->aprilTagCmdPtr->imageY);

                if (!data->aprilTagCmdPtr->mapCalcPlaneInfoType) {
                    BOOST_LOG_OWL(error)
                        << "SerialController"
                        << " sendData2Serial"
                        << " switch (data->additionCmd) OwlMailDefine::AdditionCmd::AprilTag"
                        << " (!data->aprilTagCmdPtr->mapCalcPlaneInfoType)";
                    auto data_r = boost::make_shared<OwlMailDefine::Serial2Cmd>();
                    data_r->runner = data->callbackRunner;
                    data_r->openError = false;
                    data_r->ok = false;
                    sendMail(std::move(data_r), mailbox);
                    return;
                }
                BOOST_ASSERT(data->aprilTagCmdPtr->mapCalcPlaneInfoType);


                if (!repeating) {
                    // save a copy for other use
                    atomic_store(&aprilTagCmdData, data->aprilTagCmdPtr);
                }

                auto &info = *data->aprilTagCmdPtr->mapCalcPlaneInfoType;

                auto xDirectDeg = int16_t(std::round(info.xDirectDeg));
                auto zDirectDeg = int16_t(std::round(info.zDirectDeg));
                auto xzDirectDeg = int16_t(std::round(info.xzDirectDeg));
                // auto ImageP_x = int16_t(std::round(info.ImageP.x));
                // auto ImageP_y = int16_t(std::round(info.ImageP.y));
                auto PlaneP_x = int16_t(std::round(info.PlaneP.x));
                auto PlaneP_y = int16_t(std::round(info.PlaneP.y));
                // auto ScaleXY_x = int16_t(std::round(info.ScaleXY.x));
                // auto ScaleXY_y = int16_t(std::round(info.ScaleXY.y));
                // auto ScaleXZ_x = int16_t(std::round(info.ScaleXZ.x));
                // auto ScaleXZ_y = int16_t(std::round(info.ScaleXZ.y));

                // if (needRepeat) {
                //     // ok, this package is safe, now to remember this package
                //
                //     // clone a package without callback to recorder
                //     packageId = package_record_->setNewMail(data->repeat());
                // }

                BOOST_LOG_OWL(trace_sp_tag)
                    << "SerialController AprilTag Write:"
                    << "[x:" << PlaneP_x
                    << ",y:" << PlaneP_y
                    << ",xD:" << xDirectDeg
                    << ",zD:" << zDirectDeg
                    << ",xzD:" << xzDirectDeg
                    << "]";

                // send cmd to serial
                // make send data
                constexpr uint8_t packageSize = 17;
                // constexpr uint8_t packageSize = 33;
                makeADataBuffer<packageSize>(
                        std::array<uint8_t, packageSize>{
                                // 0xAA
                                uint8_t(0xAA),

                                // AdditionCmd
                                uint8_t(to_underlying(data->additionCmd)),
                                // data size
                                uint8_t(packageSize - 5),

                                // // image size
                                // uint8_t(uint16_t(pcx) & 0xff),
                                // uint8_t(uint16_t(pcx) >> 8),
                                // uint8_t(uint16_t(pcy) & 0xff),
                                // uint8_t(uint16_t(pcy) >> 8),

                                // https://stackoverflow.com/questions/2711522/what-happens-if-i-assign-a-negative-value-to-an-unsigned-variable

                                uint8_t(uint16_t(xDirectDeg & 0xff)),
                                uint8_t(uint16_t(xDirectDeg & 0xff00) >> 8),

                                uint8_t(uint16_t(zDirectDeg & 0xff)),
                                uint8_t(uint16_t(zDirectDeg & 0xff00) >> 8),

                                uint8_t(uint16_t(xzDirectDeg & 0xff)),
                                uint8_t(uint16_t(xzDirectDeg & 0xff00) >> 8),

                                // uint8_t(uint16_t(ImageP_x & 0xff)),
                                // uint8_t(uint16_t(ImageP_x & 0xff00) >> 8),
                                // uint8_t(uint16_t(ImageP_y & 0xff)),
                                // uint8_t(uint16_t(ImageP_y & 0xff00) >> 8),

                                uint8_t(uint16_t(PlaneP_x & 0xff)),
                                uint8_t(uint16_t(PlaneP_x & 0xff00) >> 8),
                                uint8_t(uint16_t(PlaneP_y & 0xff)),
                                uint8_t(uint16_t(PlaneP_y & 0xff00) >> 8),

                                // uint8_t(uint16_t(ScaleXY_x & 0xff)),
                                // uint8_t(uint16_t(ScaleXY_x & 0xff00) >> 8),
                                // uint8_t(uint16_t(ScaleXY_y & 0xff)),
                                // uint8_t(uint16_t(ScaleXY_y & 0xff00) >> 8),
                                //
                                // uint8_t(uint16_t(ScaleXZ_x & 0xff)),
                                // uint8_t(uint16_t(ScaleXZ_x & 0xff00) >> 8),
                                // uint8_t(uint16_t(ScaleXZ_y & 0xff)),
                                // uint8_t(uint16_t(ScaleXZ_y & 0xff00) >> 8),


                                // serial ID
                                uint8_t(0),
                                uint8_t(0),
                                // xor checksum byte
                                uint8_t(0),
                                // 0xBB
                                uint8_t(0xBB),
                        },
                        shared_from_this(),
                        data,
                        mailbox
                );
                return;
            }
            case OwlMailDefine::AdditionCmd::JoyCon: {
                if (!repeating)
                    BOOST_LOG_OWL(trace_cmd_sp_w) << "SerialController"
                                                  << " sendData2Serial"
                                                  << " switch (data->additionCmd) OwlMailDefine::AdditionCmd::JoyCon";
                auto jcp = data->joyConPtr;
                if (!jcp) {
                    BOOST_LOG_OWL(error) << "SerialController"
                                         << " sendData2Serial"
                                         << " OwlMailDefine::AdditionCmd::JoyCon nullptr";
                    auto data_r = boost::make_shared<OwlMailDefine::Serial2Cmd>();
                    data_r->runner = data->callbackRunner;
                    data_r->openError = false;
                    data_r->ok = false;
                    sendMail(std::move(data_r), mailbox);
                    return;
                }
                BOOST_ASSERT(jcp);
                // if (needRepeat) {
                //     // ok, this package is safe, now to remember this package
                //
                //     // clone a package without callback to recorder
                //     packageId = package_record_->setNewMail(data->repeat());
                // }
                constexpr uint8_t packageSize = 29;
                makeADataBuffer<packageSize>(
                        std::array<uint8_t, packageSize>{
                                // 0xAA
                                uint8_t(0xAA),

                                // AdditionCmd
                                uint8_t(to_underlying(data->additionCmd)),
                                // data size
                                uint8_t(packageSize - 5),

                                uint8_t(jcp->leftRockerX > 0 ? jcp->leftRockerX : 0),
                                uint8_t(jcp->leftRockerX < 0 ? -jcp->leftRockerX : 0),
                                uint8_t(jcp->leftRockerY > 0 ? jcp->leftRockerY : 0),
                                uint8_t(jcp->leftRockerY < 0 ? -jcp->leftRockerY : 0),
                                uint8_t(jcp->rightRockerX > 0 ? jcp->rightRockerX : 0),
                                uint8_t(jcp->rightRockerX < 0 ? -jcp->rightRockerX : 0),
                                uint8_t(jcp->rightRockerY > 0 ? jcp->rightRockerY : 0),
                                uint8_t(jcp->rightRockerY < 0 ? -jcp->rightRockerY : 0),

                                uint8_t(jcp->leftBackTop),
                                uint8_t(jcp->leftBackBottom),
                                uint8_t(jcp->rightBackTop),
                                uint8_t(jcp->rightBackBottom),

                                uint8_t(jcp->CrossUp),
                                uint8_t(jcp->CrossDown),
                                uint8_t(jcp->CrossLeft),
                                uint8_t(jcp->CrossRight),

                                uint8_t(jcp->A),
                                uint8_t(jcp->B),
                                uint8_t(jcp->X),
                                uint8_t(jcp->Y),

                                uint8_t(jcp->buttonAdd),
                                uint8_t(jcp->buttonReduce),

                                // serial ID
                                uint8_t(0),
                                uint8_t(0),
                                // xor checksum byte
                                uint8_t(0),
                                // 0xBB
                                uint8_t(0xBB),
                        },
                        shared_from_this(),
                        data,
                        mailbox
                );
                return;
            }
            case OwlMailDefine::AdditionCmd::JoyConSimple: {
                if (!repeating)
                    BOOST_LOG_OWL(trace_cmd_sp_w) << "SerialController"
                                                  << " sendData2Serial"
                                                  << " switch (data->additionCmd) OwlMailDefine::AdditionCmd::JoyConSimple";
                auto jcp = data->joyConPtr;
                if (!jcp) {
                    BOOST_LOG_OWL(error) << "SerialController"
                                         << " sendData2Serial"
                                         << " OwlMailDefine::AdditionCmd::JoyConSimple nullptr";
                    auto data_r = boost::make_shared<OwlMailDefine::Serial2Cmd>();
                    data_r->runner = data->callbackRunner;
                    data_r->openError = false;
                    data_r->ok = false;
                    sendMail(std::move(data_r), mailbox);
                    return;
                }
                BOOST_ASSERT(jcp);
                // if (needRepeat) {
                //     // ok, this package is safe, now to remember this package
                //
                //     // clone a package without callback to recorder
                //     packageId = package_record_->setNewMail(data->repeat());
                // }
                constexpr uint8_t packageSize = 21;
                makeADataBuffer<packageSize>(
                        std::array<uint8_t, packageSize>{
                                // 0xAA
                                uint8_t(0xAA),

                                // AdditionCmd
                                uint8_t(to_underlying(data->additionCmd)),
                                // data size
                                uint8_t(packageSize - 5),

                                uint8_t(jcp->leftRockerX > 0 ? jcp->leftRockerX : 0),
                                uint8_t(jcp->leftRockerX < 0 ? -jcp->leftRockerX : 0),
                                uint8_t(jcp->leftRockerY > 0 ? jcp->leftRockerY : 0),
                                uint8_t(jcp->leftRockerY < 0 ? -jcp->leftRockerY : 0),
                                uint8_t(jcp->rightRockerX > 0 ? jcp->rightRockerX : 0),
                                uint8_t(jcp->rightRockerX < 0 ? -jcp->rightRockerX : 0),
                                uint8_t(jcp->rightRockerY > 0 ? jcp->rightRockerY : 0),
                                uint8_t(jcp->rightRockerY < 0 ? -jcp->rightRockerY : 0),

                                uint8_t(jcp->leftBackTop),
                                uint8_t(jcp->leftBackBottom),
                                uint8_t(jcp->rightBackTop),
                                uint8_t(jcp->rightBackBottom),

                                uint8_t(jcp->buttonAdd),
                                uint8_t(jcp->buttonReduce),

                                // serial ID
                                uint8_t(0),
                                uint8_t(0),
                                // xor checksum byte
                                uint8_t(0),
                                // 0xBB
                                uint8_t(0xBB),
                        },
                        shared_from_this(),
                        data,
                        mailbox
                );
                return;
            }
            case OwlMailDefine::AdditionCmd::JoyConGyro: {
                if (!repeating)
                    BOOST_LOG_OWL(trace_cmd_sp_w) << "SerialController"
                                                  << " sendData2Serial"
                                                  << " switch (data->additionCmd) OwlMailDefine::AdditionCmd::JoyConGyro";
                auto jcgp = data->joyConPtr;
                if (!jcgp) {
                    BOOST_LOG_OWL(error) << "SerialController"
                                         << " sendData2Serial"
                                         << " OwlMailDefine::AdditionCmd::JoyConGyro nullptr";
                    auto data_r = boost::make_shared<OwlMailDefine::Serial2Cmd>();
                    data_r->runner = data->callbackRunner;
                    data_r->openError = false;
                    data_r->ok = false;
                    sendMail(std::move(data_r), mailbox);
                    return;
                }
                BOOST_ASSERT(jcgp);
                // if (needRepeat) {
                //     // ok, this package is safe, now to remember this package
                // 
                //     // clone a package without callback to recorder
                //     packageId = package_record_->setNewMail(data->repeat());
                // }
                constexpr uint8_t packageSize = 15;
                makeADataBuffer<packageSize>(
                        std::array<uint8_t, packageSize>{
                                // 0xAA
                                uint8_t(0xAA),

                                // AdditionCmd
                                uint8_t(to_underlying(data->additionCmd)),
                                // data size
                                uint8_t(packageSize - 5),

                                uint8_t(jcgp->A),
                                uint8_t(jcgp->B),
                                uint8_t(jcgp->X),
                                uint8_t(jcgp->Y),

                                uint8_t(jcgp->A),
                                uint8_t(jcgp->B),
                                uint8_t(jcgp->X),
                                uint8_t(jcgp->Y),

                                // serial ID
                                uint8_t(0),
                                uint8_t(0),
                                // xor checksum byte
                                uint8_t(0),
                                // 0xBB
                                uint8_t(0xBB),
                        },
                        shared_from_this(),
                        data,
                        mailbox
                );
                return;
            }
            case OwlMailDefine::AdditionCmd::emergencyStop:
            case OwlMailDefine::AdditionCmd::unlock:
            case OwlMailDefine::AdditionCmd::calibrate: {
                if (needRepeat) {
                    // ok, this package is safe, now to remember this package

                    // clone a package without callback to recorder
                    packageId = package_record_->setNewMail(data->repeat());
                }
                constexpr uint8_t packageSize = 7;
                makeADataBuffer<packageSize>(
                        std::array<uint8_t, packageSize>{
                                // 0xAA
                                uint8_t(0xAA),
                                // AdditionCmd
                                uint8_t(to_underlying(data->additionCmd)),
                                // data size
                                uint8_t(packageSize - 5),
                                // serial ID
                                uint8_t(packageId & 0xff),
                                uint8_t(packageId >> 8),
                                // xor checksum byte
                                uint8_t(0),
                                // 0xBB
                                uint8_t(0xBB),
                        },
                        shared_from_this(),
                        data,
                        mailbox
                );
                return;
            }
            case OwlMailDefine::AdditionCmd::ping: {
                // if (needRepeat) {
                //     // ok, this package is safe, now to remember this package
                //
                //     // clone a package without callback to recorder
                //     packageId = package_record_->setNewMail(data->repeat());
                // }
                constexpr uint8_t packageSize = 7;
                makeADataBuffer<packageSize>(
                        std::array<uint8_t, packageSize>{
                                // 0xAA
                                uint8_t(0xAA),
                                // AdditionCmd
                                uint8_t(to_underlying(data->additionCmd)),
                                // data size
                                uint8_t(2),
                                // serial ID
                                uint8_t(0),
                                uint8_t(0),
                                // xor checksum byte
                                uint8_t(0),
                                // 0xBB
                                uint8_t(0xBB),
                        },
                        shared_from_this(),
                        data,
                        mailbox
                );
                return;
            }
            default: {
                BOOST_LOG_OWL(error) << "SerialController"
                                     << " sendData2Serial"
                                     << " switch (data->additionCmd) default";
                auto data_r = boost::make_shared<OwlMailDefine::Serial2Cmd>();
                data_r->runner = data->callbackRunner;
                data_r->openError = true;
                data_r->ok = false;
                sendMail(std::move(data_r), mailbox);
                return;
            }
        }
    }


    bool PortController::open(const std::string &deviceName, boost::system::error_code &ec) {
        BOOST_ASSERT(sp_);
        BOOST_LOG_OWL(trace) << "PortController::open";
        if (sp_->is_open()) {
            close();
        }
        BOOST_LOG_OWL(trace) << "PortController::open do";
        deviceName_ = deviceName;
        sp_->open(deviceName, ec);
        if (ec) {
            BOOST_LOG_OWL(error) << "PortController open error: " << ec.what();
            return false;
        }
        BOOST_LOG_OWL(trace) << "PortController::open ok";
#ifndef DEBUG_DisableStateReader
        BOOST_LOG_OWL(trace) << "PortController::open start stateReader_";
        if constexpr (flag_DEBUG_IF_CHECK_POINT) {
            BOOST_ASSERT(!weak_from_this().expired());
            BOOST_LOG_OWL(trace) << "PortController::open parentRef_.use_count(): " << parentRef_.use_count();
            BOOST_ASSERT(!parentRef_.expired());
            BOOST_ASSERT(parentRef_.lock());
        }
        BOOST_ASSERT(stateReader_);
        BOOST_LOG_OWL(trace) << "PortController::open stateReader_.use_count(): " << stateReader_.use_count();
        stateReader_->start();
#endif // DEBUG_DisableStateReader
        BOOST_LOG_OWL(trace) << "PortController::open true";
        return true;
    }


    PortController::PortController(
            boost::asio::io_context &ioc,
            boost::weak_ptr<SerialController> &&parentRef)
            : sp_(boost::make_shared<boost::asio::serial_port>(boost::asio::make_strand(ioc))),
              parentRef_(std::move(parentRef)) {
    }

    void PortController::init() {
        if constexpr (flag_DEBUG_IF_CHECK_POINT) {
            BOOST_LOG_OWL(trace) << "PortController::open parentRef_.use_count(): " << parentRef_.use_count();
            BOOST_ASSERT(!parentRef_.expired());
            BOOST_ASSERT(parentRef_.lock());
            BOOST_ASSERT(!weak_from_this().expired());
            BOOST_LOG_OWL(trace) << "weak_from_this().lock().use_count() : " << weak_from_this().lock().use_count();
        }
#ifndef DEBUG_DisableStateReader
        BOOST_ASSERT(!weak_from_this().expired());
        stateReader_ = boost::make_shared<StateReader>(weak_from_this(), sp_);
        BOOST_ASSERT(!weak_from_this().expired());
        BOOST_LOG_OWL(trace) << "stateReader_.use_count() : " << stateReader_.use_count();
        BOOST_ASSERT(stateReader_.use_count() > 0);
        stateReader_->init();
#endif // DEBUG_DisableStateReader
    }

    void PortController::sendAirplaneState(const boost::shared_ptr<AirplaneState> &airplaneState) {
        boost::asio::dispatch(sp_->get_executor(), [
                this, self = shared_from_this(), airplaneState]() {
            auto ptr = parentRef_.lock();
            if (!ptr) {
                BOOST_LOG_OWL(error) << "PortController"
                                     << " parentRef_.lock() failed.";
                return;
            }
            BOOST_ASSERT(ptr);
            ptr->sendAirplaneState(airplaneState);
        });
    }

    void SerialController::sendAirplaneState(const boost::shared_ptr<AirplaneState> &airplaneState) {
        boost::asio::dispatch(ioc_, [this, self = shared_from_this(), airplaneState]() {
            auto p = airplaneState;
            if (p) {
                BOOST_ASSERT(p);
                BOOST_LOG_OWL(trace_cmd_sp_r)
                    << "new airplaneState come:"
                    << "\n\tstateFly: " << p->stateFly
                    << "\n\tpitch: " << p->pitch
                    << "\n\troll: " << p->roll
                    << "\n\tyaw: " << p->yaw
                    << "\n\tvx: " << p->vx
                    << "\n\tvy: " << p->vy
                    << "\n\tvz: " << p->vz
                    << "\n\thigh: " << p->high
                    << "\n\tvoltage: " << p->voltage
                    << "\n\ttimestamp: " << p->timestamp
                    << "";
            }
            atomic_store(&newestAirplaneState, p);
        });
    }

} // OwlSerialController
