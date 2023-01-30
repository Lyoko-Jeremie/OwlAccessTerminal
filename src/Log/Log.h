// jeremie

#ifndef OWLACCESSTERMINAL_LOG_H
#define OWLACCESSTERMINAL_LOG_H

#include <string>
#include <boost/log/trivial.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/attributes.hpp>

namespace OwlLog {

    extern thread_local std::string threadName;

    class thread_name_impl :
            public boost::log::attribute::impl {
    public:
        boost::log::attribute_value get_value() override {
            return boost::log::attributes::make_attribute_value(
                    OwlLog::threadName.empty() ? std::string("no name") : OwlLog::threadName);
        }

        using value_type = std::string;
    };

    class thread_name :
            public boost::log::attribute {
    public:
        thread_name() : boost::log::attribute(new thread_name_impl()) {
        }

        explicit thread_name(boost::log::attributes::cast_source const &source)
                : boost::log::attribute(source.as<thread_name_impl>()) {
        }

        using value_type = thread_name_impl::value_type;

    };

    // https://stackoverflow.com/questions/60977433/including-thread-name-in-boost-log
    void init_logging();


} // OwlLog

#endif //OWLACCESSTERMINAL_LOG_H
