// jeremie

#include "ConfigLoader.h"
#include <sstream>

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

        return config_;
    }
} // OwlConfigLoader