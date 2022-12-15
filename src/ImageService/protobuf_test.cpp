// jeremie

#include <boost/utility/string_view.hpp>
#include "protobuf_test.h"

//      if (ir.has_package_id()) { o.emplace("package_id", ir.package_id()); } else { o.emplace("package_id", nullptr); }
#define ProtoBuff2Json(o, ir, KEY) if (ir.has_##KEY ()) { o.emplace(#KEY, ir.KEY ()); } else { o.emplace(#KEY, nullptr); }

//                   ir.set_cmd_id(getFromJsonObjectDefault(o, "cmd_id", ir.cmd_id()));
#define Json2ProtoBuff(o, ir, KEY) ir.set_##KEY(getFromJsonObjectDefault(o, #KEY, ir.KEY()));

template<typename T>
T getFromJsonObject(const boost::json::object &v, boost::string_view key, bool &good) {
    try {
        T r = boost::json::value_to<T>(v.at(key));
        return r;
    } catch (std::exception &e) {
        good = false;
        return T{};
    }
}

template<typename T>
T getFromJsonObjectDefault(const boost::json::object &v, boost::string_view key, T &&dv) {
    try {
        T r = boost::json::value_to<T>(v.at(key));
        return r;
    } catch (std::exception &e) {
        return dv;
    }
}

namespace OwlImageService::ProtobufTest {


    boost::json::object ImageRequest2Json(const ImageRequest &ir) {
        boost::json::object o = {
                {"cmd_id", ir.cmd_id()}
        };
        ProtoBuff2Json(o, ir, package_id);
        ProtoBuff2Json(o, ir, camera_id);
        ProtoBuff2Json(o, ir, image_width);
        ProtoBuff2Json(o, ir, image_height);

        return o;
    }

    ImageRequest Json2ImageRequest(const boost::json::object &o) {
        // have some bug
        ImageRequest ir;

//        ir.set_cmd_id(getFromJsonObjectDefault(o, "cmd_id", ir.cmd_id()));
        Json2ProtoBuff(o, ir, cmd_id);
        Json2ProtoBuff(o, ir, camera_id);
        Json2ProtoBuff(o, ir, image_width);
        Json2ProtoBuff(o, ir, image_height);

        return ir;
    }

    void createImageRequest() {
        ImageRequest ir;
        ir.set_cmd_id(1);
        ir.set_package_id(1);
        ir.set_camera_id(1);
        std::cout << ir.SerializeAsString() << "\n";
//        std::cout << ir.DebugString() << "\n";
        std::cout << ir.ShortDebugString() << "\n";
        std::cout << ir.ByteSizeLong() << "\n";

        std::cout << boost::json::serialize(ImageRequest2Json(ir)) << "\n";

        auto data = ir.SerializeAsString();

        ImageRequest ir2;
        std::cout << ir2.ParseFromString(data) << "\n";
//        std::cout << ir2.DebugString() << "\n";
        std::cout << ir2.ShortDebugString() << "\n";
        std::cout << ir2.ByteSizeLong() << "\n";
        std::cout << boost::json::serialize(ImageRequest2Json(ir2)) << "\n";

        std::cout << Json2ImageRequest(ImageRequest2Json(ir)).SerializeAsString() << "\n";

//        ImageRequest ir3;
//        ir3;

    }
}