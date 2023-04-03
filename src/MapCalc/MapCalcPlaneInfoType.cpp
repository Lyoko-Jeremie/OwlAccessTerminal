// jeremie

#include "MapCalcPlaneInfoType.h"

namespace OwlMapCalc {

    boost::json::object MapCalcPlaneInfoType2JsonObject(const boost::shared_ptr<MapCalcPlaneInfoType> &info) {
        if (!info) {
            return {};
        }
        boost::json::object o{
                {"xDirectDeg",  info->xDirectDeg},
                {"zDirectDeg",  info->zDirectDeg},
                {"xzDirectDeg", info->xzDirectDeg},
                {
                 "PlaneP",
                                boost::json::object{
                                        {"x", info->PlaneP.x},
                                        {"y", info->PlaneP.y},
                                }
                },
                {
                 "ImageP",
                                boost::json::object{
                                        {"x", info->ImageP.x},
                                        {"y", info->ImageP.y},
                                }
                },
                {
                 "ScaleXZ",
                                boost::json::object{
                                        {"x", info->ScaleXZ.x},
                                        {"y", info->ScaleXZ.y},
                                }
                },
                {
                 "ScaleXY",
                                boost::json::object{
                                        {"x", info->ScaleXY.x},
                                        {"y", info->ScaleXY.y},
                                }
                },
                {
                 "timestamp",       info->timestamp
                },
        };
        return o;
    }
}
