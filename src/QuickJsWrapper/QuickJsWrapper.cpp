// jeremie

#include "QuickJsWrapper.h"
#include "../OwlLog/OwlLog.h"


namespace OwlQuickJsWrapper {


    QuickJsWrapper::QuickJsWrapper() : impl_(boost::make_shared<QuickJsWrapperImpl>()) {}


} // OwlQuickJsWrapper