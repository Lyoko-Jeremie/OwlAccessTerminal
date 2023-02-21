// jeremie

#include "QuickJsWrapperImpl.h"
#include <boost/log/trivial.hpp>

#include "./QuickJsH.h"

#include "./MathModule.h"

namespace OwlQuickJsWrapper {

    void logConsole(const std::string &s) {
        BOOST_LOG_TRIVIAL(info) << s;
    }

    void warningConsole(const std::string &s) {
        BOOST_LOG_TRIVIAL(warning) << s;
    }

    void errorConsole(const std::string &s) {
        BOOST_LOG_TRIVIAL(error) << s;
    }


    QuickJsWrapperImpl::QuickJsWrapperImpl()
            : runtime_(),
              context_(runtime_) {

    }

    bool QuickJsWrapperImpl::init() {
        try {
            installMathModule(context_);
            installMathModuleExtend(context_);
            context_.eval(R"xxx(
                import * as Math from 'Math';
                globalThis.Math = Math;
                )xxx", "<import>", JS_EVAL_TYPE_MODULE);

            auto & module = context_.addModule("console");
            module.function<&logConsole>("log");
            module.function<&warningConsole>("warning");
            module.function<&errorConsole>("error");
            context_.eval(R"xxx(
                import * as console from 'console';
                globalThis.console = console;
                )xxx", "<import>", JS_EVAL_TYPE_MODULE);
        }
        catch (qjs::exception &) {
            auto exc = context_.getException();
            BOOST_LOG_TRIVIAL(error) << "QuickJsWrapperImpl init qjs::exception " << (std::string) exc;
            if ((bool) exc["stack"]) {
                BOOST_LOG_TRIVIAL(error) << "QuickJsWrapperImpl init qjs::exception " << (std::string) exc["stack"];
            }
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
        }
        return true;
    }

} // OwlQuickJsWrapperImpl