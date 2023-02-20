// jeremie

#include "QuickJsWrapper.h"
#include <boost/log/trivial.hpp>


namespace OwlQuickJsWrapper {


    QuickJsWrapper::QuickJsWrapper() : impl_(std::make_shared<QuickJsWrapperImpl>()) {}



} // OwlQuickJsWrapper