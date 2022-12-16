#include <iostream>
#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/thread.hpp>
#include <boost/log/trivial.hpp>
#include <boost/program_options.hpp>
#include "CommandService/CommandService.h"
#include "CommandService/SerialController.h"
#include "WebControlService/CmdExecute.h"
#include "WebControlService/EmbedWebServer/EmbedWebServer.h"
#include "ImageService/ImageService.h"
#include "ConfigLoader/ConfigLoader.h"
#include "AsyncCallbackMailbox/AsyncCallbackMailbox.h"

#include "ImageService/protobuf_test.h"

#include <google/protobuf/stubs/common.h>
#include <opencv2/imgcodecs.hpp>


#ifndef DEFAULT_CONFIG
#define DEFAULT_CONFIG R"(config.json)"
#endif // DEFAULT_CONFIG

struct ThreadCallee {
    boost::asio::io_context &ioc;
    boost::thread_group &tg;

    int operator()() {
        try {
            ioc.run();
        } catch (int e) {
            tg.interrupt_all();
            BOOST_LOG_TRIVIAL(error) << "catch (int) exception: " << e;
            return -1;
        } catch (const std::exception &e) {
            tg.interrupt_all();
            BOOST_LOG_TRIVIAL(error) << "catch std::exception: " << e.what();
            return -1;
        } catch (...) {
            tg.interrupt_all();
            BOOST_LOG_TRIVIAL(error) << "catch (...) exception";
            return -1;
        }
        return 0;
    }
};


int main(int argc, const char *argv[]) {
    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    BOOST_LOG_TRIVIAL(info) << "cv::haveImageWriter(jpg):" << cv::haveImageWriter("jpg");

//    // test
//    OwlImageService::ProtobufTest::createImageRequest();
//    // Optional:  Delete all global objects allocated by libprotobuf.
//    google::protobuf::ShutdownProtobufLibrary();
//    std::cout << sizeof(int) << std::endl;
//    return 0;

    // parse start params
    std::string config_file;
    boost::program_options::options_description desc("options");
    desc.add_options()
            ("config,c", boost::program_options::value<std::string>(&config_file)->
                    default_value(DEFAULT_CONFIG)->
                    value_name("CONFIG"), "specify config file")
            ("help,h", "print help message")
            ("version,v", "print version and build info");
    boost::program_options::positional_options_description pd;
    pd.add("config", 1);
    boost::program_options::variables_map vMap;
    boost::program_options::store(
            boost::program_options::command_line_parser(argc, argv)
                    .options(desc)
                    .positional(pd)
                    .run(), vMap);
    boost::program_options::notify(vMap);
    if (vMap.count("help")) {
        std::cout << "usage: " << argv[0] << " [[-c] CONFIG]" << "\n" << std::endl;

        std::cout << "    OwlAccessTerminal  Copyright (C) 2023 \n"
                  // << "    This program comes with ABSOLUTELY NO WARRANTY; \n"
                  // << "    This is free software, and you are welcome to redistribute it\n"
                  // << "    under certain conditions; \n"
                  // << "         GNU GENERAL PUBLIC LICENSE , Version 3 "
                  << "\n" << std::endl;

        std::cout << desc << std::endl;
        return 0;
    }
    if (vMap.count("version")) {
        std::cout << "Boost " << BOOST_LIB_VERSION <<
                  ", ProtoBuf " << GOOGLE_PROTOBUF_VERSION <<
                  ", OpenCV " << CV_VERSION
                  << std::endl;
        return 0;
    }

    BOOST_LOG_TRIVIAL(info) << "config_file: " << config_file;



    // load config
    auto config = std::make_shared<OwlConfigLoader::ConfigLoader>();
    config->init(config_file);

    boost::asio::io_context ioc_cmd;
    auto mailbox_cmd = std::make_shared<OwlMailDefine::CmdSerialMailbox::element_type>(
            ioc_cmd, ioc_cmd
    );
    auto cmdService = std::make_shared<OwlCommandService::CommandService>(
            ioc_cmd,
            mailbox_cmd->shared_from_this(),
            boost::asio::ip::udp::endpoint(
                    boost::asio::ip::udp::v4(),
                    config->config.CommandServiceUdpPort
            )
    );
    cmdService->start();
    auto serialControllerService = std::make_shared<OwlSerialController::SerialController>(
            ioc_cmd,
            mailbox_cmd->shared_from_this()
    );
    bool serialControllerServiceStartOk = serialControllerService->start(
            config->config.airplane_fly_serial_addr,
            config->config.airplane_fly_serial_baud_rate
    );
    BOOST_LOG_TRIVIAL(info)
        << "serialControllerService start: "
        << serialControllerServiceStartOk;


    boost::asio::io_context ioc_image;
    auto imageService = std::make_shared<OwlImageService::ImageService>(
            ioc_image,
            boost::asio::ip::tcp::endpoint(
                    boost::asio::ip::tcp::v4(),
                    config->config.ImageServiceTcpPort
            )
    );
    imageService->start();


    boost::asio::io_context ioc_web_static;
    auto mailbox_web = std::make_shared<OwlMailDefine::WebCmdMailbox::element_type>(
            ioc_web_static, ioc_web_static
    );
    auto webService = std::make_shared<OwlEmbedWebServer::EmbedWebServer>(
            ioc_web_static,
            mailbox_web->shared_from_this(),
            boost::asio::ip::tcp::endpoint(
                    boost::asio::ip::tcp::v4(),
                    config->config.EmbedWebServerHttpPort
            ),
            std::make_shared<std::string>(config->config.embedWebServer.doc_root),
            std::make_shared<std::string>(config->config.embedWebServer.index_file_of_root),
            std::make_shared<std::string>(config->config.embedWebServer.backend_json_string),
            std::make_shared<std::string>(config->config.embedWebServer.allowFileExtList)
    );
    webService->start();
    auto cmdExecuteService = std::make_shared<OwlCmdExecute::CmdExecute>(
            ioc_web_static,
            mailbox_web->shared_from_this()
    );


    boost::asio::io_context ioc_keyboard;
    boost::asio::signal_set sig(ioc_keyboard);
    sig.add(SIGINT);
    sig.add(SIGTERM);
    sig.async_wait([&](const boost::system::error_code error, int signum) {
        if (error) {
            return;
        }
        BOOST_LOG_TRIVIAL(error) << "got signal: " << signum;
        switch (signum) {
            case SIGINT:
            case SIGTERM: {
                // stop all service on there
                BOOST_LOG_TRIVIAL(info) << "stopping all service. ";
                ioc_cmd.stop();
                ioc_image.stop();
                ioc_web_static.stop();
                ioc_keyboard.stop();
            }
                break;
        }
    });

    size_t processor_count = boost::thread::hardware_concurrency();
    BOOST_LOG_TRIVIAL(info) << "processor_count: " << processor_count;

    boost::thread_group tg;
    tg.create_thread(ThreadCallee{ioc_cmd, tg});
    tg.create_thread(ThreadCallee{ioc_image, tg});
    tg.create_thread(ThreadCallee{ioc_web_static, tg});
    tg.create_thread(ThreadCallee{ioc_keyboard, tg});

    BOOST_LOG_TRIVIAL(info) << "boost::thread_group running";
    BOOST_LOG_TRIVIAL(info) << "boost::thread_group size : " << tg.size();
    tg.join_all();
    BOOST_LOG_TRIVIAL(info) << "boost::thread_group end";

    google::protobuf::ShutdownProtobufLibrary();
    BOOST_LOG_TRIVIAL(info) << "boost::thread_group all clear.";
    return 0;
}
