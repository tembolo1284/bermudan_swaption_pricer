#include "SwapBuilder.hpp"

#include <ql/pricingengines/swap/discountingswapengine.hpp>
#include <ql/time/calendars/target.hpp>
#include <ql/time/daycounters/thirty360.hpp>
#include <ql/time/dategenerationrule.hpp>

using namespace QuantLib;

namespace {
    constexpr Frequency kFixedLegFrequency = Annual;
    constexpr Frequency kFloatLegFrequency = Semiannual;
    const BusinessDayConvention kFixedLegBDC = Unadjusted;
    const BusinessDayConvention kFloatLegBDC = ModifiedFollowing;
    const DayCounter kFixedLegDC = Thirty360(Thirty360::European);
}

SwapBuilder::SwapBuilder(const Handle<YieldTermStructure>& termStructure)
    : termStructure_(termStructure) {
    QL_REQUIRE(!termStructure_.empty(), "Term structure handle is empty");

    Date settlementDate = termStructure_->referenceDate();
    Calendar cal = TARGET();

    Date start = cal.advance(settlementDate, 1, Years, kFloatLegBDC);
    Date maturity = cal.advance(start, 5, Years, kFloatLegBDC);

    fixedSchedule_ = Schedule(start, maturity, Period(kFixedLegFrequency),
                              cal, kFixedLegBDC, kFixedLegBDC,
                              DateGeneration::Forward, false);

    floatSchedule_ = Schedule(start, maturity, Period(kFloatLegFrequency),
                              cal, kFloatLegBDC, kFloatLegBDC,
                              DateGeneration::Forward, false);

    indexSixMonths_ = ext::make_shared<Euribor6M>(termStructure_);
}

ext::shared_ptr<VanillaSwap>
SwapBuilder::buildSwap(Rate fixedRate) const {
    auto swap = ext::make_shared<VanillaSwap>(
        Swap::Payer,
        1000.0,
        fixedSchedule_, fixedRate, kFixedLegDC,
        floatSchedule_, ext::static_pointer_cast<IborIndex>(indexSixMonths_), 0.0,
        indexSixMonths_->dayCounter());

    swap->setPricingEngine(ext::make_shared<DiscountingSwapEngine>(termStructure_));
    return swap;
}

Rate SwapBuilder::fairRate() const {
    auto swap = ext::make_shared<VanillaSwap>(
        Swap::Payer,
        1000.0,
        fixedSchedule_, /*dummy*/ 0.03, kFixedLegDC,
        floatSchedule_, ext::static_pointer_cast<IborIndex>(indexSixMonths_), 0.0,
        indexSixMonths_->dayCounter());

    swap->setPricingEngine(ext::make_shared<DiscountingSwapEngine>(termStructure_));
    return swap->fairRate();
}

Schedule SwapBuilder::fixedSchedule() const { return fixedSchedule_; }
Schedule SwapBuilder::floatSchedule() const { return floatSchedule_; }

