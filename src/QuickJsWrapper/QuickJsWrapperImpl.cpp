// jeremie

#include "QuickJsWrapperImpl.h"
#include "../Log/Log.h"
#include <boost/exception/diagnostic_information.hpp>

#include <sstream>
#include <opencv2/opencv.hpp>
#include "./QuickJsH.h"

#include "./MathModule.h"

namespace OwlQuickJsWrapper {

    void logConsole(const qjs::rest<std::string> &args) {
        std::stringstream ss;
        for (auto const &arg: args) ss << arg;
        BOOST_LOG_OWL(info) << ss.str();
    }

    void warningConsole(const qjs::rest<std::string> &args) {
        std::stringstream ss;
        for (auto const &arg: args) ss << arg;
        BOOST_LOG_OWL(warning) << ss.str();
    }

    void errorConsole(const qjs::rest<std::string> &args) {
        std::stringstream ss;
        for (auto const &arg: args) ss << arg;
        BOOST_LOG_OWL(error) << ss.str();
    }


    QuickJsWrapperImpl::QuickJsWrapperImpl()
            : runtime_(),
              context_(runtime_) {

    }

    bool QuickJsWrapperImpl::init() {
        try {

            BOOST_LOG_OWL(trace) << "QuickJsWrapperImpl init JS_SetMemoryLimit begin";
//            JS_SetMemoryLimit(runtime_.rt, std::numeric_limits<size_t>::max());
            JS_SetMaxStackSize(runtime_.rt, 0);
            BOOST_LOG_OWL(trace) << "QuickJsWrapperImpl init JS_SetMaxStackSize end";

            installMathModule(context_);
            installMathModuleExtend(context_);
            installMathExOpenCVModule(context_);
            context_.eval(R"xxx(
                import * as Math from 'Math';
                import * as MathEx from 'MathEx';
                import * as MathExOpenCV from 'MathExOpenCV';
                globalThis.Math = Math;
                globalThis.MathEx = MathEx;
                globalThis.MathExOpenCV = MathExOpenCV;
                )xxx", "<import>", JS_EVAL_TYPE_MODULE);

            auto & module = context_.addModule("console");
            module.function<&logConsole>("log");
            module.function<&warningConsole>("warning");
            module.function<&errorConsole>("error");
            context_.eval(R"xxx(
                import * as consoleC from 'console';
                globalThis.console = consoleC;
                )xxx", "<import>", JS_EVAL_TYPE_MODULE);
        }
        catch (qjs::exception &e) {
            try {
                auto exc = context_.getException();
                BOOST_LOG_OWL(error) << "QuickJsWrapperImpl init qjs::exception " << (std::string) exc;
                if ((bool) exc["stack"]) {
                    BOOST_LOG_OWL(error) << "QuickJsWrapperImpl init qjs::exception " << (std::string) exc["stack"];
                }
            } catch (...) {
                BOOST_LOG_OWL(error) << "QuickJsWrapperImpl::init qjs::exception&e catch (...) exception"
                                         << "\n current_exception_diagnostic_information : "
                                         << boost::current_exception_diagnostic_information();
            }
            return false;
        } catch (cv::Exception &e) {
            BOOST_LOG_OWL(error) << "QuickJsWrapperImpl init cv::exception :"
                                     << e.what();
            return false;
        }
        return true;
    }

    bool QuickJsWrapperImpl::evalCode(const std::string &codeString) {
        try {
            context_.eval(codeString);
        }
        catch (qjs::exception &e) {
            try {
                auto exc = context_.getException();
                BOOST_LOG_OWL(error) << "QuickJsWrapperImpl evalCode qjs::exception " << (std::string) exc;
                if ((bool) exc["stack"]) {
                    BOOST_LOG_OWL(error) << "QuickJsWrapperImpl evalCode qjs::exception "
                                             << (std::string) exc["stack"];
                }
            } catch (...) {
                BOOST_LOG_OWL(error) << "QuickJsWrapperImpl::evalCode qjs::exception&e catch (...) exception"
                                         << "\n current_exception_diagnostic_information : "
                                         << boost::current_exception_diagnostic_information();
            }
            return false;
        } catch (cv::Exception &e) {
            BOOST_LOG_OWL(error) << "QuickJsWrapperImpl evalCode cv::exception :"
                                     << e.what();
            return false;
        }
        return true;
    }

    bool QuickJsWrapperImpl::loadCode(const std::string &filePath) {
        try {
            BOOST_LOG_OWL(trace) << "QuickJsWrapperImpl loadCode context_.evalFile(filePath.c_str()) begin";
            context_.evalFile(filePath.c_str());
            BOOST_LOG_OWL(trace) << "QuickJsWrapperImpl loadCode context_.evalFile(filePath.c_str()) end";
        }
        catch (qjs::exception &e) {
            try {
                auto exc = context_.getException();
                BOOST_LOG_OWL(error) << "QuickJsWrapperImpl loadCode qjs::exception " << (std::string) exc;
                if ((bool) exc["stack"]) {
                    BOOST_LOG_OWL(error) << "QuickJsWrapperImpl loadCode qjs::exception "
                                             << (std::string) exc["stack"];
                }
            } catch (...) {
                BOOST_LOG_OWL(error) << "QuickJsWrapperImpl::loadCode qjs::exception&e catch (...) exception"
                                         << "\n current_exception_diagnostic_information : "
                                         << boost::current_exception_diagnostic_information();
            }
            return false;
        } catch (cv::Exception &e) {
            BOOST_LOG_OWL(error) << "QuickJsWrapperImpl loadCode cv::exception :"
                                     << e.what();
            return false;
        }
        return true;
    }

    void QuickJsWrapperImpl::trigger_qjs_update_when_thread_change() {
        JS_UpdateStackTop(runtime_.rt);
    }

} // OwlQuickJsWrapperImpl