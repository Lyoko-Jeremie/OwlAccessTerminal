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
            // TODO send cmd to serial
            auto sendDataString = std::make_shared<std::string>();
            // TODO make send data
            (*sendDataString) = "";
            boost::asio::async_write(
                    airplanePortController->sp_,
                    boost::asio::buffer(*sendDataString),
                    boost::asio::transfer_exactly(sendDataString->size()),
                    [this, self = shared_from_this(), sendDataString](
                            const boost::system::error_code &ec,
                            size_t bytes_transferred
                    ) {
                        boost::ignore_unused(bytes_transferred);
                        if (!ec) {
                            // error
                            BOOST_LOG_TRIVIAL(error) << "SerialController"
                                                     << " receiveMail"
                                                     << " async_write error: "
                                                     << ec.what();
                            return;
                        }
                        // make cmd result
                        sendMail({});
                    }
            );
        }

        void sendMail(OwlMailDefine::MailSerial2Cmd &&data) {
            // TODO send cmd result to web
            mailbox_->sendB2A(std::move(data));
        }

    };

} // OwlSerialController

#endif //OWLACCESSTERMINAL_SERIALCONTROLLER_H
