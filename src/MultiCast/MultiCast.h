// jeremie

#ifndef OWLACCESSTERMINAL_MULTICAST_H
#define OWLACCESSTERMINAL_MULTICAST_H

#include "../MemoryBoost.h"
#include <chrono>
#include <utility>
#include <boost/asio.hpp>
#include <boost/json.hpp>
#include "../OwlLog/OwlLog.h"
#include "../ConfigLoader/ConfigLoader.h"
#include "./ControlMulticastMail.h"

namespace OwlMultiCast {

    class MultiCast : public boost::enable_shared_from_this<MultiCast> {
    public:
        MultiCast(
                boost::asio::io_context &ioc,
                boost::shared_ptr<OwlConfigLoader::ConfigLoader> config,
                OwlMailDefine::ControlMulticastMailbox &&mailbox
        ) : ioc_(ioc),
            config_(std::move(config)),
            mailbox_(mailbox),
            timer_(ioc_),
            sender_socket_(ioc_),
            listen_socket_(ioc_) {

            // multicast_port_
            // listen_address_
            // multicast_address_

            sender_endpoint_ = decltype(sender_endpoint_){multicast_address_, multicast_port_};
            sender_socket_.open(sender_endpoint_.protocol());

            listen_endpoint_ = decltype(listen_endpoint_){listen_address_, multicast_port_};
            listen_socket_.open(listen_endpoint_.protocol());
            listen_socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
            listen_socket_.bind(listen_endpoint_);

            // Join the multicast group.
            listen_socket_.set_option(boost::asio::ip::multicast::join_group(multicast_address_));

            json_parse_options_.allow_comments = true;
            json_parse_options_.allow_trailing_commas = true;
            json_parse_options_.max_depth = 5;

            static_send_message_ = R"({"MultiCast":"MultiCast"})";
        }

    private:
        boost::asio::io_context &ioc_;
        boost::shared_ptr<OwlConfigLoader::ConfigLoader> config_;
        OwlMailDefine::ControlMulticastMailbox mailbox_;

        boost::asio::steady_timer timer_;

        boost::asio::ip::address multicast_address_{boost::asio::ip::make_address("239.255.0.1")};
        boost::asio::ip::port_type multicast_port_ = 30003;

        // send package
        boost::asio::ip::udp::endpoint sender_endpoint_;
        boost::asio::ip::udp::socket sender_socket_;

        // listen package
        boost::asio::ip::address listen_address_{boost::asio::ip::make_address("0.0.0.0")};
        boost::asio::ip::udp::endpoint listen_endpoint_;
        boost::asio::ip::udp::socket listen_socket_;

        // where the package come
        boost::asio::ip::udp::endpoint receiver_endpoint_;
        std::array<char, 1024 * 64 * 10> receive_data_{};


        boost::json::parse_options json_parse_options_;

        std::string static_send_message_;
        std::string response_message_;
    public:

    private:

        template<typename T>
        std::remove_cvref_t<T> get(const boost::json::object &v, boost::string_view key, T &&d) {
            try {
                if (!v.contains(key)) {
                    return d;
                }
                auto rr = boost::json::try_value_to<std::remove_cvref_t<T>>(v.at(key));
                return rr.has_value() ? rr.value() : d;
            } catch (std::exception &e) {
                return d;
            }
        }

        void do_receive_json(std::size_t length) {
            boost::system::error_code ecc;
            boost::json::value json_v = boost::json::parse(
                    std::string{receive_data_.data(), receive_data_.data() + length},
                    ecc,
                    {},
                    json_parse_options_
            );
            if (ecc) {
                BOOST_LOG_OWL(warning) << "MultiCast do_receive() invalid package";
                do_receive();
                return;
            }

            BOOST_LOG_OWL(info) << "receiver_endpoint_ "
                                << receiver_endpoint_.address() << ":" << receiver_endpoint_.port()
                                << " " << boost::json::serialize(json_v);

            auto multiCastFlag = get(json_v.as_object(), "MultiCast", std::string{});
            if (multiCastFlag != "multiCastFlag") {
                // err
                BOOST_LOG_OWL(warning) << "MultiCast do_receive_json() (multiCastFlag != multiCastFlag)";
                do_response();
                return;
            }

            // TODO response_message_
            response_message_ = R"({"MultiCast","MultiCast"})";
            do_response();
        }

        void do_receive() {
            listen_socket_.async_receive_from(
                    boost::asio::buffer(receive_data_), receiver_endpoint_,
                    [this](boost::system::error_code ec, std::size_t length) {
                        if (!ec) {
                            do_receive_json(length);
                            return;
                        }
                        if (ec == boost::asio::error::operation_aborted) {
                            BOOST_LOG_OWL(info) << "MultiCast do_receive() ec operation_aborted";
                            return;
                        }
                        BOOST_LOG_OWL(error) << "MultiCast do_receive() ec " << ec.what();
                    });
        }

        void do_response() {

            sender_socket_.async_send_to(
                    boost::asio::buffer(response_message_), receiver_endpoint_,
                    [this](boost::system::error_code ec, std::size_t /*length*/) {
                        do_receive();
                    });
        }

        void do_send() {

            sender_socket_.async_send_to(
                    boost::asio::buffer(static_send_message_), sender_endpoint_,
                    [this](boost::system::error_code ec, std::size_t /*length*/) {
                        do_timeout();
                    });
        }

        void do_timeout() {
            timer_.expires_after(std::chrono::seconds(1));
            timer_.async_wait(
                    [this](boost::system::error_code ec) {
                        if (ec == boost::asio::error::operation_aborted) {
                            BOOST_LOG_OWL(info) << "MultiCast do_timeout() ec operation_aborted";
                            return;
                        }
                        do_send();
                    });
        }

    };

} // OwlMultiCast

#endif //OWLACCESSTERMINAL_MULTICAST_H
