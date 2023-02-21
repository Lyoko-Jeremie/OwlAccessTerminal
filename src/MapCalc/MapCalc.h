// jeremie

#ifndef OWLACCESSTERMINAL_MAPCALC_H
#define OWLACCESSTERMINAL_MAPCALC_H

#include <memory>
#include <functional>
#include <array>
#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <utility>
#include "MapCalcMail.h"
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
                OwlMailDefine::ServiceMapCalcMailbox &&mailbox
        );

        ~MapCalc() {
            BOOST_LOG_TRIVIAL(trace) << "~MapCalc()";
            mailbox_->receiveB2A = nullptr;
        }

        using MapCalcFunctionType =
                std::shared_ptr<std::array<double, 3>>(
                        std::shared_ptr<OwlMailDefine::AprilTagCmd> tagInfo,
                        std::shared_ptr<OwlSerialController::AirplaneState> airplaneState
                );

    private:
        boost::asio::io_context &ioc_;
        OwlMailDefine::ServiceMapCalcMailbox mailbox_;

        std::shared_ptr<OwlQuickJsWrapper::QuickJsWrapper> qjw_;

        std::function<MapCalcFunctionType> calc_;

    public:

        bool loadCalcJsCodeFile(const std::string &filePath);

        bool loadMapCalcFunction(const std::string &functionName);

        bool testMapCalcFunction();

        std::shared_ptr<std::array<double, 3>> calcMapPosition(
                std::shared_ptr<OwlMailDefine::AprilTagCmd> tagInfo,
                std::shared_ptr<OwlSerialController::AirplaneState> airplaneState
        ) {
            if (calc_) {
                return calc_(std::move(tagInfo), std::move(airplaneState));
            }
            return {};
        }

    private:

        void receiveMail(OwlMailDefine::MailService2MapCalc &&data) {
            boost::asio::dispatch(ioc_, [this, self = shared_from_this(), data]() {
                auto mail = std::make_shared<OwlMailDefine::MapCalc2Service>();
                mail->runner = data->callbackRunner;
                if (!calc_) {
                    return sendMail(std::move(mail));
                }
                auto r = calcMapPosition(
                        data->tagInfo,
                        data->airplaneState
                );
                if (!r) {
                    return sendMail(std::move(mail));
                }
                mail->ok = true;
                mail->x = r->at(0);
                mail->y = r->at(1);
                mail->z = r->at(2);
                return sendMail(std::move(mail));
            });
        }

        void sendMail(OwlMailDefine::MailMapCalc2Service &&data) {
            mailbox_->sendB2A(std::move(data));
        }

    };

} // OwlMapCalc

#endif //OWLACCESSTERMINAL_MAPCALC_H
