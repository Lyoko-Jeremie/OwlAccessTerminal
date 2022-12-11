#include <iostream>
#include <memory>
#include <boost/asio.hpp>
#include "./CommandService/CommandService.h"

int main() {
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
    ioc.run();
    return 0;
}
