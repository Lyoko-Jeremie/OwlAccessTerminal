// jeremie

#ifndef OWLACCESSTERMINAL_LOG_H
#define OWLACCESSTERMINAL_LOG_H

#include <string>
#include <boost/log/core.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/severity_feature.hpp>
#include <boost/log/sources/record_ostream.hpp>


namespace OwlLog {

    enum severity_level {
        trace,
        debug,
        info,
        warning,
        error,
        fatal,
        MAX
    };

    extern boost::log::sources::severity_logger<severity_level> slg;

    extern thread_local std::string threadName;

    // https://stackoverflow.com/questions/60977433/including-thread-name-in-boost-log
    void init_logging();


} // OwlLog

#ifndef BOOST_LOG_OWL
#define BOOST_LOG_OWL(lvl) BOOST_LOG_SEV(OwlLog::slg, OwlLog::severity_level::lvl )
#endif // BOOST_LOG_OWL

#endif //OWLACCESSTERMINAL_LOG_H
