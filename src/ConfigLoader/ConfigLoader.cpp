// jeremie

#include "ConfigLoader.h"

namespace OwlConfigLoader {
    boost::json::value ConfigLoader::load_json_file(const std::string &filePath) {
        boost::system::error_code ec;
        std::stringstream ss;
        boost::filesystem::ifstream f(filePath);
        if (!f)
            return nullptr;
        ss << f.rdbuf();
        f.close();
        boost::json::stream_parser p;
        p.write(ss.str(), ec);
        if (ec)
            return nullptr;
        p.finish(ec);
        if (ec)
            return nullptr;
        return p.release();
    }

    Config ConfigLoader::parse_json(const boost::json::value &&json_v) {
        const auto &root = json_v.as_object();

        Config config_;

        config_.CommandServiceUdpPort = get(root, "CommandServiceUdpPort", config_.CommandServiceUdpPort);
        config_.ImageServiceTcpPort = get(root, "ImageServiceTcpPort", config_.ImageServiceTcpPort);
        config_.EmbedWebServerHttpPort = get(root, "EmbedWebServerHttpPort", config_.EmbedWebServerHttpPort);
        config_.ImageServiceHttpPort = get(root, "ImageServiceHttpPort", config_.ImageServiceHttpPort);
        config_.airplane_fly_serial_baud_rate = get(root, "airplane_fly_serial_baud_rate",
                                                    config_.airplane_fly_serial_baud_rate);
        config_.airplane_fly_serial_addr = get(root, "airplane_fly_serial_addr", config_.airplane_fly_serial_addr);
        config_.camera_addr_1 = get(root, "camera_addr_1", config_.camera_addr_1);
        config_.camera_addr_2 = get(root, "camera_addr_2", config_.camera_addr_2);

        if (root.contains("embedWebServer")) {
            auto embedWebServer = getObj(root, "embedWebServer");
            config_.embedWebServer.doc_root = get(root, "doc_root", config_.embedWebServer.doc_root);
            config_.embedWebServer.index_file_of_root = get(root, "index_file_of_root",
                                                            config_.embedWebServer.index_file_of_root);
            config_.embedWebServer.backend_json_string = get(root, "backend_json_string",
                                                             config_.embedWebServer.backend_json_string);
            config_.embedWebServer.allowFileExtList = get(root, "allowFileExtList",
                                                          config_.embedWebServer.allowFileExtList);
        }

        return {};
    }
} // OwlConfigLoader