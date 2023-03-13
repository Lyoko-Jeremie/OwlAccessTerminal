// jeremie

#include "MathModule.h"
#include <cmath>
#include <limits>
#include <random>
#include <opencv2/opencv.hpp>
#include "../OwlLog/OwlLog.h"

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
        return sqrt(pow(x, 2) + pow(y, 2));
    });
    module.function("maxIndex", [](const qjs::rest<double> &l) -> long long {
        return (std::max_element(l.begin(), l.end()) - l.begin());
    });
    module.function("atan2Deg", [](double y, double x) -> double {
        auto r = std::atan2(y, x);
//        BOOST_LOG_OWL(trace) << "atan2Deg r 1 :" << r;
        r = r / (M_PI / 180.0);
//        BOOST_LOG_OWL(trace) << "atan2Deg r 2 :" << r;
//        if (r >= 0) {
//            return r;
//        } else {
//            return r + 180;
//        }
//        return r + 180;
        return r;
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
//        BOOST_LOG_OWL(trace) << "getAffineTransform m :" << m;
        return std::vector<double>{
                m.begin<double>(), m.end<double>()
        };
    });
    module.function("invertAffineTransform", [](
            const std::vector<double> &mIn
    ) -> std::vector<double> {
        if (mIn.size() % 2 != 0) {
            return {};
        }
        cv::Mat m{mIn, true};
        m = m.reshape(1, 2);
        cv::Mat mOut;
        cv::invertAffineTransform(m, mOut);
        return std::vector<double>{
                mOut.begin<double>(), mOut.end<double>()
        };
    });
    module.function("transform", [](
            const std::vector<double> &pArray,
            const std::vector<double> &mIn
    ) -> std::vector<double> {
        if (pArray.size() % 2 != 0) {
            return {};
        }
        if (mIn.size() % 2 != 0) {
            return {};
        }
        cv::Mat m{mIn, true};
        m = m.reshape(1, 2);
        cv::Mat ps{pArray, true};
        ps = ps.reshape(2);
//        BOOST_LOG_OWL(trace) << "transform m :" << m;
//        BOOST_LOG_OWL(trace) << "transform m cols :" << m.cols;
//        BOOST_LOG_OWL(trace) << "transform ps :" << ps;
//        BOOST_LOG_OWL(trace) << "transform ps channels :" << ps.channels();
        cv::Mat mOut;
        cv::transform(ps, mOut, m);
//        BOOST_LOG_OWL(trace) << "transform mOut :" << mOut;
        mOut = mOut.reshape(1);
        return std::vector<double>{
                mOut.begin<double>(), mOut.end<double>()
        };
    });
    module.function("convexHull", [](
            const std::vector<double> &pArray
    ) -> std::vector<double> {
        if (pArray.size() % 2 != 0) {
            return {};
        }
        cv::Mat ps{pArray, true};
        ps = ps.reshape(2);
        cv::Mat pIndexOut;
        cv::convexHull(ps, pIndexOut);
        return std::vector<double>{
                pIndexOut.begin<double>(), pIndexOut.end<double>()
        };
    });
    module.function("minEnclosingCircle", [](
            const std::vector<double> &pArray
    ) -> std::vector<double> {
        if (pArray.size() % 2 != 0) {
            return {};
        }
        cv::Mat ps{pArray, true};
        ps = ps.reshape(2);
        cv::Point2f center;
        float radius;
        cv::minEnclosingCircle(ps, center, radius);
        return std::vector<double>{
                center.x, center.y, radius,
        };
    });
    module.function("circleBorderPoints", [](
            double centerX, double centerY, double radius,
            const std::vector<double> &pArray
    ) -> std::vector<double> {
        if (pArray.size() % 2 != 0) {
            return {};
        }
        cv::Mat pX{cv::Size{static_cast<int>(pArray.size() / 2), 1}, CV_32FC1, cv::Scalar{0}};
        cv::Mat pY{cv::Size{static_cast<int>(pArray.size() / 2), 1}, CV_32FC1, cv::Scalar{0}};
        for (size_t i = 0; i < pArray.size(); ++i) {
            if (i % 2 == 0) {
                pX.at<double>(std::floor(i / 2)) = pArray.at(i);
            } else {
                pY.at<double>(std::floor(i / 2)) = pArray.at(i);
            }
        }
        pX -= centerX;
        pY -= centerY;
        auto d = pX * pX + pY * pY;
        d = radius - d;
        cv::Mat pD = d;
        cv::threshold(pD, pD, 0.001, 1, cv::ThresholdTypes::THRESH_BINARY);
        std::vector<int> pI;
        pI.reserve(pArray.size() / 2);
        for (int i = 0; i < pD.cols; ++i) {
            if (pD.at<double>(i) == 0) {
                pI.push_back(i);
            }
        }

        return std::vector<double>{
                pI.begin(), pI.end()
        };
    });
    module.function("boundingRect", [](
            const std::vector<double> &pArray
    ) -> std::vector<int> {
        if (pArray.size() % 2 != 0) {
            return {};
        }
        cv::Mat ps{pArray, true};
        ps = ps.reshape(2);
        auto rect = cv::boundingRect(ps);
        return std::vector<int>{
                rect.x, rect.y, rect.width, rect.height,
        };
    });
    module.function("minAreaRect", [](
            const std::vector<double> &pArray
    ) -> std::vector<double> {
        if (pArray.size() % 2 != 0) {
            return {};
        }
        cv::Mat ps{pArray, true};
        ps = ps.reshape(2);
        auto rect = cv::minAreaRect(ps);
        return std::vector<double>{
                rect.center.x, rect.center.y,
                rect.size.width, rect.size.height,
                rect.angle,
        };
    });
    module.function("minAreaRect_boxPoints", [](
            const std::vector<double> &pArray
    ) -> std::vector<double> {
        if (pArray.size() % 2 != 0) {
            return {};
        }
        cv::Mat ps{pArray, true};
        ps = ps.reshape(2);
        auto rect = cv::minAreaRect(ps);
        std::vector<cv::Point2f> p;
        cv::boxPoints(rect, p);
        return std::vector<double>{
                p.at(0).x, p.at(0).y,
                p.at(1).x, p.at(1).y,
                p.at(2).x, p.at(2).y,
                p.at(3).x, p.at(3).y,
                rect.center.x, rect.center.y,
                rect.size.width, rect.size.height,
                rect.angle,
        };
    });
}















