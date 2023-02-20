// jeremie

#ifndef OWLACCESSTERMINAL_QUICKJSWRAPPER_H
#define OWLACCESSTERMINAL_QUICKJSWRAPPER_H

#include <memory>
#include <functional>
#include "./QuickJsWrapperImpl.h"

namespace OwlQuickJsWrapper {

    class QuickJsWrapper : public std::enable_shared_from_this<QuickJsWrapper> {
    public:
        QuickJsWrapper();

        void init() {
            impl_->init();
        }

        bool evalCode(const std::string &codeString) {
            return impl_->evalCode(codeString);
        }

        bool loadCode(const std::string &filePath) {
            return impl_->loadCode(filePath);
        }

        template<typename FunctionT = void(const std::string &), typename RT = std::function<FunctionT> >
        RT getCallbackFunction(const std::string &functionName) {
            return impl_->getCallbackFunction<FunctionT, RT>(functionName);
        }

        qjs::Context &getContext() {
            return impl_->getContext();
        }


    private:
        std::shared_ptr<QuickJsWrapperImpl> impl_;
    };

} // OwlQuickJsWrapper

#endif //OWLACCESSTERMINAL_QUICKJSWRAPPER_H
