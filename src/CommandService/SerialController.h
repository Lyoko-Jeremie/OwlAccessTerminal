// jeremie

#ifndef OWLACCESSTERMINAL_SERIALCONTROLLER_H
#define OWLACCESSTERMINAL_SERIALCONTROLLER_H

#include <memory>
#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>

namespace OwlSerialController {

    struct PortController : public std::enable_shared_from_this<PortController> {

        explicit PortController(
                boost::asio::io_context &ioc
        ) : sp_(ioc) {
        }

        boost::asio::serial_port sp_;
        std::string deviceName_;

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
                boost::asio::io_context &ioc
        ) : ioc_(ioc),
            airplanePortController(std::make_shared<PortController>(ioc)) {}

    private:
        boost::asio::io_context &ioc_;

        std::shared_ptr<PortController> airplanePortController;

    public:
        bool start(
                const std::string &airplanePort,
                const int bandRate
        ) {
            // TODO set and open the airplanePortController
            return airplanePortController->open(airplanePort)
                   && airplanePortController->set_option(
                    boost::asio::serial_port::baud_rate(bandRate)
            );
        }

    private:
    };

} // OwlSerialController

#endif //OWLACCESSTERMINAL_SERIALCONTROLLER_H
