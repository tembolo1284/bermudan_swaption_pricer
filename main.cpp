#include "YieldCurveBuilder.hpp"
#include "SwapBuilder.hpp"
#include "BermudanSwaptionPricer.hpp"

#include <ql/settings.hpp>
#include <ql/time/calendars/target.hpp>
#include <ql/models/shortrate/onefactormodels/hullwhite.hpp>
#include <iostream>

using namespace QuantLib;

int main() {
    try {
        Date today(15, July, 2025);
        Settings::instance().evaluationDate() = today;

        Calendar cal = TARGET();
        Date settlement = cal.advance(today, 2, Days);

        YieldCurveBuilder ycb(0.035);
        auto ts = ycb.buildCurve(settlement);

        SwapBuilder sb(ts);
        Rate atm = sb.fairRate();

        auto atmSwap = sb.buildSwap(atm);
        auto otmSwap = sb.buildSwap(atm * 1.2);
        auto itmSwap = sb.buildSwap(atm * 0.8);

        auto hw = ext::make_shared<HullWhite>(ts);

        BermudanSwaptionPricer pATM(atmSwap, hw, "tree");
        BermudanSwaptionPricer pOTM(otmSwap, hw, "tree");
        BermudanSwaptionPricer pITM(itmSwap, hw, "tree");

        std::cout << "ATM: " << pATM.price() << "\n";
        std::cout << "OTM: " << pOTM.price() << "\n";
        std::cout << "ITM: " << pITM.price() << "\n";
        return 0;
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

