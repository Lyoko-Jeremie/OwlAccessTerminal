// jeremie

#ifndef OWLACCESSTERMINAL_CONFIGLOADER_H
#define OWLACCESSTERMINAL_CONFIGLOADER_H

#include <memory>
#include <sstream>
#include <variant>
#include <functional>
#include <map>
#include <tuple>
#include <limits>
#include <atomic>
#include <boost/filesystem/fstream.hpp>
#include <boost/system.hpp>
#include <boost/json.hpp>
#include <boost/utility/string_view.hpp>
#include <boost/log/trivial.hpp>

namespace OwlCameraConfig {
    // from cv::VideoCaptureAPIs
    enum VideoCaptureAPIs {
        CAP_ANY = 0,                     //!< Auto detect == 0
        CAP_VFW = 200,                   //!< Video For Windows (obsolete, removed)
        CAP_V4L = 200,                   //!< V4L/V4L2 capturing support
        CAP_V4L2 = CAP_V4L,              //!< Same as CAP_V4L
        CAP_FIREWIRE = 300,              //!< IEEE 1394 drivers
        CAP_FIREWARE = CAP_FIREWIRE,     //!< Same value as CAP_FIREWIRE
        CAP_IEEE1394 = CAP_FIREWIRE,     //!< Same value as CAP_FIREWIRE
        CAP_DC1394 = CAP_FIREWIRE,       //!< Same value as CAP_FIREWIRE
        CAP_CMU1394 = CAP_FIREWIRE,      //!< Same value as CAP_FIREWIRE
        CAP_QT = 500,                    //!< QuickTime (obsolete, removed)
        CAP_UNICAP = 600,                //!< Unicap drivers (obsolete, removed)
        CAP_DSHOW = 700,                 //!< DirectShow (via videoInput)
        CAP_PVAPI = 800,                 //!< PvAPI, Prosilica GigE SDK
        CAP_OPENNI = 900,                //!< OpenNI (for Kinect)
        CAP_OPENNI_ASUS = 910,           //!< OpenNI (for Asus Xtion)
        CAP_ANDROID = 1000,              //!< Android - not used
        CAP_XIAPI = 1100,                //!< XIMEA Camera API
        CAP_AVFOUNDATION = 1200,         //!< AVFoundation framework for iOS (OS X Lion will have the same API)
        CAP_GIGANETIX = 1300,            //!< Smartek Giganetix GigEVisionSDK
        CAP_MSMF = 1400,                 //!< Microsoft Media Foundation (via videoInput)
        CAP_WINRT = 1410,                //!< Microsoft Windows Runtime using Media Foundation
        CAP_INTELPERC = 1500,            //!< RealSense (former Intel Perceptual Computing SDK)
        CAP_REALSENSE = 1500,            //!< Synonym for CAP_INTELPERC
        CAP_OPENNI2 = 1600,              //!< OpenNI2 (for Kinect)
        CAP_OPENNI2_ASUS = 1610,         //!< OpenNI2 (for Asus Xtion and Occipital Structure sensors)
        CAP_OPENNI2_ASTRA = 1620,        //!< OpenNI2 (for Orbbec Astra)
        CAP_GPHOTO2 = 1700,              //!< gPhoto2 connection
        CAP_GSTREAMER = 1800,            //!< GStreamer
        CAP_FFMPEG = 1900,               //!< Open and record video file or stream using the FFMPEG library
        CAP_IMAGES = 2000,               //!< OpenCV Image Sequence (e.g. img_%02d.jpg)
        CAP_ARAVIS = 2100,               //!< Aravis SDK
        CAP_OPENCV_MJPEG = 2200,         //!< Built-in OpenCV MotionJPEG codec
        CAP_INTEL_MFX = 2300,            //!< Intel MediaSDK
        CAP_XINE = 2400,                 //!< XINE engine (Linux)
        CAP_UEYE = 2500,                 //!< uEye Camera API
    };

    extern const std::map<std::string, VideoCaptureAPIs> VideoCaptureAPITable;

    VideoCaptureAPIs string2VideoCaptureAPI(const std::string &s);

} // OwlCameraConfig

namespace OwlConfigLoader {

    struct ConfigEmbedWebServer {
        std::string doc_root{"./html"};
        std::string index_file_of_root{"index.html"};
        std::string backend_json_string{"{}"};
        std::string allowFileExtList{"htm html js json jpg jpeg png bmp gif ico svg css"};
    };

    using CameraAddrType_1 = int;
    const auto CameraAddrType_1_Placeholder = std::numeric_limits<CameraAddrType_1>::max();
    using CameraAddrType_2 = std::string;
    const auto CameraAddrType_2_Placeholder = std::string{};
    using CameraAddrType = std::variant<CameraAddrType_1, CameraAddrType_2>;
    const auto Camera_VideoCaptureAPI_Placeholder = std::string{};
    const auto Camera_VideoCaptureAPI_Default = std::string{"CAP_ANY"};

    struct WifiCmd {
        std::string enable{R"(nmcli wifi on)"};
        std::string ap{R"(nmcli dev wifi hotspot ssid "<SSID>" password "<PWD>" | cat)"};
        std::string connect{R"(nmcli dev wifi connect "<BSSID>" password "<PWD>" | cat)"};
        std::string scan{R"(nmcli dev wifi list | cat)"};
        std::string showHotspotPassword{R"(nmcli dev wifi show-password | cat)"};
        std::string getWlanDeviceState{R"(nmcli dev wifi list ifname "<DEVICE_NAME>" | cat)"};
        std::string listWlanDevice{R"(nmcli dev status | grep " wifi ")"};
    };

    struct Config {

        int CommandServiceUdpPort = 23333;
        int CommandServiceHttpPort = 23338;
        int ImageServiceTcpPort = 23332;
        int ImageServiceHttpPort = 23331;
        int EmbedWebServerHttpPort = 81;

        int airplane_fly_serial_baud_rate = 115200;
        std::string airplane_fly_serial_addr = "/dev/ttyS1";

        CameraAddrType camera_addr_1 = CameraAddrType{0};
        std::string camera_1_VideoCaptureAPI = Camera_VideoCaptureAPI_Default;
        int camera_1_w = 1920;
        int camera_1_h = 1080;
        CameraAddrType camera_addr_2 = CameraAddrType{1};
        std::string camera_2_VideoCaptureAPI = Camera_VideoCaptureAPI_Default;
        int camera_2_w = 1920;
        int camera_2_h = 1080;

        std::atomic_int downCameraId{1};
        std::atomic_int frontCameraId{2};

        std::string cmd_nmcli_path = "nmcli";
        std::string cmd_bash_path = "/bin/bash";

        ConfigEmbedWebServer embedWebServer;

        WifiCmd wifiCmd;

    };

    const auto helperCameraAddr2String = []<typename T>(T &a) -> std::string {
        if constexpr (std::is_same_v<T, std::string>) { return a; }
        else if constexpr (std::is_same_v<T, int>) { return std::to_string(a); }
        else { return ""; }
    };


    class ConfigLoader : public std::enable_shared_from_this<ConfigLoader> {
    public:

        void print() {
            auto &config = *config_;
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
                << "\n" << "camera_1_VideoCaptureAPI " << config.camera_1_VideoCaptureAPI
                << "\n" << "camera_1_w " << config.camera_1_w
                << "\n" << "camera_1_h " << config.camera_1_h
                << "\n" << "camera_addr_2 " << std::visit(helperCameraAddr2String, config.camera_addr_2)
                << "\n" << "camera_2_VideoCaptureAPI " << config.camera_2_VideoCaptureAPI
                << "\n" << "camera_2_w " << config.camera_2_w
                << "\n" << "camera_2_h " << config.camera_2_h
                << "\n" << "downCameraId " << config.downCameraId.load()
                << "\n" << "frontCameraId " << config.frontCameraId.load()
                << "\n" << "cmd_nmcli_path " << config.cmd_nmcli_path
                << "\n" << "cmd_bash_path " << config.cmd_bash_path
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
                << "";
        }

        void init(const std::string &filePath) {
            auto j = load_json_file(filePath);
            BOOST_LOG_TRIVIAL(info) << "j.is_object() " << j.is_object() << "\t"
                                    << "j.kind() " << boost::json::to_string(j.kind());
            if (j.is_object()) {
                config_ = std::move(parse_json(j.as_object()));
            } else {
                BOOST_LOG_TRIVIAL(error)
                    << "ConfigLoader: config file not exit OR cannot load config file OR config file invalid.";
            }
        }

        Config &config() {
            return *config_;
        }

    private:
        std::shared_ptr<Config> config_ = std::make_shared<Config>();

    private:

        static boost::json::value load_json_file(const std::string &filePath);

        std::shared_ptr<Config> &&parse_json(const boost::json::value &&json_v);

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

        template<typename AT, typename T = std::remove_cvref_t<typename std::remove_cvref_t<AT>::value_type>>
        AT &getAtomic(const boost::json::object &v, boost::string_view key, AT &d) {
            try {
                if (!v.contains(key)) {
                    return d;
                }
                auto rr = boost::json::try_value_to<std::remove_cvref_t<T>>(v.at(key));
                if (rr.has_value()) {
                    d.store(rr.value());
                }
                return d;
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

namespace OwlCameraConfig {

    using CameraAddrType_1 = OwlConfigLoader::CameraAddrType_1;
    const auto CameraAddrType_1_Placeholder = OwlConfigLoader::CameraAddrType_1_Placeholder;
    using CameraAddrType_2 = OwlConfigLoader::CameraAddrType_2;
    const auto CameraAddrType_2_Placeholder = OwlConfigLoader::CameraAddrType_2_Placeholder;
    using CameraAddrType = OwlConfigLoader::CameraAddrType;
    const auto Camera_VideoCaptureAPI_Placeholder = OwlConfigLoader::Camera_VideoCaptureAPI_Placeholder;
    const auto Camera_VideoCaptureAPI_Default = OwlConfigLoader::Camera_VideoCaptureAPI_Default;

    //      id, CameraAddr, VideoCaptureAPI, w, h
    using CameraInfoTuple =
            std::tuple<int, CameraAddrType, std::string, int, int>;
} // OwlCameraConfig

#endif //OWLACCESSTERMINAL_CONFIGLOADER_H
