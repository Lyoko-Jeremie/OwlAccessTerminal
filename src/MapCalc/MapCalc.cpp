// jeremie

#include "MapCalc.h"
#include <boost/filesystem.hpp>
#include <boost/json.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <opencv2/opencv.hpp>
#include "../QuickJsWrapper/QuickJsWrapper.h"

namespace OwlMapCalc {

//    void installOpenCVExt(qjs::Context &ctx) {
//        auto &module=  ctx.addModule("OpenCVExt");
//        module.function("calcPlaneInfo", [](
//                double p1xPla, double p1yPla,
//                double p2xPla, double p2yPla,
//                double p3xPla, double p3yPla,
//                double p1xImg, double p1yImg,
//                double p2xImg, double p2yImg,
//                double p3xImg, double p3yImg,
//                double imgX, double imgY
//        ) {
//            // TODO
//        });
//
//    }

    MapCalc::MapCalc(
            boost::asio::io_context &ioc,
            OwlMailDefine::ServiceMapCalcMailbox &&mailbox
    ) : ioc_(boost::asio::make_strand(ioc)),
        mailbox_(mailbox) {
        mailbox_->receiveA2B([this](OwlMailDefine::MailService2MapCalc &&data) {
            receiveMail(std::move(data));
        });
        qjw_ = std::make_shared<OwlQuickJsWrapper::QuickJsWrapper>();
        qjw_->init();
//        installOpenCVExt(qjw_->getContext());
    }


    bool MapCalc::loadCalcJsCodeFile(const std::string &filePath) {
        if (!boost::filesystem::exists(filePath)) {
            BOOST_LOG_TRIVIAL(error) << "MapCalc loadCalcJsCodeFile filePath not exists : " << filePath;
            return false;
        }
        bool ok = qjw_->loadCode(filePath);
        if (!ok) {
            BOOST_LOG_TRIVIAL(error) << "MapCalc loadCalcJsCodeFile loadCode failed.";
        }
        return ok;
    }

    using MapCalcFunctionTrueType =
            qjs::Value(qjs::Value);

    bool MapCalc::loadMapCalcFunction(const std::string &functionName) {
        calc_ = nullptr;
        auto f = qjw_->getCallbackFunction<MapCalcFunctionTrueType>(functionName);
        if (!f) {
            return false;
        }
        calc_ = [this, self = shared_from_this(), calcF = std::move(f)]
                (
                        std::shared_ptr<OwlMailDefine::AprilTagCmd> tagInfo,
                        std::shared_ptr<OwlSerialController::AirplaneState> airplaneState
                ) -> MapCalcFunctionType::result_type {

            boost::json::object inputData{};
            if (tagInfo) {
                inputData.emplace("imageX", tagInfo->imageX);
                inputData.emplace("imageY", tagInfo->imageY);
            } else {
                inputData.emplace("imageX", 0);
                inputData.emplace("imageY", 0);
            }
            // tagInfo;
            auto tv = [](const OwlMailDefine::AprilTagCmd::AprilTagCenterType::element_type &t) {
                return boost::json::value{
                        {"id",       t.id},
                        {"dec_marg", t.decision_margin},
                        {"ham",      t.hamming},
                        {"cX",       t.centerX},
                        {"cY",       t.centerY},
                        {"cLTx",     t.cornerLTx},
                        {"cLTy",     t.cornerLTy},
                        {"cRTx",     t.cornerRTx},
                        {"cRTy",     t.cornerRTy},
                        {"cRBx",     t.cornerRBx},
                        {"cRBy",     t.cornerRBy},
                        {"cLBx",     t.cornerLBx},
                        {"cLBy",     t.cornerLBy},
                };
            };
            if (tagInfo && tagInfo->aprilTagCenter) {
                inputData.emplace("tagInfo", boost::json::object{
                        {"center", tv(tagInfo->aprilTagCenter.operator*())},
                });
            } else {
                inputData.emplace("tagInfo", boost::json::object{
                        {"center", {}},
                });
            }
            boost::json::array tagList{};
            if (tagInfo && tagInfo->aprilTagList) {
                for (const auto &a: tagInfo->aprilTagList.operator*()) {
                    tagList.push_back(tv(a));
                }
            }
            inputData["tagInfo"].as_object().emplace("list", tagList);

            // airplaneState;
            if (airplaneState) {
                inputData.emplace("airplaneState", boost::json::value{
                        {"timestamp", airplaneState->timestamp},
                        {"voltage",   airplaneState->voltage},
                        {"high",      airplaneState->high},
                        {"pitch",     airplaneState->pitch},
                        {"roll",      airplaneState->roll},
                        {"yaw",       airplaneState->yaw},
                        {"vx",        airplaneState->vx},
                        {"vy",        airplaneState->vy},
                        {"vz",        airplaneState->vz},
                });
            } else {
                inputData.emplace("airplaneState", boost::json::value{});
            }

            try {
                auto rData = std::make_shared<MapCalcPlaneInfoType>();
                auto r = calcF(qjw_->getContext().fromJSON(boost::json::serialize(inputData)));
                bool ok = r["ok"].as<bool>();
                if (!ok) {
                    // failed
                    return {};
                }
                auto info = r["info"].as<qjs::Value>();
                rData->xDirectDeg = info["xDirectDeg"].as<double>();
                rData->zDirectDeg = info["zDirectDeg"].as<double>();
                rData->xzDirectDeg = info["xzDirectDeg"].as<double>();
//                BOOST_LOG_TRIVIAL(trace) << "read rData->xDirectDeg " << rData->xDirectDeg;
//                BOOST_LOG_TRIVIAL(trace) << "read rData->zDirectDeg " << rData->zDirectDeg;
//                BOOST_LOG_TRIVIAL(trace) << "read rData->xzDirectDeg " << rData->xzDirectDeg;
//                BOOST_LOG_TRIVIAL(trace) << " start read ";
                {
                    auto n = info["PlaneP"].as<qjs::Value>();
                    rData->PlaneP.x = n["x"].as<double>();
                    rData->PlaneP.y = n["y"].as<double>();
//                    BOOST_LOG_TRIVIAL(trace) << "read PlaneP ok";
                }
                {
                    auto n = info["ImageP"].as<qjs::Value>();
                    rData->ImageP.x = n["x"].as<double>();
                    rData->ImageP.y = n["y"].as<double>();
//                    BOOST_LOG_TRIVIAL(trace) << "read ImageP ok";
                }
                {
                    auto n = info["ScaleXZ"].as<qjs::Value>();
                    rData->ScaleXZ.x = n["x"].as<double>();
                    rData->ScaleXZ.y = n["y"].as<double>();
//                    BOOST_LOG_TRIVIAL(trace) << "read ScaleXZ ok";
                }
                {
                    auto n = info["ScaleXY"].as<qjs::Value>();
                    rData->ScaleXY.x = n["x"].as<double>();
                    rData->ScaleXY.y = n["y"].as<double>();
//                    BOOST_LOG_TRIVIAL(trace) << "read ScaleXY ok";
                }
//                BOOST_LOG_TRIVIAL(trace)
//                    << "MapCalc loadMapCalcFunction ok :"
//                    << boost::json::serialize(MapCalcPlaneInfoType2JsonObject(rData));
                return rData;
            } catch (qjs::exception &) {
                auto exc = qjw_->getContext().getException();
                BOOST_LOG_TRIVIAL(error) << "MapCalc loadMapCalcFunction qjs::exception " << (std::string) exc;
                if ((bool) exc["stack"]) {
                    BOOST_LOG_TRIVIAL(error) << "MapCalc loadMapCalcFunction qjs::exception "
                                             << (std::string) exc["stack"];
                }
                // failed
                return {};
            } catch (cv::Exception &e) {
                BOOST_LOG_TRIVIAL(error) << "MapCalc loadMapCalcFunction cv::exception :"
                                         << e.what();
                return {};
            } catch (boost::exception &e) {
                std::string diag = boost::diagnostic_information(e);
                BOOST_LOG_TRIVIAL(error) << "MapCalc loadMapCalcFunction std::exception :"
                                         << "\n diag: " << diag
                                         << "\n what: " << dynamic_cast<std::exception const &>(e).what();
                return {};
            } catch (std::exception &e) {
                BOOST_LOG_TRIVIAL(error) << "MapCalc loadMapCalcFunction std::exception :"
                                         << e.what();
                return {};
            } catch (...) {
                BOOST_LOG_TRIVIAL(error) << "MapCalc loadMapCalcFunction catch (...) ";
                return {};
            }

        };
        return true;
    }

    bool MapCalc::testMapCalcFunction() {
        auto tagInfo = std::make_shared<OwlMailDefine::AprilTagCmd>();
        tagInfo->imageX = 800;
        tagInfo->imageY = 600;
        tagInfo->aprilTagCenter = std::make_shared<OwlMailDefine::AprilTagCmd::AprilTagCenterType::element_type>();
        tagInfo->aprilTagCenter->id = 0;
        // (0,0) at image left top
        double x = 400;
        double y = 300;
        tagInfo->aprilTagCenter->centerX = x;
        tagInfo->aprilTagCenter->centerY = y;

        tagInfo->aprilTagCenter->cornerLTx = x - 500;
        tagInfo->aprilTagCenter->cornerLTy = y - 500;

        tagInfo->aprilTagCenter->cornerRTx = x + 500;
        tagInfo->aprilTagCenter->cornerRTy = y - 500;

        tagInfo->aprilTagCenter->cornerRBx = x + 500;
        tagInfo->aprilTagCenter->cornerRBy = y + 500;

        tagInfo->aprilTagCenter->cornerLBx = x - 500;
        tagInfo->aprilTagCenter->cornerLBy = y + 500;

//        tagInfo->aprilTagCenter->cornerLTx = x;
//        tagInfo->aprilTagCenter->cornerLTy = y - 10;
//
//        tagInfo->aprilTagCenter->cornerRTx = x + 10;
//        tagInfo->aprilTagCenter->cornerRTy = y;
//
//        tagInfo->aprilTagCenter->cornerRBx = x;
//        tagInfo->aprilTagCenter->cornerRBy = y + 10;
//
//        tagInfo->aprilTagCenter->cornerLBx = x - 10;
//        tagInfo->aprilTagCenter->cornerLBy = y;
        tagInfo->aprilTagList = std::make_shared<OwlMailDefine::AprilTagCmd::AprilTagListType::element_type>();
        tagInfo->aprilTagList->push_back(tagInfo->aprilTagCenter.operator*());
//        tagInfo->aprilTagList->push_back(tagInfo->aprilTagCenter.operator*());
//        tagInfo->aprilTagList->push_back(tagInfo->aprilTagCenter.operator*());
        auto airplaneState = std::make_shared<OwlSerialController::AirplaneState>();
        airplaneState->initTimestamp();
        airplaneState->high = 50;

        auto t1 = std::chrono::high_resolution_clock::now();
        auto r = calcMapPosition(tagInfo, airplaneState);
        auto t2 = std::chrono::high_resolution_clock::now();
        BOOST_LOG_TRIVIAL(trace)
            << "MapCalc testMapCalcFunction test time: "
            << "\n" << std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count() << " [nanoseconds]"
            << "\n" << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count() << " [microseconds]"
            << "\n" << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << " [milliseconds]";
        if (r) {
            BOOST_LOG_TRIVIAL(info)
                << "MapCalc testMapCalcFunction test ok :"
                << boost::json::serialize(MapCalcPlaneInfoType2JsonObject(r));
            return true;
        }
        BOOST_LOG_TRIVIAL(error) << "MapCalc testMapCalcFunction test failed.";
        return false;
    }

} // OwlMapCalc
