// jeremie

#ifndef OWLACCESSTERMINAL_MAPCALC_H
#define OWLACCESSTERMINAL_MAPCALC_H

#include <memory>
#include <functional>
#include <array>
#include <boost/asio.hpp>
#include "../Log/Log.h"
#include <utility>
#include "MapCalcMail.h"
#include "MapCalcPlaneInfoType.h"
#include "../ConfigLoader/ConfigLoader.h"
#include "../CommandService/CmdSerialMail.h"
#include "../CommandService/AirplaneState.h"

namespace OwlQuickJsWrapper {
    class QuickJsWrapper;
}

namespace OwlMapCalc {

    class MapCalc : public std::enable_shared_from_this<MapCalc> {
    public:
        MapCalc(
                boost::asio::io_context &ioc,
                std::shared_ptr<OwlConfigLoader::ConfigLoader> config,
                OwlMailDefine::ServiceMapCalcMailbox &&mailbox
        );

        ~MapCalc() {
            BOOST_LOG_OWL(trace) << "~MapCalc()";
            mailbox_->receiveB2A(nullptr);
        }

        using MapCalcFunctionType = std::function<
                std::shared_ptr<MapCalcPlaneInfoType>(
                        std::shared_ptr<OwlMailDefine::AprilTagCmd> tagInfo,
                        std::shared_ptr<OwlSerialController::AirplaneState> airplaneState
                )>;

    private:
        boost::asio::strand<boost::asio::io_context::executor_type> ioc_;
        std::shared_ptr<OwlConfigLoader::ConfigLoader> config_;
        OwlMailDefine::ServiceMapCalcMailbox mailbox_;

        std::shared_ptr<OwlQuickJsWrapper::QuickJsWrapper> qjw_;

        MapCalcFunctionType calc_;

    public:

        void init();

    private:
        bool loadCalcJsCodeFile(const std::string &filePath);

        bool loadMapCalcFunction(const std::string &functionName);

        bool testMapCalcFunction();

        MapCalcFunctionType::result_type calcMapPosition(
                std::shared_ptr<OwlMailDefine::AprilTagCmd> tagInfo,
                std::shared_ptr<OwlSerialController::AirplaneState> airplaneState
        ) {
            BOOST_LOG_OWL(trace) << "MapCalc::calcMapPosition";
            if (calc_) {
                auto r = calc_(std::move(tagInfo), std::move(airplaneState));
                BOOST_LOG_OWL(trace) << "MapCalc::calcMapPosition return r";
                return r;
            }
            BOOST_LOG_OWL(trace) << "MapCalc::calcMapPosition return {}";
            return {};
        }

    private:

        void receiveMail(OwlMailDefine::MailService2MapCalc &&data) {
#ifdef DEBUG_IF_CHECK_POINT
            constexpr bool flag_DEBUG_IF_CHECK_POINT = true;
#else
            constexpr bool flag_DEBUG_IF_CHECK_POINT = false;
#endif // DEBUG_IF_CHECK_POINT

            BOOST_LOG_OWL(trace) << "MapCalc::receiveMail start";
            if constexpr (flag_DEBUG_IF_CHECK_POINT) {
                BOOST_ASSERT(!weak_from_this().expired());
                BOOST_LOG_OWL(trace) << "MapCalc::receiveMail weak_from_this().use_count(): "
                                     << weak_from_this().use_count();
                BOOST_LOG_OWL(trace) << "MapCalc::receiveMail shared_from_this().use_count(): "
                                     << this->shared_from_this().use_count();
            }
            boost::asio::dispatch(ioc_, [this, self = shared_from_this(), data]() {
                BOOST_ASSERT(self);
                BOOST_LOG_OWL(trace) << "MapCalc::receiveMail dispatch";
                auto mail = std::make_shared<OwlMailDefine::MapCalc2Service>();
                mail->runner = data->callbackRunner;
                if (!calc_) {
                    BOOST_LOG_OWL(trace) << "MapCalc::receiveMail mail (!calc_) send back";
                    return sendMail(std::move(mail));
                }
                BOOST_LOG_OWL(trace) << "MapCalc::receiveMail call calcMapPosition";
                auto r = calcMapPosition(
                        data->tagInfo,
                        data->airplaneState
                );
                if (!r) {
                    BOOST_LOG_OWL(trace) << "MapCalc::receiveMail mail (!r) send back";
                    return sendMail(std::move(mail));
                }
                mail->ok = true;
                mail->info = r;
                BOOST_LOG_OWL(trace) << "MapCalc::receiveMail mail ok send back";
                return sendMail(std::move(mail));
            });
        }

        void sendMail(OwlMailDefine::MailMapCalc2Service &&data) {
            mailbox_->sendB2A(std::move(data));
        }

    };

} // OwlMapCalc

#endif //OWLACCESSTERMINAL_MAPCALC_H
