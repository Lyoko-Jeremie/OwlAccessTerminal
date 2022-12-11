#include <iostream>
#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>
#include "CommandService/CommandService.h"

#include "ImageService/protobuf_test.h"

#include <google/protobuf/stubs/common.h>

int main() {
    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    OwlImageService::ProtobufTest::createImageRequest();

    // Optional:  Delete all global objects allocated by libprotobuf.
    google::protobuf::ShutdownProtobufLibrary();

    return 0;

    boost::asio::io_context ioc;
    boost::asio::executor ex = boost::asio::make_strand(ioc);
    std::cout << "Hello, World!" << std::endl;
    auto p = std::make_shared<OwlCommandService::CommandService>(
            ioc,
            boost::asio::ip::udp::endpoint(
                    boost::asio::ip::udp::v4(),
                    23333
            )
    );
    p->start();


    boost::asio::signal_set sig(ioc);
    sig.add(SIGINT);
    sig.add(SIGTERM);
    sig.async_wait([&](const boost::system::error_code error, int signum) {
        if (error) {
            return;
        }
        std::cerr << "got signal: " << signum << std::endl;
        switch (signum) {
            case SIGINT:
            case SIGTERM: {
                // TODO stop all service on there
                ioc.stop();
            }
                break;
        }
    });

    ioc.run();
    return 0;
}
