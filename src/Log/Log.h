// jeremie

#ifndef OWLACCESSTERMINAL_LOG_H
#define OWLACCESSTERMINAL_LOG_H

#include <string>
#include <boost/log/trivial.hpp>

namespace OwlLog {

    extern thread_local std::string threadName;

    // https://stackoverflow.com/questions/60977433/including-thread-name-in-boost-log
    void init_logging();


} // OwlLog

#endif //OWLACCESSTERMINAL_LOG_H
