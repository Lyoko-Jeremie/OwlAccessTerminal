// jeremie

#include "CommandService.h"
#include <boost/log/trivial.hpp>

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
                        std::cerr << ec.what() << "\n";
                        BOOST_LOG_TRIVIAL(error) << ec.what();
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
                        std::cerr << ec.what() << "\n";
                        BOOST_LOG_TRIVIAL(error) << ec.what();
                    }
                }
        );
    }

    void CommandService::send_back_json(const boost::json::value &json_value) {
        send_back(boost::json::serialize(json_value));
    }


    void CommandService::process_message(std::size_t bytes_transferred) {
        try {
            boost::system::error_code ec;
            auto jsv = boost::string_view{receive_buffer_.data(), bytes_transferred};
            boost::json::value json_v = boost::json::parse(
                    jsv,
                    ec,
                    &json_storage_resource_,
                    json_parse_options_
            );
            if (ec) {
                // ignore
//                std::cerr << ec.what() << "\n";
                BOOST_LOG_TRIVIAL(error) << ec.what();
                return;
            }
//            std::cout << boost::json::serialize(json_v) << "\n";
            BOOST_LOG_TRIVIAL(info) << boost::json::serialize(json_v);
            auto json_o = json_v.as_object();
            if (!json_o.contains("cmdId") && !json_o.contains("packageId")) {
                BOOST_LOG_TRIVIAL(warning) << "contains fail " << jsv;
                send_back_json(
                        boost::json::value{
                                {"msg",    "error"},
                                {"error",  "(cmdId||packageId) not find"},
                                {"result", false},
                        }
                );
                return;
            }
            bool good = true;
            auto cmdId = getFromJsonObject<int32_t>(json_o, "cmdId", good);
            auto packageId = getFromJsonObject<int32_t>(json_o, "packageId", good);
            if (!good) {
                BOOST_LOG_TRIVIAL(warning) << "getFromJsonObject fail " << jsv;
                send_back_json(
                        boost::json::value{
                                {"msg",    "error"},
                                {"error",  "(cmdId||packageId) getFromJsonObject fail"},
                                {"result", false},
                        }
                );
                return;
            }
            switch (cmdId) {
                case 0:
                    // ping-pong
                    BOOST_LOG_TRIVIAL(info) << "ping-pong";
                    send_back_json(
                            boost::json::value{
                                    {"cmdId",     cmdId},
                                    {"packageId", packageId},
                                    {"msg",       "pong"},
                                    {"result",    true},
                            }
                    );
                    break;
                case 10:
                    // break
                    BOOST_LOG_TRIVIAL(info) << "break";
                    send_back_json(
                            boost::json::value{
                                    {"cmdId",     cmdId},
                                    {"packageId", packageId},
                                    {"msg",       "break"},
                                    {"result",    true},
                            }
                    );
                    break;
                case 11:
                    // takeoff
                    BOOST_LOG_TRIVIAL(info) << "takeoff";
                    send_back_json(
                            boost::json::value{
                                    {"cmdId",     cmdId},
                                    {"packageId", packageId},
                                    {"msg",       "takeoff"},
                                    {"result",    true},
                            }
                    );
                    break;
                case 12:
                    // land
                    BOOST_LOG_TRIVIAL(info) << "land";
                    send_back_json(
                            boost::json::value{
                                    {"cmdId",     cmdId},
                                    {"packageId", packageId},
                                    {"msg",       "land"},
                                    {"result",    true},
                            }
                    );
                    break;
                case 13:
                    // move step
                    BOOST_LOG_TRIVIAL(info) << "move step";
                    {
                        if (!json_o.contains("forward") && !json_o.contains("distance")) {
                            BOOST_LOG_TRIVIAL(warning) << "move step contains fail " << jsv;
                            send_back_json(
                                    boost::json::value{
                                            {"cmdId",     cmdId},
                                            {"packageId", packageId},
                                            {"msg",       "error"},
                                            {"error",     "move step (forward||distance) not find"},
                                            {"result",    false},
                                    }
                            );
                            return;
                        }
                        bool good = true;
                        auto moveStepForward = getFromJsonObject<int32_t>(json_o, "forward", good);
                        auto moveStepDistance = getFromJsonObject<int32_t>(json_o, "distance", good);
                        if (!good) {
                            BOOST_LOG_TRIVIAL(warning) << "move step getFromJsonObject fail" << jsv;
                            send_back_json(
                                    boost::json::value{
                                            {"msg",    "error"},
                                            {"error",  "(forward||distance) getFromJsonObject fail"},
                                            {"result", false},
                                    }
                            );
                            return;
                        }
                        switch (moveStepForward) {
                            case 1:
                                // up
                                BOOST_LOG_TRIVIAL(info) << "move up " << moveStepDistance;
                                send_back_json(
                                        boost::json::value{
                                                {"cmdId",     cmdId},
                                                {"packageId", packageId},
                                                {"msg",       "up"},
                                                {"result",    true},
                                        }
                                );
                                break;
                            case 2:
                                // down
                                BOOST_LOG_TRIVIAL(info) << "move down " << moveStepDistance;
                                send_back_json(
                                        boost::json::value{
                                                {"cmdId",     cmdId},
                                                {"packageId", packageId},
                                                {"msg",       "down"},
                                                {"result",    true},
                                        }
                                );
                                break;
                            case 3:
                                // left
                                BOOST_LOG_TRIVIAL(info) << "move left " << moveStepDistance;
                                send_back_json(
                                        boost::json::value{
                                                {"cmdId",     cmdId},
                                                {"packageId", packageId},
                                                {"msg",       "left"},
                                                {"result",    true},
                                        }
                                );
                                break;
                            case 4:
                                // right
                                BOOST_LOG_TRIVIAL(info) << "move right " << moveStepDistance;
                                send_back_json(
                                        boost::json::value{
                                                {"cmdId",     cmdId},
                                                {"packageId", packageId},
                                                {"msg",       "right"},
                                                {"result",    true},
                                        }
                                );
                                break;
                            case 5:
                                // forward
                                BOOST_LOG_TRIVIAL(info) << "move forward " << moveStepDistance;
                                send_back_json(
                                        boost::json::value{
                                                {"cmdId",     cmdId},
                                                {"packageId", packageId},
                                                {"msg",       "forward"},
                                                {"result",    true},
                                        }
                                );
                                break;
                            case 6:
                                // back
                                BOOST_LOG_TRIVIAL(info) << "move back " << moveStepDistance;
                                send_back_json(
                                        boost::json::value{
                                                {"cmdId",     cmdId},
                                                {"packageId", packageId},
                                                {"msg",       "back"},
                                                {"result",    true},
                                        }
                                );
                                break;
                            default:
                                // ignore
                                BOOST_LOG_TRIVIAL(warning) << "move ignore " << jsv;
                                send_back_json(
                                        boost::json::value{
                                                {"cmdId",     cmdId},
                                                {"packageId", packageId},
                                                {"msg",       "move ignore"},
                                                {"result",    false},
                                        }
                                );
                                break;
                        }
                    }
                    break;
                case 14:
                    // rotate
                    BOOST_LOG_TRIVIAL(info) << "rotate ";
                    {
                        if (!json_o.contains("rotate") && !json_o.contains("rote")) {
                            BOOST_LOG_TRIVIAL(warning) << "rotate contains fail " << jsv;
                            send_back_json(
                                    boost::json::value{
                                            {"cmdId",     cmdId},
                                            {"packageId", packageId},
                                            {"msg",       "error"},
                                            {"error",     "rotate (rotate||rote) not find"},
                                            {"result",    false},
                                    }
                            );
                            return;
                        }
                        bool good = true;
                        auto rotate = getFromJsonObject<int32_t>(json_o, "rotate", good);
                        auto rote = getFromJsonObject<int32_t>(json_o, "rote", good);
                        if (!good) {
                            BOOST_LOG_TRIVIAL(warning) << "rotate getFromJsonObject fail" << jsv;
                            send_back_json(
                                    boost::json::value{
                                            {"msg",    "error"},
                                            {"error",  "(rotate||rote) getFromJsonObject fail"},
                                            {"result", false},
                                    }
                            );
                            return;
                        }
                        switch (rotate) {
                            case 1:
                                // cw
                                BOOST_LOG_TRIVIAL(info) << "rotate cw " << rote;
                                send_back_json(
                                        boost::json::value{
                                                {"cmdId",     cmdId},
                                                {"packageId", packageId},
                                                {"msg",       "rotate cw"},
                                                {"result",    true},
                                        }
                                );
                                break;
                            case 2:
                                // ccw
                                BOOST_LOG_TRIVIAL(info) << "rotate ccw " << rote;
                                send_back_json(
                                        boost::json::value{
                                                {"cmdId",     cmdId},
                                                {"packageId", packageId},
                                                {"msg",       "rotate ccw"},
                                                {"result",    true},
                                        }
                                );
                                break;
                            default:
                                // ignore
                                BOOST_LOG_TRIVIAL(info) << "rotate ignore " << jsv;
                                send_back_json(
                                        boost::json::value{
                                                {"cmdId",     cmdId},
                                                {"packageId", packageId},
                                                {"msg",       "rotate ignore"},
                                                {"result",    false},
                                        }
                                );
                                break;
                        }
                    }
                    break;
                default:
                    // ignore
                    BOOST_LOG_TRIVIAL(warning) << "ignore " << jsv;
                    send_back_json(
                            boost::json::value{
                                    {"cmdId",     cmdId},
                                    {"packageId", packageId},
                                    {"msg",       "ignore"},
                                    {"result",    false},
                            }
                    );
                    break;
            }
            return;
        } catch (std::exception &e) {
            std::cerr << "CommandService::process_message \n" << e.what();
            BOOST_LOG_TRIVIAL(error) << "CommandService::process_message \n" << e.what();
            // ignore
            send_back_json(
                    boost::json::value{
                            {"msg",    "exception"},
                            {"error",  e.what()},
                            {"result", false},
                    }
            );
            return;
        } catch (...) {
            // ignore
            return;
        }
    }


} // OwlCommandService