#include "YieldCurveBuilder.hpp"

#include <ql/quotes/simplequote.hpp>
#include <ql/termstructures/yield/flatforward.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>

using namespace QuantLib;

YieldCurveBuilder::YieldCurveBuilder(double flatRate)
    : flatRate_(flatRate) {}

Handle<YieldTermStructure>
YieldCurveBuilder::buildCurve(const Date& settlementDate) const {
    auto q = ext::make_shared<SimpleQuote>(flatRate_);
    Handle<Quote> hq(q);
    auto ts = ext::make_shared<FlatForward>(settlementDate, hq, Actual365Fixed());
    return Handle<YieldTermStructure>(ts);
}

