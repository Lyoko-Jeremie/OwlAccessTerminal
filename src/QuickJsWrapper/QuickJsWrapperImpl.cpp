// jeremie

#include "QuickJsWrapperImpl.h"
#include <boost/log/trivial.hpp>

#include <sstream>
#include <opencv2/opencv.hpp>
#include "./QuickJsH.h"

#include "./MathModule.h"

namespace OwlQuickJsWrapper {

    void logConsole(const qjs::rest<std::string> &args) {
        std::stringstream ss;
        for (auto const &arg: args) ss << arg;
        BOOST_LOG_TRIVIAL(info) << ss.str();
    }

    void warningConsole(const qjs::rest<std::string> &args) {
        std::stringstream ss;
        for (auto const &arg: args) ss << arg;
        BOOST_LOG_TRIVIAL(warning) << ss.str();
    }

    void errorConsole(const qjs::rest<std::string> &args) {
        std::stringstream ss;
        for (auto const &arg: args) ss << arg;
        BOOST_LOG_TRIVIAL(error) << ss.str();
    }


    QuickJsWrapperImpl::QuickJsWrapperImpl()
            : runtime_(),
              context_(runtime_) {

    }

    bool QuickJsWrapperImpl::init() {
        try {
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
        catch (qjs::exception &) {
            auto exc = context_.getException();
            BOOST_LOG_TRIVIAL(error) << "QuickJsWrapperImpl init qjs::exception " << (std::string) exc;
            if ((bool) exc["stack"]) {
                BOOST_LOG_TRIVIAL(error) << "QuickJsWrapperImpl init qjs::exception " << (std::string) exc["stack"];
            }
            return false;
        } catch (cv::Exception &e) {
            BOOST_LOG_TRIVIAL(error) << "QuickJsWrapperImpl init cv::exception :"
                                     << e.what();
            return false;
        }
        return true;
    }

    bool QuickJsWrapperImpl::evalCode(const std::string &codeString) {
        try {
            context_.eval(codeString);
        }
        catch (qjs::exception &) {
            auto exc = context_.getException();
            BOOST_LOG_TRIVIAL(error) << "QuickJsWrapperImpl evalCode qjs::exception " << (std::string) exc;
            if ((bool) exc["stack"]) {
                BOOST_LOG_TRIVIAL(error) << "QuickJsWrapperImpl evalCode qjs::exception " << (std::string) exc["stack"];
            }
            return false;
        } catch (cv::Exception &e) {
            BOOST_LOG_TRIVIAL(error) << "QuickJsWrapperImpl evalCode cv::exception :"
                                     << e.what();
            return false;
        }
        return true;
    }

    bool QuickJsWrapperImpl::loadCode(const std::string &filePath) {
        try {
            context_.evalFile(filePath.c_str());
        }
        catch (qjs::exception &) {
            auto exc = context_.getException();
            BOOST_LOG_TRIVIAL(error) << "QuickJsWrapperImpl loadCode qjs::exception " << (std::string) exc;
            if ((bool) exc["stack"]) {
                BOOST_LOG_TRIVIAL(error) << "QuickJsWrapperImpl loadCode qjs::exception " << (std::string) exc["stack"];
            }
            return false;
        } catch (cv::Exception &e) {
            BOOST_LOG_TRIVIAL(error) << "QuickJsWrapperImpl loadCode cv::exception :"
                                     << e.what();
            return false;
        }
        return true;
    }

} // OwlQuickJsWrapperImpl