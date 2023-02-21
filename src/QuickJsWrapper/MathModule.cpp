// jeremie

#include "MathModule.h"
#include <cmath>
#include <random>

namespace MathRandom {
    // https://en.cppreference.com/w/cpp/numeric/random/uniform_int_distribution
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> distribReal(0, 1);
}

void installMathModule(qjs::Context &context) {
    auto & module = context.addModule("Math");
    // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Math
    module.add("E", M_E);
    module.add("LN2", M_LN2);
    module.add("LN10", M_LN10);
    module.add("LOG2E", M_LOG2E);
    module.add("LOG10E", M_LOG10E);
    module.add("PI", M_PI);
    module.add("SQRT1_2", M_SQRT1_2);
    module.add("SQRT2", M_SQRT2);
    module.function<static_cast<double (*)(double)>(&::abs)>("abs");
    module.function<static_cast<double (*)(double)>(&::acos)>("acos");
    module.function<static_cast<double (*)(double)>(&::acosh)>("acosh");
    module.function<static_cast<double (*)(double)>(&::asin)>("asin");
    module.function<static_cast<double (*)(double)>(&::asinh)>("asinh");
    module.function<static_cast<double (*)(double)>(&::atan)>("atan");
    module.function<static_cast<double (*)(double)>(&::atanh)>("atanh");
    module.function<static_cast<double (*)(double, double)>(&::atan2)>("atan2");
    module.function<static_cast<double (*)(double)>(&::cbrt)>("cbrt");
    module.function<static_cast<double (*)(double)>(&::ceil)>("ceil");
//    module.function<static_cast<double (*)(double)>(&clz32)>("clz32");
    module.function<static_cast<double (*)(double)>(&::cos)>("cos");
    module.function<static_cast<double (*)(double)>(&::cosh)>("cosh");
    module.function<static_cast<double (*)(double)>(&::exp)>("exp");
    module.function<static_cast<double (*)(double)>(&::expm1)>("expm1");
    module.function<static_cast<double (*)(double)>(&::floor)>("floor");
//    module.function<static_cast<double (*)(double)>(&::fround)>("fround");
    module.function<static_cast<double (*)(double, double)>(&::hypot)>("hypot");
//    module.function<static_cast<double (*)(double)>(&::imul)>("imul");
    module.function<&::log>("log");
    module.function<&::log1p>("log1p");
    module.function<&::log10>("log10");
    module.function<&::log2>("log2");
//    module.function("max", );
//    module.function("min", );
    module.function<static_cast<double (*)(double, double)>(&::pow)>("pow");
    module.function("random", []() -> double {
        auto a = MathRandom::distribReal(MathRandom::gen);
        return a;
    });
    module.function<static_cast<double (*)(double)>(&::round)>("round");
//    module.function<static_cast<double (*)(double)>(&::sign)>("sign");
    module.function<static_cast<double (*)(double)>(&::sin)>("sin");
    module.function<static_cast<double (*)(double)>(&::sinh)>("sinh");
    module.function<static_cast<double (*)(double)>(&::sqrt)>("sqrt");
    module.function<static_cast<double (*)(double)>(&::tan)>("tan");
    module.function<static_cast<double (*)(double)>(&::tanh)>("tanh");
    module.function<static_cast<double (*)(double)>(&::trunc)>("trunc");
}


void installMathModuleExtend(qjs::Context &context, const std::string &moduleName /*= "MathEx"*/) {
    auto & module = context.addModule(moduleName.c_str());
    // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Math

    module.function("degToRad", [](double degrees) -> double {
        return degrees * (M_PI / 180.0);
    });
    module.function("radToDeg", [](double rad) -> double {
        return rad / (M_PI / 180);
    });
    module.function("randomInt2", [](int min, int max) -> double {
        std::uniform_int_distribution<> distribInt(min, max);
        return distribInt(MathRandom::gen);
    });
    module.function("distance", [](int p1x, int p1y, int p2x, int p2y) -> double {
        return sqrt(pow((p1x - p2x), 2) + pow((p1y - p2y), 2));
    });
    module.function("distanceFast", [](int p1x, int p1y, int p2x, int p2y) -> double {
        return (pow((p1x - p2x), 2) + pow((p1y - p2y), 2));
    });
}















