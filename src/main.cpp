#include <iostream>
#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/thread.hpp>
#include <boost/log/trivial.hpp>
#include "CommandService/CommandService.h"
#include "WebControlService/EmbedWebServer/EmbedWebServer.h"
#include "ImageService/ImageService.h"

#include "ImageService/protobuf_test.h"

#include <google/protobuf/stubs/common.h>


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


int main() {
    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

//    // test
//    OwlImageService::ProtobufTest::createImageRequest();
//    // Optional:  Delete all global objects allocated by libprotobuf.
//    google::protobuf::ShutdownProtobufLibrary();
//    std::cout << sizeof(int) << std::endl;
//    return 0;

    boost::asio::io_context ioc_cmd;
    std::cout << "Hello, World!" << std::endl;
    auto cmdService = std::make_shared<OwlCommandService::CommandService>(
            ioc_cmd,
            boost::asio::ip::udp::endpoint(
                    boost::asio::ip::udp::v4(),
                    23333
            )
    );
    cmdService->start();


    boost::asio::io_context ioc_image;
    auto imageService = std::make_shared<OwlImageService::ImageService>(
            ioc_image,
            boost::asio::ip::tcp::endpoint(
                    boost::asio::ip::tcp::v4(),
                    23332
            )
    );
    imageService->start();


    boost::asio::io_context ioc_web_static;
    auto webService = std::make_shared<OwlEmbedWebServer::EmbedWebServer>(
            ioc_web_static,
            boost::asio::ip::tcp::endpoint(
                    boost::asio::ip::tcp::v4(),
                    81
            ),
            std::make_shared<std::string>("./html"),
            std::make_shared<std::string>("index.html"),
            std::make_shared<std::string>("{}"),
            std::make_shared<std::string>("htm html js json jpg jpeg png bmp gif ico svg css")
    );
    webService->start();


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
