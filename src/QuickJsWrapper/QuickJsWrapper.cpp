// jeremie

#include "QuickJsWrapper.h"
#include "../Log/Log.h"


namespace OwlQuickJsWrapper {


    QuickJsWrapper::QuickJsWrapper() : impl_(std::make_shared<QuickJsWrapperImpl>()) {}



} // OwlQuickJsWrapper