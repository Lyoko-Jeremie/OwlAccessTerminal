// jeremie

#ifndef OWLACCESSTERMINAL_OWLLOG_H
#define OWLACCESSTERMINAL_OWLLOG_H

#include <string>
#include <boost/log/core.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/severity_feature.hpp>
#include <boost/log/sources/record_ostream.hpp>

#include "../VERSION/ProgramVersion.h"
#include "../VERSION/CodeVersion.h"

namespace OwlLog {

    enum severity_level {
        trace_sp_tag,
        trace_cmd_tag,
        trace_cmd_http,
        trace_cmd_sp_w,
        trace_cmd_sp_r,
        trace_map,
        trace_camera_reader,
        trace_dtor,
        trace_json,
        trace_multicast,
        trace,
        debug_sp_w,
        debug,
        info,
        info_VSERION,
        warning,
        error,
        fatal,
        MAX
    };

    extern boost::log::sources::severity_logger<severity_level> slg;
    extern const char *severity_level_str[severity_level::MAX];

    extern thread_local std::string threadName;

    // https://stackoverflow.com/questions/60977433/including-thread-name-in-boost-log
    void init_logging();


} // OwlLog

#ifndef BOOST_LOG_OWL
#define BOOST_LOG_OWL(lvl) BOOST_LOG_SEV(OwlLog::slg, OwlLog::severity_level::lvl )
#endif // BOOST_LOG_OWL

#endif //OWLACCESSTERMINAL_OWLLOG_H
