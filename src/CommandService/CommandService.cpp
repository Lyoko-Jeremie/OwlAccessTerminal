// jeremie

#include "CommandService.h"
#include "../Log/Log.h"

namespace OwlCommandService {


    void CommandService::next_receive() {
        udp_socket_.async_receive_from(
                boost::asio::buffer(receive_buffer_, UDP_Package_Max_Size),
                remote_endpoint_,
                [this, self = shared_from_this()](
                        const boost::system::error_code &ec,
                        std::size_t bytes_transferred
                ) {
                    if (ec) {
                        // ignore it
//                        std::cerr << ec.what() << "\n";
                        BOOST_LOG_OWL(error) << ec.what();
                    } else {
                        process_message(bytes_transferred);
                    }
                    next_receive();
                }
        );
    }


    void CommandService::send_back(std::string &&json_string) {
        auto send_string_ptr = std::make_shared<std::string>(std::move(json_string));
        udp_socket_.async_send_to(
                boost::asio::buffer(*send_string_ptr),
                remote_endpoint_,
                [this, self = shared_from_this(), send_string_ptr](
                        const boost::system::error_code &ec,
                        std::size_t bytes_transferred
                ) {
                    boost::ignore_unused(ec);
                    boost::ignore_unused(bytes_transferred);
                    // ignore
                    if (ec) {
//                        std::cerr << ec.what() << "\n";
                        BOOST_LOG_OWL(error) << ec.what();
                    }
                }
        );
    }

    void CommandService::send_back_json(const boost::json::value &json_value) {
        send_back(boost::json::serialize(json_value));
    }


    void CommandService::process_message(std::size_t bytes_transferred) {
        OwlProcessJsonMessage::process_json_message(
                boost::string_view{receive_buffer_.data(), bytes_transferred},
                json_storage_resource_,
                json_parse_options_,
                shared_from_this()
        );
    }


} // OwlCommandService