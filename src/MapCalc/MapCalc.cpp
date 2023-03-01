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
            std::shared_ptr<OwlConfigLoader::ConfigLoader> config,
            OwlMailDefine::ServiceMapCalcMailbox &&mailbox
    ) : ioc_(boost::asio::make_strand(ioc)),
        config_(config),
        mailbox_(mailbox) {
        mailbox_->receiveA2B([this](OwlMailDefine::MailService2MapCalc &&data) {
            receiveMail(std::move(data));
        });
    }

    void MapCalc::init() {
        BOOST_LOG_OWL(trace_map) << "MapCalc::init()";
        boost::asio::post(ioc_, [
                this, self = shared_from_this()
        ]() {
            BOOST_LOG_OWL(trace_map) << "MapCalc::init() dispatch";
            qjw_ = std::make_shared<OwlQuickJsWrapper::QuickJsWrapper>();
            BOOST_LOG_OWL(trace_map) << "MapCalc::init() qjw_ create ok";
            qjw_->init();
            BOOST_LOG_OWL(trace_map) << "MapCalc::init() qjw_->init() ok";
            BOOST_LOG_OWL(trace_map)
                << "MapCalc::init() loadCalcJsCodeFile "
                <<
                loadCalcJsCodeFile(config_->config().js_map_calc_file);
            BOOST_LOG_OWL(trace_map)
                << "MapCalc::init() loadMapCalcFunction "
                <<
                loadMapCalcFunction(config_->config().js_map_calc_function_name);
            BOOST_LOG_OWL(trace_map)
                << "MapCalc::init() testMapCalcFunction "
                <<
                testMapCalcFunction();
            BOOST_LOG_OWL(trace_map) << "MapCalc::init() ok";
        });
    }


    bool MapCalc::loadCalcJsCodeFile(const std::string &filePath) {
        if (!boost::filesystem::exists(filePath)) {
            BOOST_LOG_OWL(error) << "MapCalc loadCalcJsCodeFile filePath not exists : " << filePath;
            return false;
        }
        BOOST_LOG_OWL(trace_map) << "MapCalc loadCalcJsCodeFile qjw_->loadCode(filePath) begin";
        bool ok = qjw_->loadCode(filePath);
        BOOST_LOG_OWL(trace_map) << "MapCalc loadCalcJsCodeFile qjw_->loadCode(filePath) end";
        if (!ok) {
            BOOST_LOG_OWL(error) << "MapCalc loadCalcJsCodeFile loadCode failed.";
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
            BOOST_LOG_OWL(trace_map) << "MapCalc calc_ start";

            boost::json::object inputData{};
            try {
                if (tagInfo) {
                    inputData.emplace("imageX", tagInfo->imageX);
                    inputData.emplace("imageY", tagInfo->imageY);
                } else {
                    inputData.emplace("imageX", 0);
                    inputData.emplace("imageY", 0);
                }
                BOOST_LOG_OWL(trace_map) << "MapCalc calc_ (tagInfo) ok";
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
                boost::json::object tagInfoJson{};
                if (tagInfo && tagInfo->aprilTagCenter) {
                    tagInfoJson.emplace("center", tv(tagInfo->aprilTagCenter.operator*()));
//                    inputData.emplace("tagInfo", boost::json::object{
//                            {"center", tv(tagInfo->aprilTagCenter.operator*())},
//                    });
                } else {
                    tagInfoJson.emplace("center", boost::json::object{});
//                    inputData.emplace("tagInfo", boost::json::object{
//                            {"center", {}},
//                    });
                }
                BOOST_LOG_OWL(trace_map) << "MapCalc calc_ (tagInfo && tagInfo->aprilTagCenter) ok";
                boost::json::array tagList{};
                if (tagInfo && tagInfo->aprilTagList) {
                    for (const auto &a: tagInfo->aprilTagList.operator*()) {
                        tagList.push_back(tv(a));
                    }
                }
                tagInfoJson.emplace("list", tagList);
//                inputData["tagInfo"].as_object().emplace("list", tagList);
                inputData.emplace("tagInfo", tagInfoJson);
                BOOST_LOG_OWL(trace_map) << "MapCalc calc_ (tagInfo && tagInfo->aprilTagList) ok";

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
                BOOST_LOG_OWL(trace_map) << "MapCalc calc_ inputData (airplaneState) ok";
            } catch (boost::exception &e) {
                std::string diag = boost::diagnostic_information(e);
                BOOST_LOG_OWL(error) << "MapCalc calc_ inputData std::exception :"
                                     << "\n diag: " << diag
                                     << "\n what: " << dynamic_cast<std::exception const &>(e).what();
                throw;
                return {};
            } catch (std::exception &e) {
                BOOST_LOG_OWL(error) << "MapCalc calc_ inputData std::exception :"
                                     << e.what();
                throw;
                return {};
            } catch (...) {
                BOOST_LOG_OWL(error) << "MapCalc calc_ inputData catch (...) "
                                     << "\n" << boost::current_exception_diagnostic_information();
                throw;
                return {};
            }
            try {
                auto rData = std::make_shared<MapCalcPlaneInfoType>();
                BOOST_LOG_OWL(trace_map) << "MapCalc calc_ before calcF trigger_qjs_update_when_thread_change";
                qjw_->trigger_qjs_update_when_thread_change();
                BOOST_LOG_OWL(trace_map) << "MapCalc calc_ before calcF";
                auto r = calcF(qjw_->getContext().fromJSON(boost::json::serialize(inputData)));
                BOOST_LOG_OWL(trace_map) << "MapCalc calc_ after calcF";
                BOOST_LOG_OWL(trace_map) << "MapCalc calc_ calcF " << r.toJSON();
                bool ok = r["ok"].as<bool>();
                if (!ok) {
                    BOOST_LOG_OWL(warning) << "MapCalc calc_ (!ok)";
                    // failed
                    return {};
                }
                auto info = r["info"].as<qjs::Value>();
                BOOST_LOG_OWL(trace_map) << "MapCalc calc_ rData DirectDeg";
                rData->xDirectDeg = info["xDirectDeg"].as<double>();
                rData->zDirectDeg = info["zDirectDeg"].as<double>();
                rData->xzDirectDeg = info["xzDirectDeg"].as<double>();
                BOOST_LOG_OWL(trace_map) << "read rData->xDirectDeg " << rData->xDirectDeg;
                BOOST_LOG_OWL(trace_map) << "read rData->zDirectDeg " << rData->zDirectDeg;
                BOOST_LOG_OWL(trace_map) << "read rData->xzDirectDeg " << rData->xzDirectDeg;
                BOOST_LOG_OWL(trace_map) << " start read ";
                {
                    auto n = info["PlaneP"].as<qjs::Value>();
                    rData->PlaneP.x = n["x"].as<double>();
                    rData->PlaneP.y = n["y"].as<double>();
                    BOOST_LOG_OWL(trace_map) << "read PlaneP ok";
                }
                {
                    auto n = info["ImageP"].as<qjs::Value>();
                    rData->ImageP.x = n["x"].as<double>();
                    rData->ImageP.y = n["y"].as<double>();
                    BOOST_LOG_OWL(trace_map) << "read ImageP ok";
                }
                {
                    auto n = info["ScaleXZ"].as<qjs::Value>();
                    rData->ScaleXZ.x = n["x"].as<double>();
                    rData->ScaleXZ.y = n["y"].as<double>();
                    BOOST_LOG_OWL(trace_map) << "read ScaleXZ ok";
                }
                {
                    auto n = info["ScaleXY"].as<qjs::Value>();
                    rData->ScaleXY.x = n["x"].as<double>();
                    rData->ScaleXY.y = n["y"].as<double>();
                    BOOST_LOG_OWL(trace_map) << "read ScaleXY ok";
                }
                BOOST_LOG_OWL(trace_map)
                    << "MapCalc calc_ rData ok :"
                    << boost::json::serialize(MapCalcPlaneInfoType2JsonObject(rData));
                return rData;
            } catch (qjs::exception &e) {
                try {
                    auto exc = qjw_->getContext().getException();
                    BOOST_LOG_OWL(error) << "MapCalc calc_ calcF qjs::exception " << (std::string) exc;
                    if ((bool) exc["stack"]) {
                        BOOST_LOG_OWL(error) << "MapCalc loadMapCalcFunction qjs::exception "
                                             << (std::string) exc["stack"];
                    }
                } catch (...) {
                    BOOST_LOG_OWL(error) << "MapCalc calc_ qjs::exception&e catch (...) exception"
                                         << "\n current_exception_diagnostic_information : "
                                         << boost::current_exception_diagnostic_information();
                }
                // failed
                return {};
            } catch (cv::Exception &e) {
                BOOST_LOG_OWL(error) << "MapCalc calc_ calcF cv::exception :"
                                     << e.what();
                return {};
            } catch (boost::exception &e) {
                std::string diag = boost::diagnostic_information(e);
                BOOST_LOG_OWL(error) << "MapCalc calc_ calcF std::exception :"
                                     << "\n diag: " << diag
                                     << "\n what: " << dynamic_cast<std::exception const &>(e).what();
                return {};
            } catch (std::exception &e) {
                BOOST_LOG_OWL(error) << "MapCalc calc_ calcF std::exception :"
                                     << e.what();
                return {};
            } catch (...) {
                BOOST_LOG_OWL(error) << "MapCalc calc_ calcF catch (...) "
                                     << "\n" << boost::current_exception_diagnostic_information();
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

//        tagInfo->aprilTagCenter->cornerLTx = x - 500;
//        tagInfo->aprilTagCenter->cornerLTy = y - 500;
//
//        tagInfo->aprilTagCenter->cornerRTx = x + 500;
//        tagInfo->aprilTagCenter->cornerRTy = y - 500;
//
//        tagInfo->aprilTagCenter->cornerRBx = x + 500;
//        tagInfo->aprilTagCenter->cornerRBy = y + 500;
//
//        tagInfo->aprilTagCenter->cornerLBx = x - 500;
//        tagInfo->aprilTagCenter->cornerLBy = y + 500;

        tagInfo->aprilTagCenter->cornerLTx = x;
        tagInfo->aprilTagCenter->cornerLTy = y - 500;

        tagInfo->aprilTagCenter->cornerRTx = x + 500;
        tagInfo->aprilTagCenter->cornerRTy = y;

        tagInfo->aprilTagCenter->cornerRBx = x;
        tagInfo->aprilTagCenter->cornerRBy = y + 500;

        tagInfo->aprilTagCenter->cornerLBx = x - 500;
        tagInfo->aprilTagCenter->cornerLBy = y;
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
        BOOST_LOG_OWL(trace_map)
            << "MapCalc testMapCalcFunction test time: "
            << "\n" << std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count() << " [nanoseconds]"
            << "\n" << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count() << " [microseconds]"
            << "\n" << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << " [milliseconds]";
        if (r) {
            BOOST_LOG_OWL(info)
                << "MapCalc testMapCalcFunction test ok :"
                << boost::json::serialize(MapCalcPlaneInfoType2JsonObject(r));
            return true;
        }
        BOOST_LOG_OWL(error) << "MapCalc testMapCalcFunction test failed.";
        return false;
    }

} // OwlMapCalc
