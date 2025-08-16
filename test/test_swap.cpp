// test/test_swap.cpp
#include <gtest/gtest.h>
#include "YieldCurveBuilder.hpp"
#include "SwapBuilder.hpp"
#include <ql/settings.hpp>
#include <ql/time/calendars/target.hpp>

using namespace QuantLib;

TEST(SwapBuilder, FairRateAndNPVSign) {
    Date today(15, July, 2025);
    Settings::instance().evaluationDate() = today;
    Calendar cal = TARGET();
    Date settlement = cal.advance(today, 2, Days);

    YieldCurveBuilder yc(0.035);
    auto ts = yc.buildCurve(settlement);

    SwapBuilder sb(ts);
    Rate atm = sb.fairRate();
    auto under = sb.buildSwap(atm * 0.95);
    auto over  = sb.buildSwap(atm * 1.05);

    EXPECT_GT(under->NPV(), 0.0); // payer swap at lower than fair -> positive
    EXPECT_LT(over->NPV(), 0.0);  // payer swap at higher than fair -> negative
}

