// jeremie

#ifndef OWLACCESSTERMINAL_SERIALCONTROLLER_H
#define OWLACCESSTERMINAL_SERIALCONTROLLER_H

#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/log/trivial.hpp>
#include <boost/core/ignore_unused.hpp>
#include <utility>
#include "CmdSerialMail.h"

namespace OwlSerialController {

    class SerialController;

    struct PortController : public std::enable_shared_from_this<PortController> {

        explicit PortController(
                boost::asio::io_context &ioc,
                std::weak_ptr<SerialController> &&parentRef
        ) : sp_(ioc), parentRef_(std::move(parentRef)) {
        }

        boost::asio::serial_port sp_;
        std::weak_ptr<SerialController> parentRef_;
        std::string deviceName_;

        boost::asio::streambuf readBuffer;

        /**
         * @tparam SettableSerialPortOption from boost::asio::serial_port::
         * @param option SettableSerialPortOption
         * @return ok or err
         */
        template<typename SettableSerialPortOption>
        bool set_option(
                SettableSerialPortOption option
        ) {
            boost::system::error_code ec;
            return set_option<SettableSerialPortOption>(option, ec);
        }

        /**
         * @tparam SettableSerialPortOption from boost::asio::serial_port::
         * @param option SettableSerialPortOption
         * @param ec
         * @return ok or err
         */
        template<typename SettableSerialPortOption>
        bool set_option(
                SettableSerialPortOption option,
                boost::system::error_code &ec
        ) {
            boost::asio::serial_port::baud_rate(0);
            sp_.set_option(option, ec);
            if (ec) {
                BOOST_LOG_TRIVIAL(error) << "PortController set_option error: " << ec.what();
                return false;
            }
            return true;
        }

        bool open(
                const std::string &deviceName
        ) {
            boost::system::error_code ec;
            return open(deviceName, ec);
        }

        bool open(
                const std::string &deviceName,
                boost::system::error_code &ec
        ) {
            deviceName_ = deviceName;
            sp_.open(deviceName, ec);
            if (ec) {
                BOOST_LOG_TRIVIAL(error) << "PortController open error: " << ec.what();
                return false;
            }
            return true;
        }


    };

    class SerialController : public std::enable_shared_from_this<SerialController> {
    public:
        explicit SerialController(
                boost::asio::io_context &ioc,
                OwlMailDefine::CmdSerialMailbox &&mailbox
        ) : ioc_(ioc), mailbox_(std::move(mailbox)),
            airplanePortController(
                    std::make_shared<PortController>(ioc, weak_from_this())
            ) {
            mailbox_->receiveA2B = [this](OwlMailDefine::MailCmd2Serial &&data) {
                receiveMail(std::move(data));
            };
        }

        ~SerialController() {
            mailbox_->receiveA2B = nullptr;
        }

    private:
        boost::asio::io_context &ioc_;
        OwlMailDefine::CmdSerialMailbox mailbox_;

        std::shared_ptr<PortController> airplanePortController;

    public:
        bool start(
                const std::string &airplanePort,
                const int bandRate
        ) {
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

    private:
        friend struct PortController;

        void receiveMail(OwlMailDefine::MailCmd2Serial &&data) {
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
            // 0xCW1
            (*sendDataString)[8] = uint8_t(uint16_t(data->cw) & 0xff);
            (*sendDataString)[9] = uint8_t(uint16_t(data->cw) >> 8);
            // 0xBBBB
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

        void sendMail(OwlMailDefine::MailSerial2Cmd &&data) {
            // send cmd result to Service
            mailbox_->sendB2A(std::move(data));
        }

    };

} // OwlSerialController

#endif //OWLACCESSTERMINAL_SERIALCONTROLLER_H
