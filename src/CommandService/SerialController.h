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
#include "../ConfigLoader/ConfigLoader.h"

namespace OwlSerialController {


    class StateReader;

    class SerialController;

    struct PortController : public std::enable_shared_from_this<PortController> {

        explicit PortController(
                boost::asio::io_context &ioc,
                std::weak_ptr<SerialController> &&parentRef
        );

        boost::asio::serial_port sp_;
        std::weak_ptr<SerialController> parentRef_;
        std::string deviceName_;

        boost::asio::streambuf readBuffer;

        std::shared_ptr<StateReader> stateReader_;

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
        );

        bool close() {
            boost::system::error_code ec;
            sp_.close(ec);
            if (ec) {
                BOOST_LOG_TRIVIAL(error) << "PortController close error: " << ec.what();
                return false;
            }
            return true;
        }

        void read();

        void read_exactly(size_t need_bytes_transferred);

        void read_until(const std::shared_ptr<std::string> &until_delim_ptr);

    };

    class SerialController : public std::enable_shared_from_this<SerialController> {
    public:
        explicit SerialController(
                boost::asio::io_context &ioc,
                std::shared_ptr<OwlConfigLoader::ConfigLoader> &&config,
                std::vector<OwlMailDefine::CmdSerialMailbox> &&mailbox_list
        ) : ioc_(ioc), config_(std::move(config)), mailbox_list_(std::move(mailbox_list)),
            airplanePortController(
                    std::make_shared<PortController>(ioc, weak_from_this())
            ) {

            for (auto &m: mailbox_list_) {
                m->receiveA2B = [this, &m](OwlMailDefine::MailCmd2Serial &&data) {
                    receiveMail(std::move(data), m);
                };
            }
        }

        ~SerialController() {
            for (auto &m: mailbox_list_) {
                m->receiveA2B = nullptr;
            }
        }

    private:
        boost::asio::io_context &ioc_;
        std::shared_ptr<OwlConfigLoader::ConfigLoader> config_;
        std::vector<OwlMailDefine::CmdSerialMailbox> mailbox_list_;

        std::shared_ptr<PortController> airplanePortController;

        bool initOk = false;

    public:

    private:
        friend struct PortController;

        void receiveMail(OwlMailDefine::MailCmd2Serial &&data, OwlMailDefine::CmdSerialMailbox &mailbox);

        void sendMail(OwlMailDefine::MailSerial2Cmd &&data, OwlMailDefine::CmdSerialMailbox &mailbox) {
            // send cmd result to Service
            mailbox->sendB2A(std::move(data));
        }

        bool initPort();

    };

} // OwlSerialController

#endif //OWLACCESSTERMINAL_SERIALCONTROLLER_H
