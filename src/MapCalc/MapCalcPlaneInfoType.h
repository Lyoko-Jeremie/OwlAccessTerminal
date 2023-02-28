// jeremie

#ifndef OWLACCESSTERMINAL_MAPCALCPLANEINFOTYPE_H
#define OWLACCESSTERMINAL_MAPCALCPLANEINFOTYPE_H

#include <memory>
#include <boost/json.hpp>

namespace OwlMapCalc {

    // same as typescript `map_calc.ts` `interface PlaneInfo{}`
    struct MapCalcPlaneInfoType : public std::enable_shared_from_this<MapCalcPlaneInfoType> {
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
    };

    extern boost::json::object MapCalcPlaneInfoType2JsonObject(const std::shared_ptr<MapCalcPlaneInfoType> &info);

}


#endif //OWLACCESSTERMINAL_MAPCALCPLANEINFOTYPE_H
