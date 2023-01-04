// jeremie

#include "SerialController.h"

namespace OwlSerialController {

    bool SerialController::start(const std::string &airplanePort, const int bandRate) {
        // TODO set and open the airplanePortController
        bool init = airplanePortController->open(airplanePort)
                    && airplanePortController->set_option(
                boost::asio::serial_port::baud_rate(bandRate)
        );
        if (!init) {
            return false;
        }
        boost::asio::async_read(
                airplanePortController->sp_,
                airplanePortController->readBuffer,
                [this, self = weak_from_this()](
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
                    return;
//                        {
//                            auto thisP = self.lock();
//                            if (thisP) {
//                                // TODO process the data
//                                thisP->airplanePortController->
//                                        readBuffer.consume(bytes_transferred);
//                            }
//                        }
                }
        );
        return true;
    }

    void SerialController::receiveMail(OwlMailDefine::MailCmd2Serial &&data) {
        // send cmd to serial
        auto sendDataString = std::make_shared<std::vector<uint8_t>>();
        // 0xAA,0xAdditionCmd,0xXXXX,0xYYYY,0xZZZZ,0xCWCW,0xBB
        sendDataString->resize(11);
        // make send data
        // 0xAA
        (*sendDataString)[0] = char(0xAA);
        // AdditionCmd
        (*sendDataString)[1] = uint8_t(data->additionCmd);
        // 0xXX
        (*sendDataString)[2] = uint8_t(uint16_t(data->x) & 0xff);
        (*sendDataString)[3] = uint8_t(uint16_t(data->x) >> 8);
        // 0xYY
        (*sendDataString)[4] = uint8_t(uint16_t(data->y) & 0xff);
        (*sendDataString)[5] = uint8_t(uint16_t(data->y) >> 8);
        // 0xZZ
        (*sendDataString)[6] = uint8_t(uint16_t(data->z) & 0xff);
        (*sendDataString)[7] = uint8_t(uint16_t(data->z) >> 8);
        // 0xCW
        (*sendDataString)[8] = uint8_t(uint16_t(data->cw) & 0xff);
        (*sendDataString)[9] = uint8_t(uint16_t(data->cw) >> 8);
        // 0xBB
        (*sendDataString)[11] = char(0xBB);
        // send it
        boost::asio::async_write(
                airplanePortController->sp_,
                boost::asio::buffer(*sendDataString),
                boost::asio::transfer_exactly(sendDataString->size()),
                [this, self = shared_from_this(), sendDataString, data](
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
                        sendMail(std::move(data_r));
                        return;
                    }
                    data_r->ok = true;
                    sendMail(std::move(data_r));
                }
        );
    }

} // OwlSerialController