// jeremie

#ifndef OWLACCESSTERMINAL_MAPCALCPLANEINFOTYPE_H
#define OWLACCESSTERMINAL_MAPCALCPLANEINFOTYPE_H

#include "../MemoryBoost.h"
#include <boost/json.hpp>

namespace OwlMapCalc {

    // same as typescript `map_calc.ts` `interface PlaneInfo{}`
    struct MapCalcPlaneInfoType : public boost::enable_shared_from_this<MapCalcPlaneInfoType> {
        struct Point2 {
            double x;
            double y;
        };
        struct Vec2 {
            double x;
            double y;
        };
        // the deg of plane X axis in image
        double xDirectDeg;
        // the deg of plane Z axis in image
        double zDirectDeg;
        // the deg of plane XZ axis (45deg) in image
        double xzDirectDeg;
        // Point 2 Point pair
        // cm XZ
        Point2 PlaneP;
        // pixel XY
        Point2 ImageP;
        // Plane scale of image on Plane XZ Direct
        // imgPixel[px]/planeDistance[1cm(SizeXY base size)]
        // scalePlaInImg
        Vec2 ScaleXZ;
        // Plane scale in image XY Direct
        // planeDistance[cm(SizeXY base size)]/imgPixel[1px]
        // scaleImgInPla
        Vec2 ScaleXY;


        int64_t timestamp{0};

        void initTimestamp() {
            timestamp = std::chrono::time_point_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now()).time_since_epoch().count();
        }
    };

    extern boost::json::object MapCalcPlaneInfoType2JsonObject(const boost::shared_ptr<MapCalcPlaneInfoType> &info);

}


#endif //OWLACCESSTERMINAL_MAPCALCPLANEINFOTYPE_H
