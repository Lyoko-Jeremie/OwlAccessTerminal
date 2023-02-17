// jeremie

#ifndef OWLACCESSTERMINAL_SERIALCONTROLLER_H
#define OWLACCESSTERMINAL_SERIALCONTROLLER_H

#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/log/trivial.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/assert.hpp>
#include <utility>
#include "CmdSerialMail.h"
#include "../ConfigLoader/ConfigLoader.h"

namespace OwlSerialController {


    struct AirplaneState;

    class StateReader;

    class SerialController;

    struct PortController : public std::enable_shared_from_this<PortController> {

        explicit PortController(
                boost::asio::io_context &ioc,
                std::weak_ptr<SerialController> &&parentRef
        );

        void init();

        ~PortController() {
            BOOST_LOG_TRIVIAL(trace) << "~SerialController()";
            close();
        }

        std::shared_ptr<boost::asio::serial_port> sp_;
        std::weak_ptr<SerialController> parentRef_;
        std::string deviceName_;

        std::shared_ptr<StateReader> stateReader_;

        friend class StateReader;

        void sendAirplaneState(const std::shared_ptr<AirplaneState> &airplaneState);

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
            BOOST_ASSERT(sp_);
            sp_->set_option(option, ec);
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
            BOOST_ASSERT(sp_);
            BOOST_LOG_TRIVIAL(trace) << "PortController::close";
            if (sp_->is_open()) {
                boost::system::error_code ec;
                sp_->close(ec);
                if (ec) {
                    BOOST_LOG_TRIVIAL(error) << "PortController close error: " << ec.what();
                    return false;
                }
            }
            BOOST_LOG_TRIVIAL(trace) << "PortController::close true";
            return true;
        }

    };

    class SerialController : public std::enable_shared_from_this<SerialController> {
    public:
        explicit SerialController(
                boost::asio::io_context &ioc,
                std::shared_ptr<OwlConfigLoader::ConfigLoader> &&config,
                std::vector<OwlMailDefine::CmdSerialMailbox> &&mailbox_list
        ) : ioc_(ioc), config_(std::move(config)), mailbox_list_(std::move(mailbox_list)) {

            for (auto &m: mailbox_list_) {
                m->receiveA2B = [this, &m](OwlMailDefine::MailCmd2Serial &&data) {
                    receiveMail(std::move(data), m);
                };
            }
        }

        void init() {
            BOOST_ASSERT(!weak_from_this().expired());
            BOOST_ASSERT(weak_from_this().lock().unique());
            airplanePortController = std::make_shared<PortController>(ioc_, weak_from_this());
            BOOST_ASSERT(!weak_from_this().expired());
            BOOST_LOG_TRIVIAL(trace) << "airplanePortController.use_count() : " << airplanePortController.use_count();
            BOOST_ASSERT(airplanePortController.use_count() > 0);
            airplanePortController->init();
        }

        ~SerialController() {
            BOOST_LOG_TRIVIAL(trace) << "~SerialController()";
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

        template<uint8_t packageSize>
        friend void sendADataBuffer(
                std::shared_ptr<SerialController> selfPtr,
                std::shared_ptr<std::array<uint8_t, packageSize>> sendDataBuffer,
                OwlMailDefine::MailCmd2Serial data,
                OwlMailDefine::CmdSerialMailbox &mailbox
        );

    public:

        void sendAirplaneState(const std::shared_ptr<AirplaneState> &airplaneState);

    private:

        std::shared_ptr<AirplaneState> newestAirplaneState;

    private:
        friend struct PortController;

        void receiveMail(OwlMailDefine::MailCmd2Serial &&data, OwlMailDefine::CmdSerialMailbox &mailbox);

        void receiveMailGetAirplaneState(
                OwlMailDefine::MailCmd2Serial &&data,
                OwlMailDefine::CmdSerialMailbox &mailbox);

        void sendMail(OwlMailDefine::MailSerial2Cmd &&data, OwlMailDefine::CmdSerialMailbox &mailbox) {
            // send cmd result to Service
            mailbox->sendB2A(std::move(data));
        }

        bool initPort();

    };

} // OwlSerialController

#endif //OWLACCESSTERMINAL_SERIALCONTROLLER_H
