// jeremie

#ifndef OWLACCESSTERMINAL_COMMANDSERVICE_H
#define OWLACCESSTERMINAL_COMMANDSERVICE_H

#include <memory>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/json.hpp>
#include <boost/system/error_code.hpp>
#include <boost/utility/string_view.hpp>


namespace OwlCommandService {

#define UDP_Package_Max_Size (1024*1024*6)

    class CommandService : public std::enable_shared_from_this<CommandService> {
    public:
        CommandService(boost::asio::io_context &_ioc,
                       const boost::asio::ip::udp::endpoint &endpoint)
                : executor(boost::asio::make_strand(_ioc)),
                  udp_socket(_ioc, endpoint),
                  receive_buffer_(),
                  json_storage_(),
                  json_storage_resource(json_storage_.data(), UDP_Package_Max_Size) {

            json_parse_options.allow_comments = true;
            json_parse_options.allow_trailing_commas = true;
            json_parse_options.max_depth = 5;
            //  boost::asio::ip::udp::endpoint _endpoint(boost::asio::ip::udp::v4(), 2333);
        }

    private:
        boost::asio::executor executor;
        boost::asio::ip::udp::socket udp_socket;

        // the max udp receive buffer, it used to receive package from remove client
        boost::array<char, UDP_Package_Max_Size> receive_buffer_;
        // the remote client endpoint , it used to record where the package come from
        boost::asio::ip::udp::endpoint remote_endpoint_;

        boost::json::parse_options json_parse_options;
        boost::array<unsigned char, UDP_Package_Max_Size> json_storage_;
        boost::json::static_resource json_storage_resource;

    public:
        void start() {
            next_receive();
        }

    private:
        void next_receive();

        void process_message(std::size_t bytes_transferred);

        void send_back(std::string &&json_string);

        void send_back_json(const boost::json::value &json_value);


    };

    template<typename T>
    T get_from_json_object(boost::json::object &v, boost::string_view key, bool &good) {
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
