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

    enum {
        UDP_Package_Max_Size = (1024 * 1024 * 6), // 6M
        JSON_Package_Max_Size = (1024 * 1024 * 6), // 6M
    };

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
            listen_socket_(ioc_),
            json_storage_(std::make_unique<decltype(json_storage_)::element_type>(JSON_Package_Max_Size, 0)),
            json_storage_resource_(std::make_unique<decltype(json_storage_resource_)::element_type>
                                           (json_storage_->data(), json_storage_->size())) {

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

            // notice all client i am here
            static_send_message_ = R"({"MultiCast":"Notice"})";

        }

        void start() {
            BOOST_LOG_OWL(trace_multicast) << "MultiCast start";
            do_receive();
            do_send();
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
        std::array<char, UDP_Package_Max_Size> receive_data_{};


        boost::json::parse_options json_parse_options_;
        std::unique_ptr<std::vector<unsigned char>> json_storage_;
        std::unique_ptr<boost::json::static_resource> json_storage_resource_;

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

        // ======================================================================================================

        void do_receive_json(std::size_t length) {
            auto data = std::string{receive_data_.data(), receive_data_.data() + length};
            BOOST_LOG_OWL(trace_multicast) << "MultiCast do_receive_json() data"
                                           << "receiver_endpoint_ "
                                           << receiver_endpoint_.address() << ":" << receiver_endpoint_.port()
                                           << " " << data;
            boost::system::error_code ecc;
            boost::json::value json_v = boost::json::parse(
                    data,
                    ecc,
                    &*json_storage_resource_,
                    json_parse_options_
            );
            if (ecc) {
                BOOST_LOG_OWL(warning) << "MultiCast do_receive_json() invalid package";
                do_receive();
                return;
            }

            BOOST_LOG_OWL(trace_multicast) << "MultiCast do_receive_json() "
                                           << "receiver_endpoint_ "
                                           << receiver_endpoint_.address() << ":" << receiver_endpoint_.port()
                                           << " " << boost::json::serialize(json_v);

            auto multiCastFlag = get(json_v.as_object(), "MultiCast", std::string{});
            if (multiCastFlag != "Query") {
                // ignore it
                BOOST_LOG_OWL(trace_multicast) << "MultiCast do_receive_json() (multiCastFlag != Query) , ignore";
                do_response();
                return;
            }
            // now we receive a Query package, so we need response it

            // TODO response_message_
            response_message_ = R"({"MultiCast","Response"})";
            do_response();
        }

        void do_receive() {
            listen_socket_.async_receive_from(
                    boost::asio::buffer(receive_data_), receiver_endpoint_,
                    [this, sef = shared_from_this()](boost::system::error_code ec, std::size_t length) {
                        if (!ec) {
                            do_receive_json(length);
                            return;
                        }
                        if (ec == boost::asio::error::operation_aborted) {
                            BOOST_LOG_OWL(trace_multicast) << "MultiCast do_receive() ec operation_aborted";
                            return;
                        }
                        BOOST_LOG_OWL(error) << "MultiCast do_receive() ec " << ec.what();
                    });
        }

        void do_response() {

            BOOST_LOG_OWL(trace_multicast) << "MultiCast do_response() "
                                           << "receiver_endpoint_ "
                                           << receiver_endpoint_.address() << ":" << receiver_endpoint_.port()
                                           << " " << response_message_;

            sender_socket_.async_send_to(
                    boost::asio::buffer(response_message_), receiver_endpoint_,
                    [this, sef = shared_from_this()](boost::system::error_code ec, std::size_t /*length*/) {
                        if (ec == boost::asio::error::operation_aborted) {
                            BOOST_LOG_OWL(trace_multicast) << "MultiCast do_response() ec operation_aborted";
                            return;
                        }
                        if (!ec) {
                            do_receive();
                            return;
                        }
                        BOOST_LOG_OWL(error) << "MultiCast do_response() ec " << ec.what();
                    });
        }

        // ======================================================================================================

        void do_send() {

            sender_socket_.async_send_to(
                    boost::asio::buffer(static_send_message_), sender_endpoint_,
                    [this, sef = shared_from_this()](boost::system::error_code ec, std::size_t /*length*/) {
                        if (ec == boost::asio::error::operation_aborted) {
                            BOOST_LOG_OWL(trace_multicast) << "MultiCast do_send() ec operation_aborted";
                            return;
                        }
                        if (!ec) {
                            BOOST_LOG_OWL(trace_multicast) << "MultiCast do_send() wait to send next";
                            do_timeout();
                            return;
                        }
                        BOOST_LOG_OWL(error) << "MultiCast do_send() ec operation_aborted";
                    });
        }

        void do_timeout() {
            timer_.expires_after(std::chrono::seconds(1));
            timer_.async_wait(
                    [this, sef = shared_from_this()](boost::system::error_code ec) {
                        if (ec == boost::asio::error::operation_aborted) {
                            BOOST_LOG_OWL(trace_multicast) << "MultiCast do_timeout() ec operation_aborted";
                            return;
                        }
                        BOOST_LOG_OWL(trace_multicast) << "MultiCast do_timeout() to send next";
                        do_send();
                    });
        }

    };

} // OwlMultiCast

#endif //OWLACCESSTERMINAL_MULTICAST_H
