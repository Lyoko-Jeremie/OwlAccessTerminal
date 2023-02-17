// jeremie

#include "Log.h"
#include <boost/core/null_deleter.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/attributes.hpp>

namespace OwlLog {
    thread_local std::string threadName;

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

    void init_logging() {
        boost::shared_ptr<boost::log::core> core = boost::log::core::get();

        typedef boost::log::sinks::synchronous_sink<boost::log::sinks::text_ostream_backend> sink_t;
        boost::shared_ptr<sink_t> sink(new sink_t());
        sink->locked_backend()->add_stream(boost::shared_ptr<std::ostream>(&std::cout, boost::null_deleter()));
        sink->set_formatter(
                boost::log::expressions::stream
                        // << "["
                        // << std::setw(5)
                        // << boost::log::expressions::attr<unsigned int>("LineID")
                        // << "]"
                        << "["
                        << boost::log::expressions::format_date_time<boost::posix_time::ptime>(
                                "TimeStamp", "%Y-%m-%d %H:%M:%S.%f")
                        << "]"
                        // << "["
                        // << boost::log::expressions::attr<boost::log::attributes::current_process_id::value_type>(
                        //         "ProcessID")
                        // << "]"
                        // << "["
                        // << boost::log::expressions::attr<boost::log::attributes::current_process_name::value_type>(
                        //         "ProcessName")
                        // << "]"
                        << "["
                        << boost::log::expressions::attr<boost::log::attributes::current_thread_id::value_type>(
                                "ThreadID")
                        << "]"
                        << "["
                        << std::setw(20)
                        << boost::log::expressions::attr<thread_name::value_type>("ThreadName")
                        << "]"
                        << "[" << boost::log::trivial::severity << "] "
                        << boost::log::expressions::smessage);
        core->add_sink(sink);

        // https://www.boost.org/doc/libs/1_81_0/libs/log/doc/html/log/detailed/attributes.html
        // core->add_global_attribute("LineID", boost::log::attributes::counter<size_t>(1));
        core->add_global_attribute("ThreadName", thread_name());
        core->add_global_attribute("ThreadID", boost::log::attributes::current_thread_id());
        core->add_global_attribute("TimeStamp", boost::log::attributes::local_clock());
        // core->add_global_attribute("ProcessID", boost::log::attributes::current_process_id());
        // core->add_global_attribute("ProcessName", boost::log::attributes::current_process_name());

        // https://stackoverflow.com/questions/69967084/how-to-set-the-severity-level-of-boost-log-library
        // core->get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::info);

    }
} // OwlLog