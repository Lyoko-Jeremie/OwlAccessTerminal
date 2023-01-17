// jeremie

#ifndef OWLACCESSTERMINAL_CONFIGLOADER_H
#define OWLACCESSTERMINAL_CONFIGLOADER_H

#include <memory>
#include <sstream>
#include <variant>
#include <functional>
#include <boost/filesystem/fstream.hpp>
#include <boost/system.hpp>
#include <boost/json.hpp>
#include <boost/utility/string_view.hpp>
#include <boost/log/trivial.hpp>

namespace OwlConfigLoader {

    struct ConfigEmbedWebServer {
        std::string doc_root{"./html"};
        std::string index_file_of_root{"index.html"};
        std::string backend_json_string{"{}"};
        std::string allowFileExtList{"htm html js json jpg jpeg png bmp gif ico svg css"};
    };

    using CameraAddrType_1 = int;
    using CameraAddrType_2 = std::string;
    using CameraAddrType = std::variant<CameraAddrType_1, CameraAddrType_2>;

    struct Config {

        int CommandServiceUdpPort = 23333;
        int CommandServiceHttpPort = 23338;
        int ImageServiceTcpPort = 23332;
        int ImageServiceHttpPort = 23331;
        int EmbedWebServerHttpPort = 81;

        int airplane_fly_serial_baud_rate = 115200;
        std::string airplane_fly_serial_addr = "/dev/ttys1";
        CameraAddrType camera_addr_1 = CameraAddrType{0};
        CameraAddrType camera_addr_2 = CameraAddrType{1};

        ConfigEmbedWebServer embedWebServer;
    };

    const auto helperCameraAddr2String = []<typename T>(T &a) -> std::string {
        if constexpr (std::is_same_v<T, std::string>) { return a; }
        else if constexpr (std::is_same_v<T, int>) { return std::to_string(a); }
        else { return ""; }
    };


    class ConfigLoader : public std::enable_shared_from_this<ConfigLoader> {
    public:

        Config config;

        void print() {
            BOOST_LOG_TRIVIAL(info)
                << "\n"
                << "\n" << "ConfigLoader config:"
                << "\n" << "CommandServiceUdpPort " << config.CommandServiceUdpPort
                << "\n" << "CommandServiceHttpPort " << config.CommandServiceHttpPort
                << "\n" << "ImageServiceTcpPort " << config.ImageServiceTcpPort
                << "\n" << "ImageServiceHttpPort " << config.ImageServiceHttpPort
                << "\n" << "EmbedWebServerHttpPort " << config.EmbedWebServerHttpPort
                << "\n" << "airplane_fly_serial_baud_rate " << config.airplane_fly_serial_baud_rate
                << "\n" << "airplane_fly_serial_addr " << config.airplane_fly_serial_addr
                << "\n" << "camera_addr_1 " << std::visit(helperCameraAddr2String, config.camera_addr_1)
                << "\n" << "camera_addr_2 " << std::visit(helperCameraAddr2String, config.camera_addr_2)
                << "\n" << "ConfigEmbedWebServer :"
                << "\n" << "\t doc_root " << config.embedWebServer.doc_root
                << "\n" << "\t index_file_of_root " << config.embedWebServer.index_file_of_root
                << "\n" << "\t backend_json_string " << config.embedWebServer.backend_json_string
                << "\n" << "\t allowFileExtList " << config.embedWebServer.allowFileExtList
                << "";
        }

        void init(const std::string &filePath) {
            auto j = load_json_file(filePath);
            BOOST_LOG_TRIVIAL(info) << "j.is_object() " << j.is_object() << "\t"
                                    << "j.kind() " << boost::json::to_string(j.kind());
            if (j.is_object()) {
                config = parse_json(j.as_object());
            } else {
                BOOST_LOG_TRIVIAL(error)
                    << "ConfigLoader: config file not exit OR cannot load config file OR config file invalid.";
            }
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

        CameraAddrType getCameraAddr(const boost::json::object &v, boost::string_view key, CameraAddrType &&d) {
            try {
                if (!v.contains(key)) {
                    return d;
                }
            } catch (std::exception &e) {
                return d;
            }
            try {
                auto rr = boost::json::try_value_to<CameraAddrType_1>(v.at(key));
                if (rr.has_value()) { return CameraAddrType{rr.value()}; }
            } catch (std::exception &e) {
                return d;
            }
            try {
                auto rr = boost::json::try_value_to<CameraAddrType_2>(v.at(key));
                if (rr.has_value()) { return CameraAddrType{rr.value()}; }
            } catch (std::exception &e) {
                return d;
            }
            return d;
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
