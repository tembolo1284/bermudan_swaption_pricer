// test/test_curve.cpp
#include <gtest/gtest.h>
#include "YieldCurveBuilder.hpp"
#include <ql/settings.hpp>
#include <ql/time/calendars/target.hpp>

using namespace QuantLib;

TEST(YieldCurveBuilder, FlatCurveBuilds) {
    Date today(15, July, 2025);
    Settings::instance().evaluationDate() = today;

    Calendar cal = TARGET();
    Date settlement = cal.advance(today, 2, Days);

    YieldCurveBuilder builder(0.035);
    auto handle = builder.buildCurve(settlement);

    ASSERT_FALSE(handle.empty());
    EXPECT_EQ(handle->referenceDate(), settlement);

    // Simple DF sanity: ~ e^(-r * t)
    Time t1 = 1.0;
    double df1 = handle->discount(settlement + Period(1, Years));
    double approx = std::exp(-0.035 * t1);
    EXPECT_NEAR(df1, approx, 1e-3);
}

