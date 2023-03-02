// jeremie

#ifndef OWLACCESSTERMINAL_COMMANDSERVICE_H
#define OWLACCESSTERMINAL_COMMANDSERVICE_H

#include "../MemoryBoost.h"
#include <iostream>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/json.hpp>
#include <boost/system/error_code.hpp>
#include <boost/utility/string_view.hpp>
#include "CmdSerialMail.h"
#include "ProcessJsonMessage.h"

namespace OwlCommandService {

    enum {
        UDP_Package_Max_Size = (1024 * 1024 * 6)
    }; // 6M

    class CommandService
            : public boost::enable_shared_from_this<CommandService>,
              public OwlProcessJsonMessage::ProcessJsonMessageSelfTypeInterface {
    public:
        CommandService(boost::asio::io_context &_ioc,
                       OwlMailDefine::CmdSerialMailbox &&mailbox,
                       const boost::asio::ip::udp::endpoint &endpoint)
                : executor_(boost::asio::make_strand(_ioc)),
                  mailbox_(std::move(mailbox)),
                  udp_socket_(_ioc, endpoint),
                  receive_buffer_(),
                  json_storage_(),
                  json_storage_resource_(json_storage_.data(), UDP_Package_Max_Size) {

            json_parse_options_.allow_comments = true;
            json_parse_options_.allow_trailing_commas = true;
            json_parse_options_.max_depth = 5;
            //  boost::asio::ip::udp::endpoint _endpoint(boost::asio::ip::udp::v4(), 2333);

            mailbox_->receiveB2A([this](OwlMailDefine::MailSerial2Cmd &&data) {
                receiveMail(std::move(data));
            });
        }

        ~CommandService() {
            BOOST_LOG_OWL(trace_dtor) << "~CommandService()";
            mailbox_->receiveB2A(nullptr);
        }

    private:
        boost::asio::executor executor_;
        OwlMailDefine::CmdSerialMailbox mailbox_;
        boost::asio::ip::udp::socket udp_socket_;

        // the max udp receive buffer, it used to receive package from remove client
        boost::array<char, UDP_Package_Max_Size> receive_buffer_;
        // the remote client endpoint , it used to record where the package come from
        boost::asio::ip::udp::endpoint remote_endpoint_;

        boost::json::parse_options json_parse_options_;
        boost::array<unsigned char, UDP_Package_Max_Size> json_storage_;
        boost::json::static_resource json_storage_resource_;

    public:
        void start() {
            next_receive();
        }

    private:
        void next_receive();

        void process_message(std::size_t bytes_transferred);

        void send_back(std::string &&json_string);


    private:
        // https://en.cppreference.com/w/cpp/language/friend
        template<typename SelfType, typename SelfPtrType>
        friend void OwlProcessJsonMessage::process_json_message(
                boost::string_view,
                boost::json::static_resource &,
                boost::json::parse_options &,
                // const boost::shared_ptr<ProcessJsonMessageSelfTypeInterface> &&self
                SelfPtrType &&
        );

        template<typename SelfType, typename SelfPtrType>
        friend void OwlProcessJsonMessage::analysisJoyCon(
                SelfPtrType self,
                boost::json::object &joyCon,
                int cmdId,
                int packageId
        );

        template<typename SelfType, typename SelfPtrType>
        friend void OwlProcessJsonMessage::analysisJoyConSimple(
                SelfPtrType self,
                boost::json::object &joyCon,
                int cmdId,
                int packageId
        );

        template<typename SelfType, typename SelfPtrType>
        friend void OwlProcessJsonMessage::analysisJoyConGyro(
                SelfPtrType self,
                boost::json::object &joyCon,
                int cmdId,
                int packageId
        );

        void send_back_json(const boost::json::value &json_value) override;

        void receiveMail(OwlMailDefine::MailSerial2Cmd &&data) {
            // get callback from data and call it to send back html result
            data->runner(data);
        }

        void sendMail(OwlMailDefine::MailCmd2Serial &&data) override {
            // send cmd to serial
            mailbox_->sendA2B(std::move(data));
        }

    private:

    };

    template<typename T>
    T getFromJsonObject(const boost::json::object &v, boost::string_view key, bool &good) {
        try {
            T r = boost::json::value_to<T>(v.at(key));
            return r;
        } catch (std::exception &e) {
            good = false;
            return T{};
        }
    }

} // OwlCommandService

#endif //OWLACCESSTERMINAL_COMMANDSERVICE_H
