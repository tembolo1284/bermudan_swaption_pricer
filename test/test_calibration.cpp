// test/test_calibration.cpp
#include <gtest/gtest.h>

#include "YieldCurveBuilder.hpp"
#include "SwapBuilder.hpp"

#include <ql/settings.hpp>
#include <ql/time/calendars/target.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>
#include <ql/indexes/ibor/euribor.hpp>
#include <ql/quotes/simplequote.hpp>
#include <ql/models/shortrate/calibrationhelpers/swaptionhelper.hpp>
#include <ql/models/shortrate/onefactormodels/hullwhite.hpp>
#include <ql/models/shortrate/onefactormodels/blackkarasinski.hpp>
#include <ql/models/shortrate/twofactormodels/g2.hpp>
#include <ql/pricingengines/swaption/jamshidianswaptionengine.hpp>
#include <ql/pricingengines/swaption/treeswaptionengine.hpp>
#include <ql/pricingengines/swaption/g2swaptionengine.hpp>
#include <ql/pricingengines/swaption/fdg2swaptionengine.hpp>
#include <ql/math/optimization/levenbergmarquardt.hpp>
#include <ql/utilities/dataformatters.hpp>

using namespace QuantLib;

namespace {

// Paper data: swap tenors and 5x5 vol grid (row-major).
static const Integer swapLengths[5] = {1,2,3,4,5};
static const Volatility swaptionVols[25] = {
    0.2050, 0.1920, 0.1830, 0.1740, 0.1620,
    0.1900, 0.1780, 0.1660, 0.1580, 0.1490,
    0.1790, 0.1690, 0.1580, 0.1490, 0.1400,
    0.1680, 0.1580, 0.1490, 0.1390, 0.1300,
    0.1570, 0.1470, 0.1370, 0.1280, 0.1200
};
// We’ll build just the diagonal: 1x5, 2x4, 3x3, 4x2, 5x1 → indices [4, 8, 12, 16, 20].

struct MarketFixture {
    Handle<YieldTermStructure> ts;
    ext::shared_ptr<Euribor6M> index6m;
    Calendar cal;
    Date today, settlement;

    MarketFixture() {
        today = Date(15, July, 2025);
        Settings::instance().evaluationDate() = today;
        cal = TARGET();
        settlement = cal.advance(today, 2, Days);

        YieldCurveBuilder ycb(0.035);
        ts = ycb.buildCurve(settlement);
        index6m = ext::make_shared<Euribor6M>(ts);
    }
};

std::vector<ext::shared_ptr<BlackCalibrationHelper>>
makeDiagonalSwaptions(const MarketFixture& mkt) {
    std::vector<Period> maturities = {
        Period(1, Years), Period(2, Years), Period(3, Years),
        Period(4, Years), Period(5, Years)
    };

    std::vector<ext::shared_ptr<BlackCalibrationHelper>> swaptions;
    swaptions.reserve(5);

    for (Size i = 0; i < 5; ++i) {
        Size j = 5 - i - 1;               // 1x5, 2x4, 3x3, 4x2, 5x1
        Size k = i*5 + j;                  // index in swaptionVols
        auto vol = ext::make_shared<SimpleQuote>(swaptionVols[k]);

        auto helper = ext::make_shared<SwaptionHelper>(
            maturities[i],
            Period(swapLengths[j], Years),
            Handle<Quote>(vol),
            mkt.index6m,
            mkt.index6m->tenor(),
            mkt.index6m->dayCounter(),
            mkt.index6m->dayCounter(),
            mkt.ts
        );
        swaptions.push_back(helper);
    }
    return swaptions;
}

std::list<Time> collectTimes(const std::vector<ext::shared_ptr<BlackCalibrationHelper>>& swaptions) {
    std::list<Time> times;
    for (auto& s : swaptions) s->addTimesTo(times);
    return times;
}

struct ErrorStats {
    double maxAbsErr{};
    double mae{};
};

ErrorStats computeErrors(
    const std::vector<ext::shared_ptr<BlackCalibrationHelper>>& swaptions,
    const std::vector<Volatility>& diagonalMarketVols
) {
    ErrorStats stats{};
    double sum = 0.0;
    for (Size i = 0; i < swaptions.size(); ++i) {
        Real npv = swaptions[i]->modelValue();
        Volatility implied =
            swaptions[i]->impliedVolatility(npv, 1e-4, 1000, 0.05, 0.50);
        double err = std::fabs(implied - diagonalMarketVols[i]);
        stats.maxAbsErr = std::max(stats.maxAbsErr, err);
        sum += err;
    }
    stats.mae = sum / swaptions.size();
    return stats;
}

std::vector<Volatility> diagonalMarketVols() {
    // [1x5, 2x4, 3x3, 4x2, 5x1] = [0.1620, 0.1580, 0.1580, 0.1580, 0.1570]
    return {0.1620, 0.1580, 0.1580, 0.1580, 0.1570};
}

} // namespace

TEST(Calibration, Diagonal_G2pp_HW_BK) {
    MarketFixture mkt;
    auto swaptions = makeDiagonalSwaptions(mkt);

    // Build a time grid for tree engines
    auto times = collectTimes(swaptions);
    TimeGrid grid(times.begin(), times.end(), 30);

    // G2++ (analytic engine for helpers)
    auto g2 = ext::make_shared<G2>(mkt.ts);
    for (auto& s : swaptions)
        s->setPricingEngine(ext::make_shared<G2SwaptionEngine>(g2, 6.0, 16));

    LevenbergMarquardt lm;
    std::vector<ext::shared_ptr<CalibrationHelper>> helpers(swaptions.begin(), swaptions.end());
    g2->calibrate(helpers, lm, EndCriteria(400, 100, 1e-8, 1e-8, 1e-8));

    {
        auto diag = diagonalMarketVols();
        auto stats = computeErrors(swaptions, diag);
        // G2 tolerances (lenient; tighten if your platform is consistent)
        EXPECT_LE(stats.maxAbsErr, 0.015); // 1.5 vol points
        EXPECT_LE(stats.mae, 0.010);
    }

    // Hull–White analytic (Jamshidian)
    auto hw_ana = ext::make_shared<HullWhite>(mkt.ts);
    for (auto& s : swaptions)
        s->setPricingEngine(ext::make_shared<JamshidianSwaptionEngine>(hw_ana));
    hw_ana->calibrate(helpers, lm, EndCriteria(400, 100, 1e-8, 1e-8, 1e-8));
    {
        auto diag = diagonalMarketVols();
        auto stats = computeErrors(swaptions, diag);
        EXPECT_LE(stats.maxAbsErr, 0.010);
        EXPECT_LE(stats.mae, 0.006);
    }

    // Hull–White numerical (tree)
    auto hw_tree = ext::make_shared<HullWhite>(mkt.ts);
    for (auto& s : swaptions)
        s->setPricingEngine(ext::make_shared<TreeSwaptionEngine>(hw_tree, grid));
    hw_tree->calibrate(helpers, lm, EndCriteria(400, 100, 1e-8, 1e-8, 1e-8));
    {
        auto diag = diagonalMarketVols();
        auto stats = computeErrors(swaptions, diag);
        EXPECT_LE(stats.maxAbsErr, 0.015);
        EXPECT_LE(stats.mae, 0.010);
    }

    // Black–Karasinski numerical (tree)
    auto bk = ext::make_shared<BlackKarasinski>(mkt.ts);
    for (auto& s : swaptions)
        s->setPricingEngine(ext::make_shared<TreeSwaptionEngine>(bk, grid));
    bk->calibrate(helpers, lm, EndCriteria(400, 100, 1e-8, 1e-8, 1e-8));
    {
        auto diag = diagonalMarketVols();
        auto stats = computeErrors(swaptions, diag);
        EXPECT_LE(stats.maxAbsErr, 0.015);
        EXPECT_LE(stats.mae, 0.010);
    }

    // Parameter sanity checks
    EXPECT_GE(hw_ana->params()[0], 0.0);
    EXPECT_GE(hw_ana->params()[1], 0.0);
}

