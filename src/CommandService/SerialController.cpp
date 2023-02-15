// jeremie

#include "SerialController.h"
#include "./StateReader.h"
#include <boost/asio/read_until.hpp>
#include <array>

namespace OwlSerialController {

    bool SerialController::initPort() {
        airplanePortController->close();
        // set and open the airplanePortController
        initOk = airplanePortController->open(config_->config().airplane_fly_serial_addr)
                 && airplanePortController->set_option(
                boost::asio::serial_port::baud_rate(config_->config().airplane_fly_serial_baud_rate)
        );
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
                auto data_r = std::make_shared<OwlMailDefine::Serial2Cmd>();
                data_r->runner = data->callbackRunner;
                data_r->ok = newestAirplaneState.operator bool();
                data_r->newestAirplaneState = newestAirplaneState->shared_from_this();
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
        // send it
        boost::asio::async_write(
                *(selfPtr->airplanePortController->sp_),
                boost::asio::buffer(*sendDataBuffer),
                boost::asio::transfer_exactly(sendDataBuffer->size()),
                [selfPtr, sendDataBuffer, data, &mailbox](
                        const boost::system::error_code &ec,
                        size_t bytes_transferred
                ) {
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

    void SerialController::receiveMail(OwlMailDefine::MailCmd2Serial &&data, OwlMailDefine::CmdSerialMailbox &mailbox) {
        if (data->additionCmd == OwlMailDefine::AdditionCmd::getAirplaneState) {
            receiveMailGetAirplaneState(std::move(data), mailbox);
            return;
        }
        boost::asio::dispatch(ioc_, [
                this, self = shared_from_this(), data, &mailbox]() {
            if (!initOk && !initPort()) {
                auto data_r = std::make_shared<OwlMailDefine::Serial2Cmd>();
                data_r->runner = data->callbackRunner;
                data_r->openError = true;
                data_r->ok = true;
                sendMail(std::move(data_r), mailbox);
                return;
            }
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
                case OwlMailDefine::AdditionCmd::speed: {
                    // send cmd to serial
                    // 0xAA,0xAdditionCmd,0xXXXX,0xYYYY,0xZZZZ,0xCWCW,0xBB
                    // make send data
                    constexpr uint8_t packageSize = 12;
                    auto sendDataBuffer = std::make_shared<std::array<uint8_t, packageSize>>(
                            std::array<uint8_t, packageSize>{
                                    // 0xAA
                                    uint8_t(0xAA),

                                    // AdditionCmd
                                    uint8_t(data->additionCmd),
                                    // package size
                                    uint8_t(packageSize),

                                    // 0xXX
                                    uint8_t(uint16_t(data->x) & 0xff),
                                    uint8_t(uint16_t(data->x) >> 8),

                                    // 0xYY
                                    uint8_t(uint16_t(data->y) & 0xff),
                                    uint8_t(uint16_t(data->y) >> 8),

                                    // 0xZZ
                                    uint8_t(uint16_t(data->z) & 0xff),
                                    uint8_t(uint16_t(data->z) >> 8),

                                    // 0xCW
                                    uint8_t(uint16_t(data->cw) & 0xff),
                                    uint8_t(uint16_t(data->cw) >> 8),

                                    // 0xBB
                                    uint8_t(0xBB),
                            }
                    );
                    // send it
                    sendADataBuffer<packageSize>(
                            shared_from_this(),
                            sendDataBuffer,
                            data,
                            mailbox
                    );
                    return;
                }
                case OwlMailDefine::AdditionCmd::flyMode: {
                    BOOST_LOG_TRIVIAL(warning) << "SerialController"
                                               << " receiveMail"
                                               << " switch (data->additionCmd) OwlMailDefine::AdditionCmd::flyMode"
                                               << " not impl";
                    // send cmd to serial
                    // make send data
                    constexpr uint8_t packageSize = 6;
                    auto sendDataBuffer = std::make_shared<std::array<uint8_t, packageSize>>(
                            std::array<uint8_t, packageSize>{
                                    // 0xAA
                                    uint8_t(0xAA),

                                    // AdditionCmd
                                    uint8_t(data->additionCmd),
                                    // package size
                                    uint8_t(packageSize),

                                    // flyMode
                                    uint8_t(uint16_t(data->x) & 0xff),
                                    uint8_t(uint16_t(data->x) >> 8),

                                    // 0xBB
                                    uint8_t(0xBB),
                            }
                    );
                    // send it
                    sendADataBuffer<packageSize>(
                            shared_from_this(),
                            sendDataBuffer,
                            data,
                            mailbox
                    );
                }
                    break;
                case OwlMailDefine::AdditionCmd::gotoPosition: {
                    BOOST_LOG_TRIVIAL(warning) << "SerialController"
                                               << " receiveMail"
                                               << " switch (data->additionCmd) OwlMailDefine::AdditionCmd::gotoPosition"
                                               << " not impl";
                    // send cmd to serial
                    // make send data
                    constexpr uint8_t packageSize = 10;
                    auto sendDataBuffer = std::make_shared<std::array<uint8_t, packageSize>>(
                            std::array<uint8_t, packageSize>{
                                    // 0xAA
                                    uint8_t(0xAA),

                                    // AdditionCmd
                                    uint8_t(data->additionCmd),
                                    // package size
                                    uint8_t(packageSize),

                                    // y -> map.forward
                                    uint8_t(uint16_t(data->x) & 0xff),
                                    uint8_t(uint16_t(data->x) >> 8),

                                    // h -> high
                                    uint8_t(uint16_t(data->y) & 0xff),
                                    uint8_t(uint16_t(data->y) >> 8),

                                    // x -> map.right
                                    uint8_t(uint16_t(data->z) & 0xff),
                                    uint8_t(uint16_t(data->z) >> 8),

                                    // 0xBB
                                    uint8_t(0xBB),
                            }
                    );
                    // send it
                    sendADataBuffer<packageSize>(
                            shared_from_this(),
                            sendDataBuffer,
                            data,
                            mailbox
                    );
                }
                    break;
                case OwlMailDefine::AdditionCmd::led: {
                    BOOST_LOG_TRIVIAL(warning) << "SerialController"
                                               << " receiveMail"
                                               << " switch (data->additionCmd) OwlMailDefine::AdditionCmd::led"
                                               << " not impl";
                    // send cmd to serial
                    // make send data
                    constexpr uint8_t packageSize = 12;
                    auto sendDataBuffer = std::make_shared<std::array<uint8_t, packageSize>>(
                            std::array<uint8_t, packageSize>{
                                    // 0xAA
                                    uint8_t(0xAA),

                                    // AdditionCmd
                                    uint8_t(data->additionCmd),
                                    // package size
                                    uint8_t(packageSize),

                                    // B
                                    uint8_t(uint16_t(data->x) & 0xff),
                                    uint8_t(uint16_t(data->x) >> 8),

                                    // G
                                    uint8_t(uint16_t(data->y) & 0xff),
                                    uint8_t(uint16_t(data->y) >> 8),

                                    // R
                                    uint8_t(uint16_t(data->z) & 0xff),
                                    uint8_t(uint16_t(data->z) >> 8),

                                    // ledMode
                                    uint8_t(uint16_t(data->cw) & 0xff),
                                    uint8_t(uint16_t(data->cw) >> 8),

                                    // 0xBB
                                    uint8_t(0xBB),
                            }
                    );
                    // send it
                    sendADataBuffer<packageSize>(
                            shared_from_this(),
                            sendDataBuffer,
                            data,
                            mailbox
                    );
                }
                    break;
                case OwlMailDefine::AdditionCmd::AprilTag: {
                    BOOST_LOG_TRIVIAL(warning) << "SerialController"
                                               << " receiveMail"
                                               << " switch (data->additionCmd) OwlMailDefine::AdditionCmd::AprilTag"
                                               << " not impl";

                    if (!data->aprilTagCmdPtr) {
                        BOOST_LOG_TRIVIAL(error) << "SerialController"
                                                 << " receiveMail"
                                                 << " switch (data->additionCmd) OwlMailDefine::AdditionCmd::AprilTag"
                                                 << " (!data->aprilTagCmdPtr)";
                        return;
                    }

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
                    constexpr uint8_t packageSize = 14;
                    auto sendDataBuffer = std::make_shared<std::array<uint8_t, packageSize>>(
                            std::array<uint8_t, packageSize>{
                                    // 0xAA
                                    uint8_t(0xAA),

                                    // AdditionCmd
                                    uint8_t(data->additionCmd),
                                    // package size
                                    uint8_t(packageSize),

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

                                    // tag id
                                    uint8_t(uint16_t(id) & 0xff),
                                    uint8_t(uint16_t(id) >> 8),

                                    // 0xBB
                                    uint8_t(0xBB),
                            }
                    );
                    // send it
                    sendADataBuffer<packageSize>(
                            shared_from_this(),
                            sendDataBuffer,
                            data,
                            mailbox
                    );
                }
                    break;
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
        if (sp_->is_open()) {
            close();
        }
        deviceName_ = deviceName;
        sp_->open(deviceName, ec);
        if (ec) {
            BOOST_LOG_TRIVIAL(error) << "PortController open error: " << ec.what();
            return false;
        }
        stateReader_->start();
        return true;
    }


    PortController::PortController(
            boost::asio::io_context &ioc,
            std::weak_ptr<SerialController> &&parentRef)
            : sp_(std::make_shared<boost::asio::serial_port>(boost::asio::make_strand(ioc))),
              parentRef_(std::move(parentRef)) {
        stateReader_ = std::make_shared<StateReader>(weak_from_this(), sp_);
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
            ptr->sendAirplaneState(airplaneState);
        });
    }

    void SerialController::sendAirplaneState(const std::shared_ptr<AirplaneState> &airplaneState) {
        boost::asio::dispatch(ioc_, [this, self = shared_from_this(), airplaneState]() {
            newestAirplaneState = airplaneState;
        });
    }

} // OwlSerialController
