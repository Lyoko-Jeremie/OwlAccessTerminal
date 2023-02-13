// jeremie

#include "SerialController.h"

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

    void SerialController::receiveMail(OwlMailDefine::MailCmd2Serial &&data, OwlMailDefine::CmdSerialMailbox &mailbox) {
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
                            airplanePortController->sp_,
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
                                if (!ec) {
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
                            airplanePortController->sp_,
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
                                if (!ec) {
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
                            airplanePortController->sp_,
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
                                if (!ec) {
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
                            airplanePortController->sp_,
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
                                if (!ec) {
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

                    // send cmd to serial
                    auto sendDataString = std::make_shared<std::vector<uint8_t>>();
//                // 0xAA,0xAdditionCmd,0xXXXX,0xYYYY,0xZZZZ,0xCWCW,0xBB
//                sendDataString->resize(11);
//                // make send data
//                // 0xAA
//                (*sendDataString).at(0) = char(0xAA);
//                // AdditionCmd
//                (*sendDataString).at(1) = uint8_t(data->additionCmd);
//                // 0xXX
//                (*sendDataString).at(2) = uint8_t(uint16_t(data->x) & 0xff);
//                (*sendDataString).at(3) = uint8_t(uint16_t(data->x) >> 8);
//                // 0xYY
//                (*sendDataString).at(4) = uint8_t(uint16_t(data->y) & 0xff);
//                (*sendDataString).at(5) = uint8_t(uint16_t(data->y) >> 8);
//                // 0xZZ
//                (*sendDataString).at(6) = uint8_t(uint16_t(data->z) & 0xff);
//                (*sendDataString).at(7) = uint8_t(uint16_t(data->z) >> 8);
//                // 0xCW
//                (*sendDataString).at(8) = uint8_t(uint16_t(data->cw) & 0xff);
//                (*sendDataString).at(9) = uint8_t(uint16_t(data->cw) >> 8);
//                // 0xBB
//                (*sendDataString).at(10) = char(0xBB);
                    // TODO
                    // send it
                    boost::asio::async_write(
                            airplanePortController->sp_,
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
                                if (!ec) {
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
        if (sp_.is_open()) {
            close();
        }
        deviceName_ = deviceName;
        sp_.open(deviceName, ec);
        if (ec) {
            BOOST_LOG_TRIVIAL(error) << "PortController open error: " << ec.what();
            return false;
        }
        read();
        return true;
    }

    void PortController::read() {
        boost::asio::async_read(
                sp_,
                readBuffer,
                [this, self = shared_from_this()](
                        const boost::system::error_code &ec,
                        size_t bytes_transferred
                ) {
                    if (!ec) {
                        // error
                        BOOST_LOG_TRIVIAL(error) << "SerialController"
                                                 << " airplanePortController"
                                                 << " async_read error: "
                                                 << ec.what();
                        return;
                    }
                    stateReader_->portDataIn(self, bytes_transferred);
                }
        );
    }

    void PortController::read_exactly(size_t need_bytes_transferred) {
        boost::asio::async_read(
                sp_,
                readBuffer,
                boost::asio::transfer_exactly(need_bytes_transferred),
                [this, self = shared_from_this()](
                        const boost::system::error_code &ec,
                        size_t bytes_transferred
                ) {
                    if (!ec) {
                        // error
                        BOOST_LOG_TRIVIAL(error) << "SerialController"
                                                 << " airplanePortController"
                                                 << " read_exactly error: "
                                                 << ec.what();
                        return;
                    }
                    stateReader_->portDataIn(self, bytes_transferred);
                }
        );
    }

    void PortController::read_until(const std::shared_ptr<std::string> &until_delim_ptr) {
        boost::asio::async_read_until(
                sp_,
                readBuffer,
                *until_delim_ptr,
                [this, self = shared_from_this(), until_delim_ptr](
                        const boost::system::error_code &ec,
                        size_t bytes_transferred
                ) {
                    if (!ec) {
                        // error
                        BOOST_LOG_TRIVIAL(error) << "SerialController"
                                                 << " airplanePortController"
                                                 << " read_until error: "
                                                 << ec.what();
                        return;
                    }
                    stateReader_->portDataIn(self, bytes_transferred);
                }
        );
    }

    void StateReader::portDataIn(const std::shared_ptr<PortController> &pt, size_t bytes_transferred) {
        // TODO
        pt->readBuffer;
        pt->readBuffer.consume(bytes_transferred);
    }


} // OwlSerialController
