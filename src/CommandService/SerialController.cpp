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
        BOOST_LOG_TRIVIAL(trace) << "SerialController::initPort";
        if constexpr (flag_DEBUG_IF_CHECK_POINT) {
            BOOST_ASSERT(!weak_from_this().expired());
        }
        BOOST_ASSERT(airplanePortController);
        BOOST_LOG_TRIVIAL(trace) << "SerialController::initPort close";
        airplanePortController->close();
        // set and open the airplanePortController
        BOOST_LOG_TRIVIAL(trace) << "SerialController::initPort open";
        BOOST_LOG_TRIVIAL(trace) << "config_->config().airplane_fly_serial_addr "
                                 << config_->config().airplane_fly_serial_addr;
        if constexpr (flag_DEBUG_IF_CHECK_POINT) {
            BOOST_ASSERT(!weak_from_this().expired());
            BOOST_LOG_TRIVIAL(trace) << "SerialController::initPort weak_from_this().use_count(): "
                                     << weak_from_this().use_count();
            BOOST_LOG_TRIVIAL(trace) << "SerialController::initPort shared_from_this().use_count(): "
                                     << this->shared_from_this().use_count();
        }
        initOk = airplanePortController->open(config_->config().airplane_fly_serial_addr);
        BOOST_LOG_TRIVIAL(trace) << "SerialController::initPort open " << initOk;
        if (initOk) {
            initOk = airplanePortController->set_option(
                    boost::asio::serial_port::baud_rate(config_->config().airplane_fly_serial_baud_rate)
            );
            BOOST_LOG_TRIVIAL(trace) << "SerialController::initPort set_option " << initOk;
        }
        BOOST_LOG_TRIVIAL(trace) << "SerialController::initPort initOk " << initOk;
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
                auto p = atomic_load(&newestAirplaneState);
                if (!p) {
                    auto data_r = std::make_shared<OwlMailDefine::Serial2Cmd>();
                    data_r->runner = data->callbackRunner;
                    data_r->openError = true;
                    data_r->ok = false;
                    sendMail(std::move(data_r), mailbox);
                    return;
                }
                auto data_r = std::make_shared<OwlMailDefine::Serial2Cmd>();
                data_r->runner = data->callbackRunner;
                data_r->ok = p.operator bool();
                data_r->newestAirplaneState = p->shared_from_this();
                sendMail(std::move(data_r), mailbox);
                return;
            }
            default: {
                BOOST_LOG_TRIVIAL(error) << "SerialController"
                                         << " receiveMailGetAirplaneState"
                                         << " switch (data->additionCmd) default";
                auto data_r = std::make_shared<OwlMailDefine::Serial2Cmd>();
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
            std::shared_ptr<SerialController> selfPtr,
            std::shared_ptr<std::array<uint8_t, packageSize>> sendDataBuffer,
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
                    auto data_r = std::make_shared<OwlMailDefine::Serial2Cmd>();
                    data_r->runner = data->callbackRunner;
                    if (ec) {
                        // error
                        BOOST_LOG_TRIVIAL(error) << "SerialController"
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
            std::shared_ptr<SerialController> selfPtr,
            OwlMailDefine::MailCmd2Serial data,
            OwlMailDefine::CmdSerialMailbox &mailbox
    ) {
        BOOST_ASSERT(selfPtr);
        BOOST_ASSERT(data);
        auto sendDataBuffer = std::make_shared<std::array<uint8_t, packageSize>>(dataInitList);
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

    void SerialController::receiveMail(OwlMailDefine::MailCmd2Serial &&data, OwlMailDefine::CmdSerialMailbox &mailbox) {
        BOOST_ASSERT(data);
        BOOST_LOG_TRIVIAL(trace) << "SerialController::receiveMail "
                                 << to_underlying(data->additionCmd) << " additionCmd:"
                                 << OwlMailDefine::AdditionCmdNameLookupTable.at(data->additionCmd);
        if (data->additionCmd == OwlMailDefine::AdditionCmd::getAirplaneState) {
            receiveMailGetAirplaneState(std::move(data), mailbox);
            return;
        }
        if constexpr (flag_DEBUG_IF_CHECK_POINT) {
            BOOST_ASSERT(!weak_from_this().expired());
        }
        boost::asio::dispatch(ioc_, [
                this, self = shared_from_this(), data, &mailbox]() {
            BOOST_ASSERT(data);
            BOOST_LOG_TRIVIAL(trace) << "SerialController::receiveMail " << "dispatch ";
            if constexpr (flag_DEBUG_IF_CHECK_POINT) {
                BOOST_ASSERT(!weak_from_this().expired());
                BOOST_ASSERT(self.use_count() > 0);
            }
            if (!initOk && !initPort()) {
                BOOST_LOG_TRIVIAL(trace) << "SerialController::receiveMail " << "dispatch initError";
                auto data_r = std::make_shared<OwlMailDefine::Serial2Cmd>();
                data_r->runner = data->callbackRunner;
                data_r->openError = true;
                data_r->ok = true;
                sendMail(std::move(data_r), mailbox);
                return;
            }
            BOOST_LOG_TRIVIAL(trace) << "SerialController::receiveMail " << "dispatch initOk";
            switch (data->additionCmd) {
                case OwlMailDefine::AdditionCmd::ignore: {
                    BOOST_LOG_TRIVIAL(error) << "SerialController"
                                             << " receiveMail"
                                             << " switch (data->additionCmd) OwlMailDefine::AdditionCmd::ignore";
                    auto data_r = std::make_shared<OwlMailDefine::Serial2Cmd>();
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
                    BOOST_LOG_TRIVIAL(trace) << "SerialController"
                                             << " receiveMail"
                                             << " switch (data->additionCmd) OwlMailDefine::AdditionCmd::* move class";
                    auto mcp = data->moveCmdPtr;
                    if (!mcp) {
                        BOOST_LOG_TRIVIAL(error) << "SerialController"
                                                 << " receiveMail"
                                                 << " OwlMailDefine::AdditionCmd::* move class nullptr";
                        auto data_r = std::make_shared<OwlMailDefine::Serial2Cmd>();
                        data_r->runner = data->callbackRunner;
                        data_r->openError = false;
                        data_r->ok = false;
                        sendMail(std::move(data_r), mailbox);
                        return;
                    }
                    constexpr uint8_t packageSize = 12;
                    makeADataBuffer<packageSize>(
                            std::array<uint8_t, packageSize>{
                                    // 0xAA
                                    uint8_t(0xAA),

                                    // AdditionCmd
                                    uint8_t(to_underlying(data->additionCmd)),
                                    // data size
                                    uint8_t(packageSize - 4),

                                    uint8_t(uint16_t(mcp->x) & 0xff),
                                    uint8_t(uint16_t(mcp->x) >> 8),

                                    uint8_t(uint16_t(mcp->y) & 0xff),
                                    uint8_t(uint16_t(mcp->y) >> 8),

                                    uint8_t(uint16_t(mcp->z) & 0xff),
                                    uint8_t(uint16_t(mcp->z) >> 8),

                                    uint8_t(uint16_t(mcp->cw) & 0xff),
                                    uint8_t(uint16_t(mcp->cw) >> 8),

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
                    BOOST_LOG_TRIVIAL(trace) << "SerialController"
                                             << " receiveMail"
                                             << " switch (data->additionCmd) OwlMailDefine::AdditionCmd::AprilTag";

                    if (!data->aprilTagCmdPtr) {
                        BOOST_LOG_TRIVIAL(error) << "SerialController"
                                                 << " receiveMail"
                                                 << " switch (data->additionCmd) OwlMailDefine::AdditionCmd::AprilTag"
                                                 << " (!data->aprilTagCmdPtr)";
                        return;
                    }
                    BOOST_ASSERT(data->aprilTagCmdPtr);

                    auto pcx = static_cast<uint16_t>(data->aprilTagCmdPtr->imageX);
                    auto pcy = static_cast<uint16_t>(data->aprilTagCmdPtr->imageY);

                    auto center = data->aprilTagCmdPtr->aprilTagCenter;
                    if (!center) {
                        BOOST_LOG_TRIVIAL(error) << "SerialController"
                                                 << " receiveMail"
                                                 << " switch (data->additionCmd) OwlMailDefine::AdditionCmd::AprilTag"
                                                 << " (!center)";
                        return;
                    }
                    BOOST_ASSERT(center);
                    auto x = center->cornerLTx - center->cornerLBx;
                    auto y = center->cornerLTy - center->cornerLBy;
                    auto r = atan2(y, x);
                    // rad(+PI ~ -PI) -> degree(+180 ~ -180) -> degree(+360 ~ 0)
                    auto d = static_cast<uint16_t>(((r / (M_PI / 180.0)) + 180));

                    auto id = static_cast<uint16_t>(center->id);
                    auto cx = static_cast<uint16_t>(center->centerX);
                    auto cy = static_cast<uint16_t>(center->centerY);

                    // send cmd to serial
                    // make send data
                    constexpr uint8_t packageSize = 16;
                    makeADataBuffer<packageSize>(
                            std::array<uint8_t, packageSize>{
                                    // 0xAA
                                    uint8_t(0xAA),

                                    // AdditionCmd
                                    uint8_t(to_underlying(data->additionCmd)),
                                    // data size
                                    uint8_t(packageSize - 4),

                                    // image size
                                    uint8_t(uint16_t(pcx) & 0xff),
                                    uint8_t(uint16_t(pcx) >> 8),
                                    uint8_t(uint16_t(pcy) & 0xff),
                                    uint8_t(uint16_t(pcy) >> 8),

                                    // tag center
                                    uint8_t(uint16_t(cx) & 0xff),
                                    uint8_t(uint16_t(cx) >> 8),
                                    uint8_t(uint16_t(cy) & 0xff),
                                    uint8_t(uint16_t(cy) >> 8),

                                    // tag degree
                                    uint8_t(uint16_t(d) & 0xff),
                                    uint8_t(uint16_t(d) >> 8),

                                    // tag id
                                    uint8_t(uint16_t(id) & 0xff),
                                    uint8_t(uint16_t(id) >> 8),

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
                    BOOST_LOG_TRIVIAL(trace) << "SerialController"
                                             << " receiveMail"
                                             << " switch (data->additionCmd) OwlMailDefine::AdditionCmd::JoyCon";
                    auto jcp = data->joyConPtr;
                    if (!jcp) {
                        BOOST_LOG_TRIVIAL(error) << "SerialController"
                                                 << " receiveMail"
                                                 << " OwlMailDefine::AdditionCmd::JoyCon nullptr";
                        auto data_r = std::make_shared<OwlMailDefine::Serial2Cmd>();
                        data_r->runner = data->callbackRunner;
                        data_r->openError = false;
                        data_r->ok = false;
                        sendMail(std::move(data_r), mailbox);
                        return;
                    }
                    BOOST_ASSERT(jcp);
                    constexpr uint8_t packageSize = 26;
                    makeADataBuffer<packageSize>(
                            std::array<uint8_t, packageSize>{
                                    // 0xAA
                                    uint8_t(0xAA),

                                    // AdditionCmd
                                    uint8_t(to_underlying(data->additionCmd)),
                                    // data size
                                    uint8_t(packageSize - 4),

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
                    BOOST_LOG_TRIVIAL(trace) << "SerialController"
                                             << " receiveMail"
                                             << " switch (data->additionCmd) OwlMailDefine::AdditionCmd::JoyConSimple";
                    auto jcp = data->joyConPtr;
                    if (!jcp) {
                        BOOST_LOG_TRIVIAL(error) << "SerialController"
                                                 << " receiveMail"
                                                 << " OwlMailDefine::AdditionCmd::JoyConSimple nullptr";
                        auto data_r = std::make_shared<OwlMailDefine::Serial2Cmd>();
                        data_r->runner = data->callbackRunner;
                        data_r->openError = false;
                        data_r->ok = false;
                        sendMail(std::move(data_r), mailbox);
                        return;
                    }
                    BOOST_ASSERT(jcp);
                    constexpr uint8_t packageSize = 18;
                    makeADataBuffer<packageSize>(
                            std::array<uint8_t, packageSize>{
                                    // 0xAA
                                    uint8_t(0xAA),

                                    // AdditionCmd
                                    uint8_t(to_underlying(data->additionCmd)),
                                    // data size
                                    uint8_t(packageSize - 4),

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
                    BOOST_LOG_TRIVIAL(trace) << "SerialController"
                                             << " receiveMail"
                                             << " switch (data->additionCmd) OwlMailDefine::AdditionCmd::JoyConGyro";
                    auto jcgp = data->joyConPtr;
                    if (!jcgp) {
                        BOOST_LOG_TRIVIAL(error) << "SerialController"
                                                 << " receiveMail"
                                                 << " OwlMailDefine::AdditionCmd::JoyConGyro nullptr";
                        auto data_r = std::make_shared<OwlMailDefine::Serial2Cmd>();
                        data_r->runner = data->callbackRunner;
                        data_r->openError = false;
                        data_r->ok = false;
                        sendMail(std::move(data_r), mailbox);
                        return;
                    }
                    BOOST_ASSERT(jcgp);
                    constexpr uint8_t packageSize = 12;
                    makeADataBuffer<packageSize>(
                            std::array<uint8_t, packageSize>{
                                    // 0xAA
                                    uint8_t(0xAA),

                                    // AdditionCmd
                                    uint8_t(to_underlying(data->additionCmd)),
                                    // data size
                                    uint8_t(packageSize - 4),

                                    uint8_t(jcgp->A),
                                    uint8_t(jcgp->B),
                                    uint8_t(jcgp->X),
                                    uint8_t(jcgp->Y),

                                    uint8_t(jcgp->A),
                                    uint8_t(jcgp->B),
                                    uint8_t(jcgp->X),
                                    uint8_t(jcgp->Y),

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
                    constexpr uint8_t packageSize = 3;
                    makeADataBuffer<packageSize>(
                            std::array<uint8_t, packageSize>{
                                    // 0xAA
                                    uint8_t(0xAA),
                                    // AdditionCmd
                                    uint8_t(to_underlying(data->additionCmd)),
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
                    BOOST_LOG_TRIVIAL(error) << "SerialController"
                                             << " receiveMail"
                                             << " switch (data->additionCmd) default";
                    auto data_r = std::make_shared<OwlMailDefine::Serial2Cmd>();
                    data_r->runner = data->callbackRunner;
                    data_r->openError = true;
                    data_r->ok = false;
                    sendMail(std::move(data_r), mailbox);
                    return;
                }
            }
        });
    }


    bool PortController::open(const std::string &deviceName, boost::system::error_code &ec) {
        BOOST_ASSERT(sp_);
        BOOST_LOG_TRIVIAL(trace) << "PortController::open";
        if (sp_->is_open()) {
            close();
        }
        BOOST_LOG_TRIVIAL(trace) << "PortController::open do";
        deviceName_ = deviceName;
        sp_->open(deviceName, ec);
        if (ec) {
            BOOST_LOG_TRIVIAL(error) << "PortController open error: " << ec.what();
            return false;
        }
        BOOST_LOG_TRIVIAL(trace) << "PortController::open ok";
#ifndef DEBUG_DisableStateReader
        BOOST_LOG_TRIVIAL(trace) << "PortController::open start stateReader_";
        if constexpr (flag_DEBUG_IF_CHECK_POINT) {
            BOOST_ASSERT(!weak_from_this().expired());
            BOOST_LOG_TRIVIAL(trace) << "PortController::open parentRef_.use_count(): " << parentRef_.use_count();
            BOOST_ASSERT(!parentRef_.expired());
            BOOST_ASSERT(parentRef_.lock());
        }
        BOOST_ASSERT(stateReader_);
        BOOST_LOG_TRIVIAL(trace) << "PortController::open stateReader_.use_count(): " << stateReader_.use_count();
        stateReader_->start();
#endif // DEBUG_DisableStateReader
        BOOST_LOG_TRIVIAL(trace) << "PortController::open true";
        return true;
    }


    PortController::PortController(
            boost::asio::io_context &ioc,
            std::weak_ptr<SerialController> &&parentRef)
            : sp_(std::make_shared<boost::asio::serial_port>(boost::asio::make_strand(ioc))),
              parentRef_(std::move(parentRef)) {
    }

    void PortController::init() {
        if constexpr (flag_DEBUG_IF_CHECK_POINT) {
            BOOST_LOG_TRIVIAL(trace) << "PortController::open parentRef_.use_count(): " << parentRef_.use_count();
            BOOST_ASSERT(!parentRef_.expired());
            BOOST_ASSERT(parentRef_.lock());
            BOOST_ASSERT(!weak_from_this().expired());
            BOOST_LOG_TRIVIAL(trace) << "weak_from_this().lock().use_count() : " << weak_from_this().lock().use_count();
        }
#ifndef DEBUG_DisableStateReader
        BOOST_ASSERT(!weak_from_this().expired());
        stateReader_ = std::make_shared<StateReader>(weak_from_this(), sp_);
        BOOST_ASSERT(!weak_from_this().expired());
        BOOST_LOG_TRIVIAL(trace) << "stateReader_.use_count() : " << stateReader_.use_count();
        BOOST_ASSERT(stateReader_.use_count() > 0);
        stateReader_->init();
#endif // DEBUG_DisableStateReader
    }

    void PortController::sendAirplaneState(const std::shared_ptr<AirplaneState> &airplaneState) {
        boost::asio::dispatch(sp_->get_executor(), [
                this, self = shared_from_this(), airplaneState]() {
            auto ptr = parentRef_.lock();
            if (!ptr) {
                BOOST_LOG_TRIVIAL(error) << "PortController"
                                         << " parentRef_.lock() failed.";
                return;
            }
            BOOST_ASSERT(ptr);
            ptr->sendAirplaneState(airplaneState);
        });
    }

    void SerialController::sendAirplaneState(const std::shared_ptr<AirplaneState> &airplaneState) {
        boost::asio::dispatch(ioc_, [this, self = shared_from_this(), airplaneState]() {
            auto p = airplaneState;
            if (p) {
                BOOST_ASSERT(p);
                BOOST_LOG_TRIVIAL(trace)
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
            std::atomic_store(&newestAirplaneState, p);
        });
    }

} // OwlSerialController
