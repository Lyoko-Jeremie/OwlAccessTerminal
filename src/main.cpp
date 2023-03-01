// jeremie

#include <iostream>
#include <memory>
#include <vector>
#include <utility>
#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/thread.hpp>
#include <boost/log/trivial.hpp>
#include <boost/program_options.hpp>
#include "CommandService/CommandService.h"
#include "CommandService/CmdServiceHttp.h"
#include "CommandService/SerialController.h"
#include "WebControlService/CmdExecute.h"
#include "WebControlService/EmbedWebServer/EmbedWebServer.h"
#include "ImageService/ImageService.h"
#include "ImageService/ImageServiceHttp.h"
#include "ImageService/CameraReader.h"
#include "ConfigLoader/ConfigLoader.h"
#include "TimeService/TimeService.h"
#include "MapCalc/MapCalcMail.h"
#include "MapCalc/MapCalc.h"

#include "ImageService/protobuf_test.h"

#include "Log/Log.h"

#include <google/protobuf/stubs/common.h>
#include <opencv2/imgcodecs.hpp>


#ifndef DEFAULT_CONFIG
#define DEFAULT_CONFIG R"(config.json)"
#endif // DEFAULT_CONFIG

struct ThreadCallee {
    boost::asio::io_context &ioc;
    boost::thread_group &tg;
    std::string thisThreadName;

    int operator()() {
        try {
            OwlLog::threadName = thisThreadName;
            BOOST_LOG_TRIVIAL(info) << ">>>" << OwlLog::threadName << "<<< running thread <<< <<<";
            // use work to keep ioc run
            auto work_guard_ = boost::asio::make_work_guard(ioc);
            ioc.run();
            boost::ignore_unused(work_guard_);
            BOOST_LOG_TRIVIAL(warning) << "ThreadCallee ioc exit. thread: " << OwlLog::threadName;
        } catch (int e) {
            tg.interrupt_all();
            BOOST_LOG_TRIVIAL(error) << "ThreadCallee catch (int) exception: " << e;
            return -1;
        } catch (boost::exception &e) {
            tg.interrupt_all();
            // https://www.boost.org/doc/libs/1_81_0/libs/exception/doc/diagnostic_information.html
            BOOST_LOG_TRIVIAL(error) << "ThreadCallee catch std::exception :"
                                     << "\n diag: " << boost::diagnostic_information(e)
                                     << "\n what: " << dynamic_cast<std::exception const &>(e).what();
            return -1;
        } catch (const std::exception &e) {
            tg.interrupt_all();
            BOOST_LOG_TRIVIAL(error) << "ThreadCallee catch std::exception: " << e.what();
            return -1;
        } catch (...) {
            tg.interrupt_all();
            // https://www.boost.org/doc/libs/1_81_0/libs/exception/doc/current_exception_diagnostic_information.html
            BOOST_LOG_TRIVIAL(error) << "ThreadCallee catch (...) exception"
                                     << "\n current_exception_diagnostic_information : "
                                     << boost::current_exception_diagnostic_information();
            return -1;
        }
        return 0;
    }
};

//int main() {
//    auto u = boost::urls::parse_uri_reference("/abc?zzxcv=123&er=12");
//    if (u.has_error()) {
//        BOOST_LOG_TRIVIAL(trace) << "(u.has_error()) " << u.error().what();
//    }
//    BOOST_LOG_TRIVIAL(trace) << "ok";
//    auto v = u.value();
//    BOOST_LOG_TRIVIAL(trace) << v.path();
//    BOOST_LOG_TRIVIAL(trace) << v.query();
//    // auto q = u.value().params();
//    // q.find("");
//
//}

//#include <opencv2/aruco.hpp>
//
//int main2(int argc, const char *argv[]) {
//
//    std::cout << "Hello, World!" << std::endl;
//    OwlLog::threadName = "main";
//    OwlLog::init_logging();
//
////    auto image = cv::imread(R"(d:\FTPMain\Upload\Apriltag\tag36h11_1.png)");
////    auto image = cv::imread(R"(d:\FTPMain\Upload\1.jpg)");
//    auto image = cv::imread(R"(000.png)");
//
//    if (image.empty()) {
//        std::cout << "(image.empty())" << std::endl;
//        return 0;
//    }
//
//    BOOST_LOG_TRIVIAL(trace) << "image:"
//                             << " cols " << image.cols
//                             << " rows " << image.rows
//                             << " channels " << image.channels();
////    if (image.channels() > 1) {
////        cv::cvtColor(image, image, cv::ColorConversionCodes::COLOR_BGR2GRAY);
////    }
////    cv::resize(image, image, cv::Size{640, 480}, 0, 0, cv::InterpolationFlags::INTER_CUBIC);
//    BOOST_LOG_TRIVIAL(trace) << "image:"
//                             << " cols " << image.cols
//                             << " rows " << image.rows
//                             << " channels " << image.channels();
//
//    std::vector<int> markerIds;
//    std::vector<std::vector<cv::Point2f>> markerCorners, rejectedCandidates;
//    cv::aruco::DetectorParameters detectorParams = cv::aruco::DetectorParameters();
//    cv::aruco::Dictionary dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_APRILTAG_36h11);
//    cv::aruco::ArucoDetector detector(dictionary, detectorParams);
//    detector.detectMarkers(image, markerCorners, markerIds, rejectedCandidates);
//
//    BOOST_LOG_TRIVIAL(trace) << "markerIds.size():" << markerIds.size();
//    BOOST_LOG_TRIVIAL(trace) << "markerCorners.size():" << markerCorners.size();
//    BOOST_LOG_TRIVIAL(trace) << "rejectedCandidates.size():" << rejectedCandidates.size();
//
//    cv::Mat markerImage;
//    cv::aruco::generateImageMarker(dictionary, 1, 200, markerImage, 1);
//    cv::imwrite("marker1.png", markerImage);
//
//    auto im1 = image.clone();
//    cv::aruco::drawDetectedMarkers(im1, rejectedCandidates);
//    cv::imwrite("123.png", im1);
//    auto im2 = image.clone();
//    cv::aruco::drawDetectedMarkers(im2, markerCorners);
//    cv::imwrite("321.png", im2);
//
//}

int main(int argc, const char *argv[]) {
    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    OwlLog::threadName = "main";

    OwlLog::init_logging();

    BOOST_LOG_OWL(info)
        << "BOOST_LOG_SEV(OwlLog::slg, OwlLog::severity_level_my::aaa)";

    BOOST_LOG_TRIVIAL(info) << "cv::haveImageWriter(.jpg):" << cv::haveImageWriter(".jpg");

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
    config->print();

    boost::asio::io_context ioc_cmd;
    boost::asio::io_context ioc_map_calc;
    auto mailbox_map_calc = std::make_shared<OwlMailDefine::ServiceMapCalcMailbox::element_type>(
            ioc_cmd, ioc_map_calc, "mailbox_map_calc"
    );
    auto mapCalcService = std::make_shared<OwlMapCalc::MapCalc>(
            ioc_map_calc,
            config->shared_from_this(),
            mailbox_map_calc->shared_from_this()
    );
    mapCalcService->init();

    auto mailbox_cmd_udp = std::make_shared<OwlMailDefine::CmdSerialMailbox::element_type>(
            ioc_cmd, ioc_cmd, "mailbox_cmd_udp"
    );
    auto cmdService = std::make_shared<OwlCommandService::CommandService>(
            ioc_cmd,
            mailbox_cmd_udp->shared_from_this(),
            boost::asio::ip::udp::endpoint(
                    boost::asio::ip::udp::v4(),
                    config->config().CommandServiceUdpPort
            )
    );
    cmdService->start();
    auto mailbox_cmd_http = std::make_shared<OwlMailDefine::CmdSerialMailbox::element_type>(
            ioc_cmd, ioc_cmd, "mailbox_cmd_http"
    );
    auto cmdHttpService = std::make_shared<OwlCommandServiceHttp::CmdServiceHttp>(
            ioc_cmd,
            boost::asio::ip::tcp::endpoint(
                    boost::asio::ip::tcp::v4(),
                    config->config().CommandServiceHttpPort
            ),
            mailbox_cmd_http->shared_from_this(),
            mailbox_map_calc->shared_from_this()
    );
    cmdHttpService->start();
    auto serialControllerService = std::make_shared<OwlSerialController::SerialController>(
            ioc_cmd,
            config->shared_from_this(),
            std::vector<OwlMailDefine::CmdSerialMailbox>{
                    mailbox_cmd_udp->shared_from_this(),
                    mailbox_cmd_http->shared_from_this()
            }
    );
    serialControllerService->init();


    boost::asio::io_context ioc_time;
    boost::asio::io_context ioc_imageWeb;
    auto mailbox_imageWeb_time = std::make_shared<OwlMailDefine::ServiceTimeMailbox::element_type>(
            ioc_imageWeb, ioc_time, "mailbox_imageWeb_time"
    );
    auto timeService = std::make_shared<OwlTimeService::TimeService>(
            ioc_time,
            mailbox_imageWeb_time->shared_from_this()
    );
    boost::asio::io_context ioc_cameraReader;
    auto mailbox_image_protobuf = std::make_shared<OwlMailDefine::ServiceCameraMailbox::element_type>(
            ioc_imageWeb, ioc_cameraReader, "mailbox_image_protobuf"
    );
    auto imageServiceProtobuf = std::make_shared<OwlImageService::ImageService>(
            ioc_imageWeb,
            boost::asio::ip::tcp::endpoint(
                    boost::asio::ip::tcp::v4(),
                    config->config().ImageServiceTcpPort
            ),
            mailbox_image_protobuf->shared_from_this()
    );
    imageServiceProtobuf->start();
    auto mailbox_image_http = std::make_shared<OwlMailDefine::ServiceCameraMailbox::element_type>(
            ioc_imageWeb, ioc_cameraReader, "mailbox_image_http"
    );
    auto imageServiceHttp = std::make_shared<OwlImageServiceHttp::ImageServiceHttp>(
            ioc_imageWeb,
            boost::asio::ip::tcp::endpoint(
                    boost::asio::ip::tcp::v4(),
                    config->config().ImageServiceHttpPort
            ),
            config->shared_from_this(),
            mailbox_image_http->shared_from_this(),
            mailbox_imageWeb_time->shared_from_this()
    );
    imageServiceHttp->start();
    auto cameraReader = std::make_shared<OwlCameraReader::CameraReader>(
            ioc_cameraReader,
            std::vector<OwlCameraConfig::CameraInfoTuple>{
                    {1, config->config().camera_addr_1, config->config().camera_1_VideoCaptureAPI,
                            config->config().camera_1_w, config->config().camera_1_h},
                    {2, config->config().camera_addr_2, config->config().camera_2_VideoCaptureAPI,
                            config->config().camera_2_w, config->config().camera_2_h},
            },
            mailbox_image_protobuf->shared_from_this(),
            mailbox_image_http->shared_from_this()
    );
    cameraReader->start();


#ifdef EnableWebStaticModule
    boost::asio::io_context ioc_web_static;
    auto mailbox_web = std::make_shared<OwlMailDefine::WebCmdMailbox::element_type>(
            ioc_web_static, ioc_web_static, "mailbox_web"
    );
    auto webService = std::make_shared<OwlEmbedWebServer::EmbedWebServer>(
            ioc_web_static,
            mailbox_web->shared_from_this(),
            boost::asio::ip::tcp::endpoint(
                    boost::asio::ip::tcp::v4(),
                    config->config().EmbedWebServerHttpPort
            ),
            std::make_shared<std::string>(config->config().embedWebServer.doc_root),
            std::make_shared<std::string>(config->config().embedWebServer.index_file_of_root),
            std::make_shared<std::string>(config->config().embedWebServer.backend_json_string),
            std::make_shared<std::string>(config->config().embedWebServer.allowFileExtList)
    );
    webService->start();
    auto cmdExecuteService = std::make_shared<OwlCmdExecute::CmdExecute>(
            ioc_web_static,
            config->shared_from_this(),
            mailbox_web->shared_from_this()
    );
#endif // EnableWebStaticModule

    boost::asio::io_context ioc_keyboard;
    boost::asio::signal_set sig(ioc_keyboard);
    sig.add(SIGINT);
    sig.add(SIGTERM);
    sig.async_wait([&](const boost::system::error_code error, int signum) {
        if (error) {
            BOOST_LOG_TRIVIAL(error) << "got signal error: " << error.what() << " signum " << signum;
            return;
        }
        BOOST_LOG_TRIVIAL(error) << "got signal: " << signum;
        switch (signum) {
            case SIGINT:
            case SIGTERM: {
                // stop all service on there
                BOOST_LOG_TRIVIAL(info) << "stopping all service. ";
                ioc_map_calc.stop();
                ioc_cmd.stop();
                ioc_time.stop();
                ioc_imageWeb.stop();
                ioc_cameraReader.stop();
#ifdef EnableWebStaticModule
                ioc_web_static.stop();
#endif // EnableWebStaticModule
                ioc_keyboard.stop();
            }
                break;
            default:
                BOOST_LOG_TRIVIAL(warning) << "sig switch default.";
                break;
        }
    });

    size_t processor_count = boost::thread::hardware_concurrency();
    BOOST_LOG_TRIVIAL(info) << "processor_count: " << processor_count;

    boost::thread_group tg;
    tg.create_thread(ThreadCallee{ioc_map_calc, tg, "ioc_map_calc"});
    tg.create_thread(ThreadCallee{ioc_cmd, tg, "ioc_cmd"});
    tg.create_thread(ThreadCallee{ioc_time, tg, "ioc_time"});
    tg.create_thread(ThreadCallee{ioc_imageWeb, tg, "ioc_imageWeb"});
    tg.create_thread(ThreadCallee{ioc_cameraReader, tg, "ioc_cameraReader 1"});
    tg.create_thread(ThreadCallee{ioc_cameraReader, tg, "ioc_cameraReader 2"});
#ifdef EnableWebStaticModule
    tg.create_thread(ThreadCallee{ioc_web_static, tg, "ioc_web_static"});
#endif // EnableWebStaticModule
    tg.create_thread(ThreadCallee{ioc_keyboard, tg, "ioc_keyboard"});


    BOOST_ASSERT(!serialControllerService->weak_from_this().expired());
    BOOST_LOG_TRIVIAL(info) << "boost::thread_group running";
    BOOST_LOG_TRIVIAL(info) << "boost::thread_group size : " << tg.size();
    tg.join_all();
    BOOST_LOG_TRIVIAL(info) << "boost::thread_group end";

    google::protobuf::ShutdownProtobufLibrary();
    BOOST_LOG_TRIVIAL(info) << "boost::thread_group all clear.";
    return 0;
}
