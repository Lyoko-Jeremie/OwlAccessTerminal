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

        void evalCode(const std::string &codeString) {
            impl_->evalCode(codeString);
        }

        template<typename FunctionT = void(const std::string &), typename RT = std::function<FunctionT> >
        RT getCallbackFunction(const std::string &functionName) {
            return impl_->getCallbackFunction<FunctionT, RT>(functionName);
        }

    private:
        std::shared_ptr<QuickJsWrapperImpl> impl_;
    };

} // OwlQuickJsWrapper

#endif //OWLACCESSTERMINAL_QUICKJSWRAPPER_H
