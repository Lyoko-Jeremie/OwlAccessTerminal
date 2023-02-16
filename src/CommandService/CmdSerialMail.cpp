// jeremie

#include "CmdSerialMail.h"

namespace OwlMailDefine {
    const std::map<AdditionCmd, std::string> AdditionCmdNameLookupTable{
            {AdditionCmd::ignore, "ignore"},

            {AdditionCmd::takeoff, "takeoff"},
            {AdditionCmd::land, "land"},
            {AdditionCmd::stop, "stop"},
            {AdditionCmd::keep, "keep"},
            {AdditionCmd::move, "move"},
            {AdditionCmd::rotate, "rotate"},
            {AdditionCmd::high, "high"},
            {AdditionCmd::speed, "speed"},
            {AdditionCmd::led, "led"},

            {AdditionCmd::gotoPosition, "gotoPosition"},
            {AdditionCmd::flyMode, "flyMode"},
            {AdditionCmd::AprilTag, "AprilTag"},

            {AdditionCmd::JoyCon, "JoyCon"},
            {AdditionCmd::JoyConSimple, "JoyConSimple"},
            {AdditionCmd::JoyConGyro, "JoyConGyro"},

            {AdditionCmd::getAirplaneState, "getAirplaneState"},
    };
}
