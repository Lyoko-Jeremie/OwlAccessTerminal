// jeremie

#ifndef OWLACCESSTERMINAL_TIMESERVICEMAIL_H
#define OWLACCESSTERMINAL_TIMESERVICEMAIL_H

#include "../MemoryBoost.h"
#include <functional>
#include <chrono>
#include "../AsyncCallbackMailbox/AsyncCallbackMailbox.h"

namespace OwlMailDefine {

    enum class TimeServiceCmd {
        ignore = 0,
        setNowClock = 1,
        getSyncClock,
        getSteadyClock,
    };

    struct Time2Service;
    struct Service2Time {
        TimeServiceCmd cmd = TimeServiceCmd::ignore;

        int64_t clockTimestampMs = 0;

        // Time2Service.runner = Service2Time.callbackRunner
        std::function<void(boost::shared_ptr<Time2Service>)> callbackRunner;
    };
    struct Time2Service {

        int64_t clockTimestampMs = 0;

        std::function<void(boost::shared_ptr<Time2Service>)> runner;
    };
    using ServiceTimeMailbox =
            boost::shared_ptr<
                    OwlAsyncCallbackMailbox::AsyncCallbackMailbox<
                            Service2Time,
                            Time2Service
                    >
            >;

    using MailService2Time = ServiceTimeMailbox::element_type::A2B_t;
    using MailTime2Service = ServiceTimeMailbox::element_type::B2A_t;

}


#endif //OWLACCESSTERMINAL_TIMESERVICEMAIL_H
