// jeremie

#include "ConfigLoader.h"

namespace OwlCameraConfig {
    const std::map<std::string, VideoCaptureAPIs> VideoCaptureAPITable{
            {"CAP_ANY",           VideoCaptureAPIs::CAP_ANY},
            {"CAP_VFW",           VideoCaptureAPIs::CAP_VFW},
            {"CAP_V4L",           VideoCaptureAPIs::CAP_V4L},
            {"CAP_V4L2",          VideoCaptureAPIs::CAP_V4L2},
            {"CAP_FIREWIRE",      VideoCaptureAPIs::CAP_FIREWIRE},
            {"CAP_FIREWARE",      VideoCaptureAPIs::CAP_FIREWARE},
            {"CAP_IEEE1394",      VideoCaptureAPIs::CAP_IEEE1394},
            {"CAP_DC1394",        VideoCaptureAPIs::CAP_DC1394},
            {"CAP_CMU1394",       VideoCaptureAPIs::CAP_CMU1394},
            {"CAP_QT",            VideoCaptureAPIs::CAP_QT},
            {"CAP_UNICAP",        VideoCaptureAPIs::CAP_UNICAP},
            {"CAP_DSHOW",         VideoCaptureAPIs::CAP_DSHOW},
            {"CAP_PVAPI",         VideoCaptureAPIs::CAP_PVAPI},
            {"CAP_OPENNI",        VideoCaptureAPIs::CAP_OPENNI},
            {"CAP_OPENNI_ASUS",   VideoCaptureAPIs::CAP_OPENNI_ASUS},
            {"CAP_ANDROID",       VideoCaptureAPIs::CAP_ANDROID},
            {"CAP_XIAPI",         VideoCaptureAPIs::CAP_XIAPI},
            {"CAP_AVFOUNDATION",  VideoCaptureAPIs::CAP_AVFOUNDATION},
            {"CAP_GIGANETIX",     VideoCaptureAPIs::CAP_GIGANETIX},
            {"CAP_MSMF",          VideoCaptureAPIs::CAP_MSMF},
            {"CAP_WINRT",         VideoCaptureAPIs::CAP_WINRT},
            {"CAP_INTELPERC",     VideoCaptureAPIs::CAP_INTELPERC},
            {"CAP_REALSENSE",     VideoCaptureAPIs::CAP_REALSENSE},
            {"CAP_OPENNI2",       VideoCaptureAPIs::CAP_OPENNI2},
            {"CAP_OPENNI2_ASUS",  VideoCaptureAPIs::CAP_OPENNI2_ASUS},
            {"CAP_OPENNI2_ASTRA", VideoCaptureAPIs::CAP_OPENNI2_ASTRA},
            {"CAP_GPHOTO2",       VideoCaptureAPIs::CAP_GPHOTO2},
            {"CAP_GSTREAMER",     VideoCaptureAPIs::CAP_GSTREAMER},
            {"CAP_FFMPEG",        VideoCaptureAPIs::CAP_FFMPEG},
            {"CAP_IMAGES",        VideoCaptureAPIs::CAP_IMAGES},
            {"CAP_ARAVIS",        VideoCaptureAPIs::CAP_ARAVIS},
            {"CAP_OPENCV_MJPEG",  VideoCaptureAPIs::CAP_OPENCV_MJPEG},
            {"CAP_INTEL_MFX",     VideoCaptureAPIs::CAP_INTEL_MFX},
            {"CAP_XINE",          VideoCaptureAPIs::CAP_XINE},
            {"CAP_UEYE",          VideoCaptureAPIs::CAP_UEYE},
    };

    VideoCaptureAPIs string2VideoCaptureAPI(const std::string &s) {
        auto it = VideoCaptureAPITable.find(s);
        if (it != VideoCaptureAPITable.end()) {
            BOOST_LOG_OWL(info) << "string2VideoCaptureAPI it: " << s;
            return it->second;
        } else {
            BOOST_LOG_OWL(info) << "string2VideoCaptureAPI else it: CAP_ANY";
            return VideoCaptureAPIs::CAP_ANY;
        }
    }
}


namespace OwlConfigLoader {


    // https://www.boost.org/doc/libs/1_81_0/libs/json/doc/html/json/examples.html
    void pretty_print(std::ostream &os, boost::json::value const &jv, std::string *indent = nullptr) {
        std::string indent_;
        if (!indent)
            indent = &indent_;
        switch (jv.kind()) {
            case boost::json::kind::object: {
                os << "{\n";
                indent->append(4, ' ');
                auto const &obj = jv.get_object();
                if (!obj.empty()) {
                    auto it = obj.begin();
                    for (;;) {
                        os << *indent << boost::json::serialize(it->key()) << " : ";
                        pretty_print(os, it->value(), indent);
                        if (++it == obj.end())
                            break;
                        os << ",\n";
                    }
                }
                os << "\n";
                indent->resize(indent->size() - 4);
                os << *indent << "}";
                break;
            }

            case boost::json::kind::array: {
                os << "[\n";
                indent->append(4, ' ');
                auto const &arr = jv.get_array();
                if (!arr.empty()) {
                    auto it = arr.begin();
                    for (;;) {
                        os << *indent;
                        pretty_print(os, *it, indent);
                        if (++it == arr.end())
                            break;
                        os << ",\n";
                    }
                }
                os << "\n";
                indent->resize(indent->size() - 4);
                os << *indent << "]";
                break;
            }

            case boost::json::kind::string: {
                os << boost::json::serialize(jv.get_string());
                break;
            }

            case boost::json::kind::uint64:
                os << jv.get_uint64();
                break;

            case boost::json::kind::int64:
                os << jv.get_int64();
                break;

            case boost::json::kind::double_:
                os << jv.get_double();
                break;

            case boost::json::kind::bool_:
                if (jv.get_bool())
                    os << "true";
                else
                    os << "false";
                break;

            case boost::json::kind::null:
                os << "null";
                break;
        }

        if (indent->empty())
            os << "\n";
    }

    boost::json::value ConfigLoader::load_json_file(const std::string &filePath) {
        boost::system::error_code ec;
        std::stringstream ss;
        boost::filesystem::ifstream f(filePath);
        if (!f) {
            BOOST_LOG_OWL(error) << "load_json_file (!f)";
            return nullptr;
        }
        ss << f.rdbuf();
        f.close();
        boost::json::stream_parser p;
        p.write(ss.str(), ec);
        if (ec) {
            BOOST_LOG_OWL(error) << "load_json_file (ec) " << ec.what();
            return nullptr;
        }
        p.finish(ec);
        if (ec) {
            BOOST_LOG_OWL(error) << "load_json_file (ec) " << ec.what();
            return nullptr;
        }
        return p.release();
    }

    boost::shared_ptr<Config> ConfigLoader::parse_json(const boost::json::value &&json_v) {
        const auto &root = json_v.as_object();

        {
            std::stringstream ss;
            pretty_print(ss, root);
            auto s = ss.str();
            BOOST_LOG_OWL(info) << "parse_json from : \n" << s;
            // BOOST_LOG_OWL(info) << "parse_json from : \n" << boost::json::serialize(root);
        }

        boost::shared_ptr<Config> _config_ = boost::make_shared<Config>();
        auto &config = *_config_;

        config.CommandServiceUdpPort = get(root, "CommandServiceUdpPort", config.CommandServiceUdpPort);
        config.CommandServiceHttpPort = get(root, "CommandServiceHttpPort", config.CommandServiceHttpPort);
        config.ImageServiceTcpPort = get(root, "ImageServiceTcpPort", config.ImageServiceTcpPort);
        config.EmbedWebServerHttpPort = get(root, "EmbedWebServerHttpPort", config.EmbedWebServerHttpPort);
        config.ImageServiceHttpPort = get(root, "ImageServiceHttpPort", config.ImageServiceHttpPort);
        config.airplane_fly_serial_baud_rate = get(root, "airplane_fly_serial_baud_rate",
                                                   config.airplane_fly_serial_baud_rate);
        config.airplane_fly_serial_addr = get(root, "airplane_fly_serial_addr", config.airplane_fly_serial_addr);

        config.camera_addr_1 = getCameraAddr(root, "camera_addr_1", std::move(config.camera_addr_1));
        config.camera_1_VideoCaptureAPI = get(root, "camera_1_VideoCaptureAPI", config.camera_1_VideoCaptureAPI);
        config.camera_1_w = get(root, "camera_1_w", config.camera_1_w);
        config.camera_1_h = get(root, "camera_1_h", config.camera_1_h);
        config.camera_addr_2 = getCameraAddr(root, "camera_addr_2", std::move(config.camera_addr_2));
        config.camera_2_VideoCaptureAPI = get(root, "camera_2_VideoCaptureAPI", config.camera_2_VideoCaptureAPI);
        config.camera_2_w = get(root, "camera_2_w", config.camera_2_w);
        config.camera_2_h = get(root, "camera_2_h", config.camera_2_h);

        getAtomic(root, "downCameraId", config.downCameraId);
        getAtomic(root, "frontCameraId", config.frontCameraId);

        config.cmd_bash_path = get(root, "cmd_bash_path", config.cmd_bash_path);

        config.js_map_calc_file = get(root, "js_map_calc_file", config.js_map_calc_file);
        config.js_map_calc_function_name = get(root, "js_map_calc_function_name", config.js_map_calc_function_name);

#ifdef EnableWebStaticModule
        if (root.contains("embedWebServer")) {
            auto embedWebServer = getObj(root, "embedWebServer");
            config.embedWebServer.doc_root = get(embedWebServer, "doc_root", config.embedWebServer.doc_root);
            config.embedWebServer.index_file_of_root = get(embedWebServer, "index_file_of_root",
                                                           config.embedWebServer.index_file_of_root);
            config.embedWebServer.backend_json_string = get(embedWebServer, "backend_json_string",
                                                            config.embedWebServer.backend_json_string);
            config.embedWebServer.allowFileExtList = get(embedWebServer, "allowFileExtList",
                                                         config.embedWebServer.allowFileExtList);
        }

        if (root.contains("wifiCmd")) {
            auto wifiCmd = getObj(root, "wifiCmd");
            config.wifiCmd.enable = get(wifiCmd, "enable", config.wifiCmd.enable);
            config.wifiCmd.ap = get(wifiCmd, "ap", config.wifiCmd.ap);
            config.wifiCmd.connect = get(wifiCmd, "connect", config.wifiCmd.connect);
            config.wifiCmd.scan = get(wifiCmd, "scan", config.wifiCmd.scan);
            config.wifiCmd.showHotspotPassword = get(wifiCmd, "showHotspotPassword",
                                                     config.wifiCmd.showHotspotPassword);
            config.wifiCmd.getWlanDeviceState = get(wifiCmd, "getWlanDeviceState",
                                                    config.wifiCmd.getWlanDeviceState);
            config.wifiCmd.listWlanDevice = get(wifiCmd, "listWlanDevice",
                                                config.wifiCmd.listWlanDevice);
        }
#endif // EnableWebStaticModule

        return _config_;
    }

    void ConfigLoader::print() {
        auto &config = *config_;
        BOOST_LOG_OWL(info)
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
            << "\n" << "camera_1_VideoCaptureAPI " << config.camera_1_VideoCaptureAPI
            << "\n" << "camera_1_w " << config.camera_1_w
            << "\n" << "camera_1_h " << config.camera_1_h
            << "\n" << "camera_addr_2 " << std::visit(helperCameraAddr2String, config.camera_addr_2)
            << "\n" << "camera_2_VideoCaptureAPI " << config.camera_2_VideoCaptureAPI
            << "\n" << "camera_2_w " << config.camera_2_w
            << "\n" << "camera_2_h " << config.camera_2_h
            << "\n" << "downCameraId " << config.downCameraId.load()
            << "\n" << "frontCameraId " << config.frontCameraId.load()
            << "\n" << "cmd_bash_path " << config.cmd_bash_path
            << "\n" << "js_map_calc_file " << config.js_map_calc_file
            << "\n" << "js_map_calc_function_name " << config.js_map_calc_function_name
            #ifdef EnableWebStaticModule
            << "\n" << "ConfigEmbedWebServer :"
            << "\n" << "\t doc_root " << config.embedWebServer.doc_root
            << "\n" << "\t index_file_of_root " << config.embedWebServer.index_file_of_root
            << "\n" << "\t backend_json_string " << config.embedWebServer.backend_json_string
            << "\n" << "\t allowFileExtList " << config.embedWebServer.allowFileExtList
            << "\n" << "wifiCmd :"
            << "\n" << "\t enable " << config.wifiCmd.enable
            << "\n" << "\t ap " << config.wifiCmd.ap
            << "\n" << "\t connect " << config.wifiCmd.connect
            << "\n" << "\t scan " << config.wifiCmd.scan
            << "\n" << "\t showHotspotPassword " << config.wifiCmd.showHotspotPassword
            << "\n" << "\t getWlanDeviceState " << config.wifiCmd.getWlanDeviceState
            << "\n" << "\t listWlanDevice " << config.wifiCmd.listWlanDevice
            #endif // EnableWebStaticModule
            << "";
    }

} // OwlConfigLoader