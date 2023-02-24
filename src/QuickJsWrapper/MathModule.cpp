// jeremie

#include "MathModule.h"
#include <cmath>
#include <limits>
#include <random>
#include <opencv2/opencv.hpp>

namespace MathRandom {
    // https://en.cppreference.com/w/cpp/numeric/random/uniform_int_distribution
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> distribReal(0, 1);
}

// https://stackoverflow.com/questions/1903954/is-there-a-standard-sign-function-signum-sgn-in-c-c
template<typename T>
int sgn(T val) {
    return (T(0) < val) - (val < T(0));
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
    // https://stackoverflow.com/questions/9772348/get-absolute-value-without-using-abs-function-nor-if-statement
    // https://stackoverflow.com/questions/2878076/what-is-different-about-c-math-h-abs-compared-to-my-abs
//    module.function("abs", [](double a) -> double {
//        if (a < 0) {
//            return 0 - a;
//        }
//        return a;
//    });
    module.function<static_cast<double (*)(double)>(&std::abs)>("abs");
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
    module.function<static_cast<double (*)(double)>(&std::log)>("log");
    module.function<static_cast<double (*)(double)>(&std::log1p)>("log1p");
    module.function<static_cast<double (*)(double)>(&std::log10)>("log10");
    module.function<static_cast<double (*)(double)>(&std::log2)>("log2");
    module.function("max", [](const qjs::rest<double> &l) -> double {
        return *std::max_element(l.begin(), l.end());
    });
    module.function("min", [](const qjs::rest<double> &l) -> double {
        return *std::min_element(l.begin(), l.end());
    });
    module.function<static_cast<double (*)(double, double)>(&::pow)>("pow");
    module.function("random", []() -> double {
        auto a = MathRandom::distribReal(MathRandom::gen);
        return a;
    });
    module.function<static_cast<double (*)(double)>(&::round)>("round");
    module.function("sign", [](double n) -> double {
//        return n < 0 ? -1 : (n > 0 ? 1 : (n == 0 ? 0 : std::numeric_limits<double>::quiet_NaN()));
        return sgn(n);
    });
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
    module.function("distance", [](double p1x, double p1y, double p2x, double p2y) -> double {
        return sqrt(pow((p1x - p2x), 2) + pow((p1y - p2y), 2));
    });
    module.function("distanceFast", [](double p1x, double p1y, double p2x, double p2y) -> double {
        return (pow((p1x - p2x), 2) + pow((p1y - p2y), 2));
    });
    module.function("pythagoreanDistance", [](double x, double y) -> double {
        return (pow(x, 2) + pow(y, 2));
    });
    module.function("maxIndex", [](const qjs::rest<double> &l) -> long long {
        return (std::max_element(l.begin(), l.end()) - l.begin());
    });
}


void installMathExOpenCVModule(qjs::Context &context, const std::string &moduleName /*= "MathExOpenCV"*/) {
    auto & module = context.addModule(moduleName.c_str());
    // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Math

    module.function("getRotationMatrix2D", [](
            double px, double py,
            double angle, double scale
    ) -> std::vector<double> {
        auto m = cv::getRotationMatrix2D(
                cv::Point2f{static_cast<float>(px), static_cast<float>(py)},
                angle, scale
        );
        return std::vector<double>{
                m.begin<double>(), m.end<double>()
        };
    });
    module.function("getAffineTransform", [](
            double p1xSrc, double p1ySrc,
            double p2xSrc, double p2ySrc,
            double p3xSrc, double p3ySrc,
            double p1xDst, double p1yDst,
            double p2xDst, double p2yDst,
            double p3xDst, double p3yDst
    ) -> std::vector<double> {
        auto m = cv::getAffineTransform(
                std::vector<cv::Point2f>{
                        cv::Point2f{static_cast<float>(p1xSrc), static_cast<float>(p1ySrc)},
                        cv::Point2f{static_cast<float>(p2xSrc), static_cast<float>(p2ySrc)},
                        cv::Point2f{static_cast<float>(p3xSrc), static_cast<float>(p3ySrc)},
                },
                std::vector<cv::Point2f>{
                        cv::Point2f{static_cast<float>(p1xDst), static_cast<float>(p1yDst)},
                        cv::Point2f{static_cast<float>(p2xDst), static_cast<float>(p2yDst)},
                        cv::Point2f{static_cast<float>(p3xDst), static_cast<float>(p3yDst)},
                }
        );
        return std::vector<double>{
                m.begin<double>(), m.end<double>()
        };
    });
    module.function("invertAffineTransform", [](
            std::vector<double> mIn
    ) -> std::vector<double> {
        cv::Mat m{mIn, true};
        m = m.reshape(3, 2);
        cv::Mat mOut;
        cv::invertAffineTransform(m, mOut);
        return std::vector<double>{
                mOut.begin<double>(), mOut.end<double>()
        };
    });
}















