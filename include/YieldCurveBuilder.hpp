#ifndef YIELD_CURVE_BUILDER_HPP
#define YIELD_CURVE_BUILDER_HPP

#include <ql/termstructures/yieldtermstructure.hpp>
#include <ql/quotes/simplequote.hpp>
#include <ql/handle.hpp>
#include <memory>

class YieldCurveBuilder {
public:
    explicit YieldCurveBuilder(double flatRate);

    QuantLib::Handle<QuantLib::YieldTermStructure> buildCurve(const QuantLib::Date& settlementDate) const;

private:
    double flatRate_;
};

#endif // YIELD_CURVE_BUILDER_HPP

