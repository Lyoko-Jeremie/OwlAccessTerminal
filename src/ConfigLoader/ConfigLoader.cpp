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
            BOOST_LOG_TRIVIAL(info) << "string2VideoCaptureAPI it: " << s;
            return it->second;
        } else {
            BOOST_LOG_TRIVIAL(info) << "string2VideoCaptureAPI else it: CAP_ANY";
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
            BOOST_LOG_TRIVIAL(error) << "load_json_file (!f)";
            return nullptr;
        }
        ss << f.rdbuf();
        f.close();
        boost::json::stream_parser p;
        p.write(ss.str(), ec);
        if (ec) {
            BOOST_LOG_TRIVIAL(error) << "load_json_file (ec) " << ec.what();
            return nullptr;
        }
        p.finish(ec);
        if (ec) {
            BOOST_LOG_TRIVIAL(error) << "load_json_file (ec) " << ec.what();
            return nullptr;
        }
        return p.release();
    }

    Config ConfigLoader::parse_json(const boost::json::value &&json_v) {
        const auto &root = json_v.as_object();

        {
            std::stringstream ss;
            pretty_print(ss, root);
            auto s = ss.str();
            BOOST_LOG_TRIVIAL(info) << "parse_json from : \n" << s;
            // BOOST_LOG_TRIVIAL(info) << "parse_json from : \n" << boost::json::serialize(root);
        }

        Config config_;

        config_.CommandServiceUdpPort = get(root, "CommandServiceUdpPort", config_.CommandServiceUdpPort);
        config_.CommandServiceHttpPort = get(root, "CommandServiceHttpPort", config_.CommandServiceHttpPort);
        config_.ImageServiceTcpPort = get(root, "ImageServiceTcpPort", config_.ImageServiceTcpPort);
        config_.EmbedWebServerHttpPort = get(root, "EmbedWebServerHttpPort", config_.EmbedWebServerHttpPort);
        config_.ImageServiceHttpPort = get(root, "ImageServiceHttpPort", config_.ImageServiceHttpPort);
        config_.airplane_fly_serial_baud_rate = get(root, "airplane_fly_serial_baud_rate",
                                                    config_.airplane_fly_serial_baud_rate);
        config_.airplane_fly_serial_addr = get(root, "airplane_fly_serial_addr", config_.airplane_fly_serial_addr);

        config_.camera_addr_1 = getCameraAddr(root, "camera_addr_1", std::move(config_.camera_addr_1));
        config_.camera_1_VideoCaptureAPI = get(root, "camera_1_VideoCaptureAPI", config_.camera_1_VideoCaptureAPI);
        config_.camera_1_w = get(root, "camera_1_w", config_.camera_1_w);
        config_.camera_1_h = get(root, "camera_1_h", config_.camera_1_h);
        config_.camera_addr_2 = getCameraAddr(root, "camera_addr_2", std::move(config_.camera_addr_2));
        config_.camera_2_VideoCaptureAPI = get(root, "camera_2_VideoCaptureAPI", config_.camera_2_VideoCaptureAPI);
        config_.camera_2_w = get(root, "camera_2_w", config_.camera_2_w);
        config_.camera_2_h = get(root, "camera_2_h", config_.camera_2_h);

        config_.downCameraId = get(root, "downCameraId", config_.downCameraId);
        config_.frontCameraId = get(root, "frontCameraId", config_.frontCameraId);

        config_.cmd_nmcli_path = get(root, "cmd_nmcli_path", config_.cmd_nmcli_path);
        config_.cmd_bash_path = get(root, "cmd_bash_path", config_.cmd_bash_path);

        if (root.contains("embedWebServer")) {
            auto embedWebServer = getObj(root, "embedWebServer");
            config_.embedWebServer.doc_root = get(embedWebServer, "doc_root", config_.embedWebServer.doc_root);
            config_.embedWebServer.index_file_of_root = get(embedWebServer, "index_file_of_root",
                                                            config_.embedWebServer.index_file_of_root);
            config_.embedWebServer.backend_json_string = get(embedWebServer, "backend_json_string",
                                                             config_.embedWebServer.backend_json_string);
            config_.embedWebServer.allowFileExtList = get(embedWebServer, "allowFileExtList",
                                                          config_.embedWebServer.allowFileExtList);
        }

        return config_;
    }
} // OwlConfigLoader