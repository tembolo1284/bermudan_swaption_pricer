#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "YieldCurveBuilder.hpp"
#include "SwapBuilder.hpp"
#include "BermudanSwaptionPricer.hpp"

#include <ql/settings.hpp>
#include <ql/time/calendars/target.hpp>
#include <ql/models/shortrate/onefactormodels/hullwhite.hpp>
#include <ql/models/shortrate/twofactormodels/g2.hpp>
#include <ql/models/shortrate/onefactormodels/blackkarasinski.hpp>

namespace py = pybind11;
using namespace QuantLib;

namespace {

// Build a flat TS on the requested date
Handle<YieldTermStructure> make_ts(const Date& today, double flat_rate) {
    Settings::instance().evaluationDate() = today;
    Calendar cal = TARGET();
    Date settlement = cal.advance(today, 2, Days);
    YieldCurveBuilder ycb(flat_rate);
    return ycb.buildCurve(settlement);
}

ext::shared_ptr<ShortRateModel>
make_model(const std::string& name, const Handle<YieldTermStructure>& ts) {
    if (name == "g2")  return ext::make_shared<G2>(ts);
    if (name == "hw")  return ext::make_shared<HullWhite>(ts);
    if (name == "bk")  return ext::make_shared<BlackKarasinski>(ts);
    QL_FAIL("Unknown model: " << name << " (use 'g2' | 'hw' | 'bk')");
}

} // namespace

PYBIND11_MODULE(bermudan_native, m) {
    m.doc() = "Pybind11 bindings for Bermudan swaption pricer (QuantLib 1.25 compatible)";

    // price_bermudan(...) -> double
    m.def("price_bermudan",
        [](int year, int month, int day,
           double flat_rate,
           const std::string& model_name,
           const std::string& engine,      // "tree" | "fdm"
           double strike_multiplier) {     // 1.0=ATM, 1.2=OTM, 0.8=ITM
            Date today(day, static_cast<Month>(month), year);
            Handle<YieldTermStructure> ts = make_ts(today, flat_rate);

            SwapBuilder sb(ts);
            Rate atm = sb.fairRate();
            ext::shared_ptr<VanillaSwap> swap = sb.buildSwap(atm * strike_multiplier);

            ext::shared_ptr<ShortRateModel> model = make_model(model_name, ts);

            BermudanSwaptionPricer pricer(swap, model, engine);
            return pricer.price();
        },
        py::arg("year"), py::arg("month"), py::arg("day"),
        py::arg("flat_rate"),
        py::arg("model_name"),
        py::arg("engine"),
        py::arg("strike_multiplier")
    );
}

