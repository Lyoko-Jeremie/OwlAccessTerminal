// jeremie

#ifndef OWLACCESSTERMINAL_CONFIGLOADER_H
#define OWLACCESSTERMINAL_CONFIGLOADER_H

#include <memory>
#include <sstream>
#include <boost/filesystem/fstream.hpp>
#include <boost/system.hpp>
#include <boost/json.hpp>
#include <boost/utility/string_view.hpp>

namespace OwlConfigLoader {

    struct ConfigEmbedWebServer {
        std::string doc_root;
        std::string index_file_of_root;
        std::string backend_json_string;
        std::string allowFileExtList;
    };

    struct Config {
        int CommandServiceUdpPort = 23333;
        int ImageServiceTcpPort = 23332;
        int EmbedWebServerHttpPort = 81;

        int airplane_fly_serial_baud_rate = 0;
        std::string airplane_fly_serial_addr;
        std::string camera_addr_1;
        std::string camera_addr_2;

        ConfigEmbedWebServer embedWebServer;
    };

    class ConfigLoader : public std::enable_shared_from_this<ConfigLoader> {
    public:

        Config config;

        void print() {}

        void init(const std::string &filePath) {
            config = parse_json(load_json_file(filePath));
        }

        static boost::json::value load_json_file(const std::string &filePath);

        Config parse_json(const boost::json::value &&json_v);

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

        boost::json::object getObj(const boost::json::object &v, boost::string_view key) {
            try {
                if (!v.contains(key)) {
                    return {};
                }
                auto oo = v.at(key);
                return oo.as_object();
            } catch (std::exception &e) {
                return {};
            }
        }
    };


} // OwlConfigLoader

#endif //OWLACCESSTERMINAL_CONFIGLOADER_H
