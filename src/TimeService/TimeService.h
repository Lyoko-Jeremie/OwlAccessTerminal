// jeremie

#ifndef OWLACCESSTERMINAL_TIMESERVICE_H
#define OWLACCESSTERMINAL_TIMESERVICE_H

#include "../MemoryBoost.h"
#include <functional>
#include <chrono>
#include <atomic>
#include <boost/asio.hpp>
#include "../OwlLog/OwlLog.h"
#include "./TimeServiceMail.h"

namespace OwlTimeService {

    class TimeService : public boost::enable_shared_from_this<TimeService> {
    public:
        TimeService(
                boost::asio::io_context &ioc,
                OwlMailDefine::ServiceTimeMailbox &&mailbox
        ) : ioc_(ioc), mailbox_(mailbox) {

            mailbox_->receiveA2B([this](OwlMailDefine::MailService2Time &&data) {
                receiveMail(std::move(data));
            });
        }

        ~TimeService() {
            BOOST_LOG_OWL(trace_dtor) << "~TimeService()";
            mailbox_->receiveB2A(nullptr);
        }

    private:
        boost::asio::io_context &ioc_;
        OwlMailDefine::ServiceTimeMailbox mailbox_;

        std::atomic_int64_t timeDiffMs{0};

        int64_t setNowClockToUpdateDiffAndGetNowSteadyClock(int64_t timestampMs) {
            // https://stackoverflow.com/questions/32300972/how-to-properly-convert-a-unix-timestamp-string-to-time-t-in-c11
            // https://en.cppreference.com/w/cpp/chrono/time_point/time_since_epoch
            // https://en.cppreference.com/w/cpp/chrono/duration/duration_cast
            auto tMs = std::chrono::time_point<std::chrono::steady_clock>{std::chrono::milliseconds(timestampMs)};
            auto nowT = std::chrono::steady_clock::now();
            std::chrono::milliseconds diff = std::chrono::duration_cast<std::chrono::milliseconds>(nowT - tMs);
            timeDiffMs.store(diff.count());
            return std::chrono::time_point_cast<std::chrono::milliseconds>(nowT).time_since_epoch().count();
        }

        int64_t getDiff() {
            return timeDiffMs.load();
        }

        int64_t getNowSteadyClock() {
            auto nowT = std::chrono::steady_clock::now();
            return std::chrono::time_point_cast<std::chrono::milliseconds>(nowT).time_since_epoch().count();
        }

        int64_t getNowSyncClock() {
            auto nowT = std::chrono::steady_clock::now();
            auto syncT = nowT - std::chrono::milliseconds{timeDiffMs.load()};
            return std::chrono::time_point_cast<std::chrono::milliseconds>(syncT).time_since_epoch().count();
        }


    private:

        void receiveMail(OwlMailDefine::MailService2Time &&data) {
            boost::asio::dispatch(ioc_, [this, self = shared_from_this(), data]() {
                switch (data->cmd) {
                    case OwlMailDefine::TimeServiceCmd::getSteadyClock: {
                        auto data_r = boost::make_shared<OwlMailDefine::Time2Service>();
                        data_r->runner = data->callbackRunner;
                        data_r->clockTimestampMs = getNowSteadyClock();
                        sendMail(std::move(data_r));
                        return;
                    }
                        break;
                    case OwlMailDefine::TimeServiceCmd::getSyncClock: {
                        auto data_r = boost::make_shared<OwlMailDefine::Time2Service>();
                        data_r->runner = data->callbackRunner;
                        data_r->clockTimestampMs = getNowSyncClock();
                        sendMail(std::move(data_r));
                        return;
                    }
                        break;
                    case OwlMailDefine::TimeServiceCmd::setNowClock: {
                        auto data_r = boost::make_shared<OwlMailDefine::Time2Service>();
                        data_r->runner = data->callbackRunner;
                        data_r->clockTimestampMs = setNowClockToUpdateDiffAndGetNowSteadyClock(data->clockTimestampMs);
                        sendMail(std::move(data_r));
                        return;
                    }
                        break;
                    case OwlMailDefine::TimeServiceCmd::ignore:
                    default: {
                        BOOST_LOG_OWL(error) << "TimeService receiveMail switch(data->cmd) TimeServiceCmd ignore..";
                        auto data_r = boost::make_shared<OwlMailDefine::Time2Service>();
                        data_r->runner = data->callbackRunner;
                        sendMail(std::move(data_r));
                        return;
                    }
                        break;
                }
            });
        }

        void sendMail(OwlMailDefine::MailTime2Service &&data) {
            // send cmd to camera
            mailbox_->sendB2A(std::move(data));
        }


    };

} // OwlTimeService

#endif //OWLACCESSTERMINAL_TIMESERVICE_H
