// jeremie

#include "OwlLog.h"
#include <boost/assert.hpp>
//#include <boost/stacktrace.hpp>

namespace boost {
    void assertion_failed(char const *expr, char const *function, char const *file, long line) {
        BOOST_LOG_OWL(error)
            << "assertion_failed : [" << expr << "]"
            << " on function [" << function << "]"
            << " on file [" << file << "]"
            << " at line [" << line << "]";
//            << " stacktrace:\n" << boost::stacktrace::stacktrace();
        std::abort();
    }

    void assertion_failed_msg(char const *expr, char const *msg, char const *function, char const *file, long line) {
        BOOST_LOG_OWL(error)
            << "assertion_failed_msg : [" << expr << "]"
            << " msg [" << msg << "]"
            << " on function [" << function << "]"
            << " on file [" << file << "]"
            << " at line [" << line << "]";
//            << " stacktrace:\n" << boost::stacktrace::stacktrace();
        std::abort();
    }
}

