// jeremie

#include "MapCalc.h"
#include <boost/filesystem.hpp>
#include <boost/json.hpp>
#include "../QuickJsWrapper/QuickJsWrapper.h"

namespace OwlMapCalc {

    MapCalc::MapCalc(
            boost::asio::io_context &ioc,
            OwlMailDefine::ServiceMapCalcMailbox &&mailbox
    ) : ioc_(ioc),
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
            // TODO
            boost::json::value inputData{};
            tagInfo;
            airplaneState;

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
                BOOST_LOG_TRIVIAL(error) << "loadMapCalcFunction qjs::exception " << (std::string) exc;
                if ((bool) exc["stack"]) {
                    BOOST_LOG_TRIVIAL(error) << (std::string) exc["stack"];
                }
                // failed
                return {};
            }

        };
        return true;
    }

} // OwlMapCalc