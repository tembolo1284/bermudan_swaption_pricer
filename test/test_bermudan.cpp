#include <gtest/gtest.h>

#include "YieldCurveBuilder.hpp"
#include "SwapBuilder.hpp"
#include "BermudanSwaptionPricer.hpp"

#include <ql/settings.hpp>
#include <ql/time/calendars/target.hpp>
#include <ql/models/shortrate/onefactormodels/hullwhite.hpp>

using namespace QuantLib;

TEST(BermudanSwaptionPricer, MonotonicityHW) {
    // Market setup
    Date today(15, July, 2025);
    Settings::instance().evaluationDate() = today;

    Calendar cal = TARGET();
    Date settlement = cal.advance(today, 2, Days);

    // Flat curve @ 3.5%
    YieldCurveBuilder ycb(0.035);
    Handle<YieldTermStructure> ts = ycb.buildCurve(settlement);

    // Build baseline swap & strikes
    SwapBuilder sb(ts);
    Rate atm = sb.fairRate();

    ext::shared_ptr<VanillaSwap> atmSwap = sb.buildSwap(atm);
    ext::shared_ptr<VanillaSwap> otmSwap = sb.buildSwap(atm * 1.20);
    ext::shared_ptr<VanillaSwap> itmSwap = sb.buildSwap(atm * 0.80);

    // Model
    ext::shared_ptr<HullWhite> hw = ext::make_shared<HullWhite>(ts);

    // Price with tree engine (default grid)
    BermudanSwaptionPricer pATM(atmSwap, hw, "tree");
    BermudanSwaptionPricer pOTM(otmSwap, hw, "tree");
    BermudanSwaptionPricer pITM(itmSwap, hw, "tree");

    const double vATM = pATM.price();
    const double vOTM = pOTM.price();
    const double vITM = pITM.price();

    // Basic sanity & monotonicity checks
    EXPECT_GE(vITM, 0.0);
    EXPECT_GE(vATM, 0.0);
    EXPECT_GE(vOTM, 0.0);

    // ITM >= ATM >= OTM (allow tiny numerical slack)
    EXPECT_GE(vITM + 1e-10, vATM);
    EXPECT_GE(vATM + 1e-10, vOTM);
}

