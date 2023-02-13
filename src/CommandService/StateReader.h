// jeremie

#ifndef OWLACCESSTERMINAL_STATEREADER_H
#define OWLACCESSTERMINAL_STATEREADER_H

#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/log/trivial.hpp>
#include <boost/core/ignore_unused.hpp>
#include <utility>
#include "CmdSerialMail.h"
#include "../ConfigLoader/ConfigLoader.h"

namespace OwlSerialController {

    struct AirplaneState {
    };

    struct PortController;

    class StateReader {
    public:
        explicit StateReader(std::weak_ptr<PortController> parentRef) : parentRef_(std::move(parentRef)) {}

        std::weak_ptr<PortController> parentRef_;

        void portDataIn(const std::shared_ptr<PortController> &pt, size_t bytes_transferred);
    };

} // OwlSerialController

#endif //OWLACCESSTERMINAL_STATEREADER_H
