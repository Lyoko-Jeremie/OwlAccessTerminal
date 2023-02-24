// jeremie

#include "MapCalc.h"
#include <boost/filesystem.hpp>
#include <boost/json.hpp>
#include <opencv2/opencv.hpp>
#include "../QuickJsWrapper/QuickJsWrapper.h"

namespace OwlMapCalc {

    MapCalc::MapCalc(
            boost::asio::io_context &ioc,
            OwlMailDefine::ServiceMapCalcMailbox &&mailbox
    ) : ioc_(boost::asio::make_strand(ioc)),
        mailbox_(mailbox) {
        mailbox_->receiveA2B = [this](OwlMailDefine::MailService2MapCalc &&data) {
            receiveMail(std::move(data));
        };
        qjw_ = std::make_shared<OwlQuickJsWrapper::QuickJsWrapper>();
        qjw_->init();
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
                ) -> std::shared_ptr<std::array<double, 3>> {

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
                auto rData = std::make_shared<std::array<double, 3>>();
                auto r = calcF(qjw_->getContext().fromJSON(boost::json::serialize(inputData)));
                bool ok = r[0].as<bool>();
                if (!ok) {
                    // failed
                    return {};
                }
                rData->at(0) = r[1].as<double>();
                rData->at(1) = r[2].as<double>();
                rData->at(2) = r[3].as<double>();
                return rData;
            }
            catch (qjs::exception &) {
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
        tagInfo->aprilTagCenter->centerX = 400;
        tagInfo->aprilTagCenter->centerY = 300;
        tagInfo->aprilTagCenter->cornerLTx = 0;
        tagInfo->aprilTagCenter->cornerLTy = 0;
        tagInfo->aprilTagCenter->cornerRTx = 400;
        tagInfo->aprilTagCenter->cornerRTy = 0;
        tagInfo->aprilTagCenter->cornerRBx = 400;
        tagInfo->aprilTagCenter->cornerRBy = 300;
        tagInfo->aprilTagCenter->cornerLBx = 0;
        tagInfo->aprilTagCenter->cornerLBy = 300;
        tagInfo->aprilTagList = std::make_shared<OwlMailDefine::AprilTagCmd::AprilTagListType::element_type>();
        tagInfo->aprilTagList->push_back(tagInfo->aprilTagCenter.operator*());
        tagInfo->aprilTagList->push_back(tagInfo->aprilTagCenter.operator*());
        tagInfo->aprilTagList->push_back(tagInfo->aprilTagCenter.operator*());
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
                << "MapCalc testMapCalcFunction test ok : ["
                << r->at(0) << "," << r->at(1) << "," << r->at(2) << "]";
            return true;
        }
        BOOST_LOG_TRIVIAL(error) << "MapCalc testMapCalcFunction test failed.";
        return false;
    }

} // OwlMapCalc
