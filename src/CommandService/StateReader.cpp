// jeremie

#include "StateReader.h"
#include "./SerialController.h"
#include <boost/asio/read_until.hpp>

namespace OwlSerialController {


    void StateReader::portDataIn(const std::shared_ptr<PortController> &pt, size_t bytes_transferred) {
        // TODO
        pt->readBuffer;
        pt->readBuffer.consume(bytes_transferred);
    }


} // OwlSerialController