// jeremie

#ifndef OWLACCESSTERMINAL_SERIALCONTROLLER_H
#define OWLACCESSTERMINAL_SERIALCONTROLLER_H

#include "../../MemoryBoost.h"
#include <boost/asio.hpp>
#include <boost/asio/read_until.hpp>
#include "../OwlLog/OwlLog.h"
#include <boost/core/ignore_unused.hpp>
#include <boost/assert.hpp>
#include <boost/bind/bind.hpp>
#include <utility>
#include "CmdSerialMail.h"
#include "../ConfigLoader/ConfigLoader.h"

namespace OwlSerialController {


    struct AirplaneState;

    class StateReader;

    class SerialController;

    struct PortController : public boost::enable_shared_from_this<PortController> {

        explicit PortController(
                boost::asio::io_context &ioc,
                boost::weak_ptr<SerialController> &&parentRef
        );

        void init();

        ~PortController() {
            BOOST_LOG_OWL(trace_dtor) << "~PortController()";
            close();
        }

        boost::shared_ptr<boost::asio::serial_port> sp_;
        boost::weak_ptr<SerialController> parentRef_;
        std::string deviceName_;

        boost::shared_ptr<StateReader> stateReader_;

        friend class StateReader;

        void sendAirplaneState(const boost::shared_ptr<AirplaneState> &airplaneState);

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
                BOOST_LOG_OWL(error) << "PortController set_option error: " << ec.what();
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
            BOOST_LOG_OWL(trace) << "PortController::close";
            if (sp_->is_open()) {
                boost::system::error_code ec;
                sp_->close(ec);
                if (ec) {
                    BOOST_LOG_OWL(error) << "PortController close error: " << ec.what();
                    return false;
                }
            }
            BOOST_LOG_OWL(trace) << "PortController::close true";
            return true;
        }

        bool isOpened() {
            return sp_->is_open();
        }

    };

    struct SerialControllerCmdPackageRecord : public boost::enable_shared_from_this<SerialControllerCmdPackageRecord> {
    private:
        std::atomic_uint16_t packageId = 1;
        boost::shared_ptr<OwlMailDefine::Cmd2Serial> mail_;

        uint16_t nextId() {
            if (packageId < 62766 && packageId > 0) {
                ++packageId;
            } else {
                packageId.store(1);
            }
            return packageId.load();
        }

    public:

        long long lastUpdateTime = 0;

        boost::shared_ptr<OwlMailDefine::Cmd2Serial> mail() {
            return atomic_load(&mail_);
        }

        uint16_t id() {
            return packageId.load();
        }

        uint16_t setNewMail(OwlMailDefine::MailCmd2Serial data) {
            BOOST_ASSERT(data);
            BOOST_ASSERT(!data->callbackRunner);
            atomic_store(&mail_, data);
            lastUpdateTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()
            ).count();
            return nextId();
        }
    };

    class SerialController : public boost::enable_shared_from_this<SerialController> {
    public:
        explicit SerialController(
                boost::asio::io_context &ioc,
                boost::shared_ptr<OwlConfigLoader::ConfigLoader> &&config,
                std::vector<OwlMailDefine::CmdSerialMailbox> &&mailbox_list
        ) : ioc_(ioc), config_(std::move(config)), mailbox_list_(std::move(mailbox_list)),
            package_repeater_timer_(ioc_, std::chrono::milliseconds(1000)) {

            package_record_ = boost::make_shared<SerialControllerCmdPackageRecord>();

            package_repeater_box_ = boost::make_shared<OwlMailDefine::CmdSerialMailbox::element_type>(
                    ioc_, ioc_, "package_repeater_box_"
            );
            package_repeater_box_->receiveB2A([](OwlMailDefine::MailSerial2Cmd &&data) {
                data->runner(data);
            });

            package_repeater_box_->receiveA2B([this](OwlMailDefine::MailCmd2Serial &&data) {
                receiveMailRepeat(std::move(data), package_repeater_box_);
            });

            for (auto &m: mailbox_list_) {
                m->receiveA2B([this, &m](OwlMailDefine::MailCmd2Serial &&data) {
                    receiveMail(std::move(data), m);
                });
            }

        }

        void init() {
            BOOST_ASSERT(!weak_from_this().expired());
            BOOST_LOG_OWL(trace) << "weak_from_this().lock().use_count() : " << weak_from_this().lock().use_count();
            airplanePortController = boost::make_shared<PortController>(ioc_, weak_from_this());
            BOOST_ASSERT(!weak_from_this().expired());
            BOOST_LOG_OWL(trace) << "airplanePortController.use_count() : " << airplanePortController.use_count();
            BOOST_ASSERT(airplanePortController.use_count() > 0);
            airplanePortController->init();

            // init repeater
            package_repeater({}, shared_from_this());
        }

        ~SerialController() {
            BOOST_LOG_OWL(trace_dtor) << "~SerialController()";
            for (auto &m: mailbox_list_) {
                m->receiveA2B(nullptr);
            }
        }

    private:
        boost::asio::io_context &ioc_;
        boost::shared_ptr<OwlConfigLoader::ConfigLoader> config_;
        std::vector<OwlMailDefine::CmdSerialMailbox> mailbox_list_;

        boost::shared_ptr<PortController> airplanePortController;

        bool initOk = false;

        template<uint8_t packageSize>
        friend void sendADataBuffer(
                boost::shared_ptr<SerialController> selfPtr,
                boost::shared_ptr<std::array<uint8_t, packageSize>> sendDataBuffer,
                OwlMailDefine::MailCmd2Serial data,
                OwlMailDefine::CmdSerialMailbox &mailbox
        );

        boost::shared_ptr<SerialControllerCmdPackageRecord> package_record_;

    public:

        void sendAirplaneState(const boost::shared_ptr<AirplaneState> &airplaneState);

    private:

        boost::shared_ptr<AirplaneState> newestAirplaneState = boost::make_shared<AirplaneState>();
        boost::shared_ptr<OwlMailDefine::AprilTagCmd> aprilTagCmdData = boost::make_shared<OwlMailDefine::AprilTagCmd>();

    private:
        friend struct PortController;

        void receiveMail(OwlMailDefine::MailCmd2Serial &&data, OwlMailDefine::CmdSerialMailbox &mailbox);

        void receiveMailRepeat(OwlMailDefine::MailCmd2Serial &&data, OwlMailDefine::CmdSerialMailbox &mailbox);

        void sendData2Serial(
                OwlMailDefine::MailCmd2Serial data,
                OwlMailDefine::CmdSerialMailbox &mailbox,
                uint16_t id,
                bool repeating,
                bool needRepeat);

        void receiveMailGetAirplaneState(
                OwlMailDefine::MailCmd2Serial &&data,
                OwlMailDefine::CmdSerialMailbox &mailbox);

        void sendMail(OwlMailDefine::MailSerial2Cmd &&data, OwlMailDefine::CmdSerialMailbox &mailbox) {
            // send cmd result to Service
            mailbox->sendB2A(std::move(data));
        }

        bool initPort();

    private:

        boost::asio::steady_timer package_repeater_timer_;
        OwlMailDefine::CmdSerialMailbox package_repeater_box_;

        void package_repeater(const boost::system::error_code &ec, boost::shared_ptr<SerialController> self) {
            boost::ignore_unused(ec);
            package_repeater_timer_.cancel();
            if (initOk && airplanePortController && airplanePortController->isOpened()) {
                auto mm = boost::make_shared<OwlMailDefine::MailCmd2Serial::element_type>();
                mm->additionCmd = OwlMailDefine::AdditionCmd::ignore;
                mm->callbackRunner = [this, self = std::move(self)](OwlMailDefine::MailSerial2Cmd &&data) {
                    boost::ignore_unused(data);
                    package_repeater_timer_.expires_after(std::chrono::milliseconds(200));
                    package_repeater_timer_.async_wait(
                            boost::bind(&SerialController::package_repeater,
                                        this, boost::asio::placeholders::error,
                                        shared_from_this()));
                };
                package_repeater_box_->sendA2B(std::move(mm));
            } else {
                package_repeater_timer_.expires_after(std::chrono::milliseconds(200));
                package_repeater_timer_.async_wait(
                        boost::bind(&SerialController::package_repeater,
                                    this, boost::asio::placeholders::error,
                                    shared_from_this()));
            }
        }

    };

} // OwlSerialController

#endif //OWLACCESSTERMINAL_SERIALCONTROLLER_H
