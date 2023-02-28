// jeremie

#ifndef OWLACCESSTERMINAL_QUICKJSWRAPPERIMPL_H
#define OWLACCESSTERMINAL_QUICKJSWRAPPERIMPL_H

#include <memory>
#include <functional>
#include <boost/log/trivial.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include "./QuickJsH.h"


namespace OwlQuickJsWrapper {


    class QuickJsWrapperImpl : public std::enable_shared_from_this<QuickJsWrapperImpl> {

    public:
        QuickJsWrapperImpl();

        bool init();

        bool evalCode(const std::string &codeString);

        bool loadCode(const std::string &filePath);

        template<typename FunctionT = void(const std::string &), typename RT = std::function<FunctionT> >
        RT getCallbackFunction(const std::string &functionName) {
            try {
                auto cb = (std::function<FunctionT>) context_.eval(functionName);
                return cb;
            }
            catch (qjs::exception &e) {
                try {
                    auto exc = context_.getException();
                    BOOST_LOG_TRIVIAL(error) << "QuickJsWrapperImpl getCallbackFunction qjs::exception "
                                             << (std::string) exc;
                    if ((bool) exc["stack"]) {
                        BOOST_LOG_TRIVIAL(error) << "QuickJsWrapperImpl getCallbackFunction qjs::exception "
                                                 << (std::string) exc["stack"];
                    }
                } catch (...) {
                    BOOST_LOG_TRIVIAL(error)
                        << "QuickJsWrapperImpl getCallbackFunction qjs::exception&e catch (...) exception"
                        << "\n current_exception_diagnostic_information : "
                        << boost::current_exception_diagnostic_information();
                }
                return {};
            }
        }

        qjs::Context &getContext() {
            return context_;
        }

        void trigger_qjs_update_when_thread_change();

    private:
        qjs::Runtime runtime_;
        qjs::Context context_;


    };


} // OwlQuickJsWrapperImpl

#endif //OWLACCESSTERMINAL_QUICKJSWRAPPERIMPL_H
