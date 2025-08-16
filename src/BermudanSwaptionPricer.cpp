#include "BermudanSwaptionPricer.hpp"

#include <ql/cashflows/coupon.hpp>
#include <ql/pricingengines/swaption/treeswaptionengine.hpp>
#include <ql/pricingengines/swaption/fdhullwhiteswaptionengine.hpp>
#include <ql/pricingengines/swaption/fdg2swaptionengine.hpp>
#include <ql/models/shortrate/onefactormodels/hullwhite.hpp>
#include <ql/models/shortrate/twofactormodels/g2.hpp>
#include <ql/models/shortrate/onefactormodels/blackkarasinski.hpp>

using namespace QuantLib;

BermudanSwaptionPricer::BermudanSwaptionPricer(
    const ext::shared_ptr<VanillaSwap>& swap,
    const ext::shared_ptr<ShortRateModel>& model,
    const std::string& engineType,
    const TimeGrid* grid)
    : swap_(swap),
      model_(model),
      engineType_(engineType),
      grid_(grid) {
    QL_REQUIRE(swap_, "Null swap");
    QL_REQUIRE(model_, "Null model");
}

double BermudanSwaptionPricer::price() {
    std::vector<Date> bermudanDates;
    const auto& leg = swap_->fixedLeg();
    bermudanDates.reserve(leg.size());
    for (const auto& cf : leg) {
        auto cpn = ext::dynamic_pointer_cast<Coupon>(cf);
        if (cpn)
            bermudanDates.push_back(cpn->accrualStartDate());
    }

    auto exercise = ext::make_shared<BermudanExercise>(bermudanDates);
    Swaption bermudan(swap_, exercise);

    auto modelG2 = ext::dynamic_pointer_cast<G2>(model_);
    auto modelHW = ext::dynamic_pointer_cast<HullWhite>(model_);
    auto modelBK = ext::dynamic_pointer_cast<BlackKarasinski>(model_);

    if (engineType_ == "fdm") {
        if (modelG2) {
            bermudan.setPricingEngine(ext::make_shared<FdG2SwaptionEngine>(modelG2));
        } else if (modelHW) {
            bermudan.setPricingEngine(ext::make_shared<FdHullWhiteSwaptionEngine>(modelHW));
        } else {
            bermudan.setPricingEngine(ext::make_shared<TreeSwaptionEngine>(
                model_, grid_ ? *grid_ : TimeGrid(50, 50)));
        }
    } else {
        if (grid_) {
            bermudan.setPricingEngine(ext::make_shared<TreeSwaptionEngine>(model_, *grid_));
        } else {
            bermudan.setPricingEngine(ext::make_shared<TreeSwaptionEngine>(model_, 50));
        }
    }

    return bermudan.NPV();
}

