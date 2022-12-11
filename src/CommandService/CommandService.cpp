// jeremie

#include "CommandService.h"

namespace OwlCommandService {


    void CommandService::next_receive() {
        udp_socket.async_receive_from(
                boost::asio::buffer(receive_buffer_, UDP_Package_Max_Size),
                remote_endpoint_,
                [this, self = shared_from_this()](
                        const boost::system::error_code &ec,
                        std::size_t bytes_transferred
                ) {
                    if (ec) {
                        // ignore it
                    } else {
                        process_message(bytes_transferred);
                        next_receive();
                    }
                }
        );
    }


    void CommandService::send_back(std::string &&json_string) {
        auto send_string_ptr = std::make_shared<std::string>(std::move(json_string));
        udp_socket.async_send_to(
                boost::asio::buffer(*send_string_ptr),
                remote_endpoint_,
                [this, self = shared_from_this(), send_string_ptr](
                        const boost::system::error_code &ec,
                        std::size_t bytes_transferred
                ) {
                    boost::ignore_unused(ec);
                    boost::ignore_unused(bytes_transferred);
                    // ignore
                }
        );
    }


    void CommandService::process_message(std::size_t bytes_transferred) {
        try {
            boost::system::error_code ec;
            boost::json::value json_v = boost::json::parse(
                    boost::string_view{receive_buffer_.data(), bytes_transferred},
                    ec,
                    &json_storage_resource,
                    json_parse_options
            );
            if (ec) {
                // ignore
                std::cerr << ec.what() << "\n";
                return;
            }
            std::cout << boost::json::serialize(json_v) << "\n";
            auto json_o = json_v.as_object();
            auto cmdId = boost::json::value_to<int32_t>(json_o.at("cmdId"));
            auto packageId = boost::json::value_to<int32_t>(json_o.at("packageId"));
            switch (cmdId) {
                case 0:
                    // ping-pong
                    send_back(
                            boost::json::serialize(
                                    boost::json::value{
                                            {"cmdId",     cmdId},
                                            {"packageId", packageId},
                                            {"msg",       "pong"},
                                    }
                            )
                    );
                    break;
                default:
                    // ignore
                    break;
            }
            return;
        } catch (std::exception &e) {
            std::cerr << e.what() << "\n";
            // ignore
            return;
        } catch (...) {
            // ignore
            return;
        }
    }


} // OwlCommandService