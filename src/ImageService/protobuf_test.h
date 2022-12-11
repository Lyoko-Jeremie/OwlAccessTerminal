// jeremie

#ifndef OWLACCESSTERMINAL_PROTOBUF_TEST_H
#define OWLACCESSTERMINAL_PROTOBUF_TEST_H

#include <boost/json.hpp>
#include "ImageProtobufDefine/ImageProtocol/ImageProtocol.pb.h"

namespace OwlImageService::ProtobufTest {

    ImageRequest Json2ImageRequest(const boost::json::object &o);

    boost::json::object ImageRequest2Json(const ImageRequest &ir);

    void createImageRequest();
}

#endif //OWLACCESSTERMINAL_PROTOBUF_TEST_H
