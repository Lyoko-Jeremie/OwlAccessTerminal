// jeremie

#ifndef TESTQUICKJSPP_MATHMODULE_H
#define TESTQUICKJSPP_MATHMODULE_H

#include "./QuickJsH.h"

extern void installMathModule(qjs::Context &context);

extern void installMathModuleExtend(qjs::Context &context, const std::string &moduleName = "MathEx");

#endif //TESTQUICKJSPP_MATHMODULE_H
