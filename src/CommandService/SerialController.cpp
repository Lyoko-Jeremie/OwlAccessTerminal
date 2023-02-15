// jeremie

#include "SerialController.h"
#include "./StateReader.h"
#include <boost/asio/read_until.hpp>

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
                    auto sendDataString = std::make_shared<std::vector<uint8_t>>();
                    // 0xAA,0xAdditionCmd,0xXXXX,0xYYYY,0xZZZZ,0xCWCW,0xBB
                    sendDataString->resize(11);
                    // make send data [big-endian with 8bit(byte)]
                    // 0xAA
                    (*sendDataString).at(0) = char(0xAA);
                    // AdditionCmd
                    (*sendDataString).at(1) = uint8_t(data->additionCmd);
                    // 0xXX
                    (*sendDataString).at(2) = uint8_t(uint16_t(data->x) & 0xff);
                    (*sendDataString).at(3) = uint8_t(uint16_t(data->x) >> 8);
                    // 0xYY
                    (*sendDataString).at(4) = uint8_t(uint16_t(data->y) & 0xff);
                    (*sendDataString).at(5) = uint8_t(uint16_t(data->y) >> 8);
                    // 0xZZ
                    (*sendDataString).at(6) = uint8_t(uint16_t(data->z) & 0xff);
                    (*sendDataString).at(7) = uint8_t(uint16_t(data->z) >> 8);
                    // 0xCW
                    (*sendDataString).at(8) = uint8_t(uint16_t(data->cw) & 0xff);
                    (*sendDataString).at(9) = uint8_t(uint16_t(data->cw) >> 8);
                    // 0xBB
                    (*sendDataString).at(10) = char(0xBB);
                    // send it
                    boost::asio::async_write(
                            *(airplanePortController->sp_),
                            boost::asio::buffer(*sendDataString),
                            boost::asio::transfer_exactly(sendDataString->size()),
                            [this, self = shared_from_this(), sendDataString, data, &mailbox](
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
                                    sendMail(std::move(data_r), mailbox);
                                    return;
                                }
                                data_r->ok = true;
                                sendMail(std::move(data_r), mailbox);
                            }
                    );
                    return;
                }
                case OwlMailDefine::AdditionCmd::flyMode: {
                    BOOST_LOG_TRIVIAL(warning) << "SerialController"
                                               << " receiveMail"
                                               << " switch (data->additionCmd) OwlMailDefine::AdditionCmd::flyMode"
                                               << " not impl";
                    // send cmd to serial
                    auto sendDataString = std::make_shared<std::vector<uint8_t>>();
                    // TODO
                    // send it
                    boost::asio::async_write(
                            *(airplanePortController->sp_),
                            boost::asio::buffer(*sendDataString),
                            boost::asio::transfer_exactly(sendDataString->size()),
                            [this, self = shared_from_this(), sendDataString, data, &mailbox](
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
                                    sendMail(std::move(data_r), mailbox);
                                    return;
                                }
                                data_r->ok = true;
                                sendMail(std::move(data_r), mailbox);
                            }
                    );
                }
                    break;
                case OwlMailDefine::AdditionCmd::gotoPosition: {
                    BOOST_LOG_TRIVIAL(warning) << "SerialController"
                                               << " receiveMail"
                                               << " switch (data->additionCmd) OwlMailDefine::AdditionCmd::gotoPosition"
                                               << " not impl";
                    // send cmd to serial
                    auto sendDataString = std::make_shared<std::vector<uint8_t>>();
                    // TODO
                    // send it
                    boost::asio::async_write(
                            *(airplanePortController->sp_),
                            boost::asio::buffer(*sendDataString),
                            boost::asio::transfer_exactly(sendDataString->size()),
                            [this, self = shared_from_this(), sendDataString, data, &mailbox](
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
                                    sendMail(std::move(data_r), mailbox);
                                    return;
                                }
                                data_r->ok = true;
                                sendMail(std::move(data_r), mailbox);
                            }
                    );
                }
                    break;
                case OwlMailDefine::AdditionCmd::led: {
                    BOOST_LOG_TRIVIAL(warning) << "SerialController"
                                               << " receiveMail"
                                               << " switch (data->additionCmd) OwlMailDefine::AdditionCmd::led"
                                               << " not impl";
                    // send cmd to serial
                    auto sendDataString = std::make_shared<std::vector<uint8_t>>();
                    // TODO
                    // send it
                    boost::asio::async_write(
                            *(airplanePortController->sp_),
                            boost::asio::buffer(*sendDataString),
                            boost::asio::transfer_exactly(sendDataString->size()),
                            [this, self = shared_from_this(), sendDataString, data, &mailbox](
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
                                    sendMail(std::move(data_r), mailbox);
                                    return;
                                }
                                data_r->ok = true;
                                sendMail(std::move(data_r), mailbox);
                            }
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

                    auto pcx = static_cast<uint16_t>(data->aprilTagCmdPtr->imageCenterX);
                    auto pcy = static_cast<uint16_t>(data->aprilTagCmdPtr->imageCenterY);

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

                    // TODO

                    // send cmd to serial
                    auto sendDataString = std::make_shared<std::vector<uint8_t>>();
                    // 0xAA,0xAdditionCmd,0xXXXX,0xYYYY,0xZZZZ,0xCWCW,0xBB
                    sendDataString->resize(11);
                    // make send data
                    // 0xAA
                    (*sendDataString).at(0) = char(0xAA);
                    // AdditionCmd
                    (*sendDataString).at(1) = uint8_t(data->additionCmd);
                    // 0xXX
                    (*sendDataString).at(2) = uint8_t(uint16_t(data->x) & 0xff);
                    (*sendDataString).at(3) = uint8_t(uint16_t(data->x) >> 8);
                    // 0xYY
                    (*sendDataString).at(4) = uint8_t(uint16_t(data->y) & 0xff);
                    (*sendDataString).at(5) = uint8_t(uint16_t(data->y) >> 8);
                    // 0xZZ
                    (*sendDataString).at(6) = uint8_t(uint16_t(data->z) & 0xff);
                    (*sendDataString).at(7) = uint8_t(uint16_t(data->z) >> 8);
                    // 0xCW
                    (*sendDataString).at(8) = uint8_t(uint16_t(data->cw) & 0xff);
                    (*sendDataString).at(9) = uint8_t(uint16_t(data->cw) >> 8);
                    // 0xBB
                    (*sendDataString).at(10) = char(0xBB);
                    // send it
                    boost::asio::async_write(
                            *(airplanePortController->sp_),
                            boost::asio::buffer(*sendDataString),
                            boost::asio::transfer_exactly(sendDataString->size()),
                            [this, self = shared_from_this(), sendDataString, data, &mailbox](
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
                                    sendMail(std::move(data_r), mailbox);
                                    return;
                                }
                                data_r->ok = true;
                                sendMail(std::move(data_r), mailbox);
                            }
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
