#ifndef BERMUDAN_SWAPTION_PRICER_HPP
#define BERMUDAN_SWAPTION_PRICER_HPP

#include <ql/instruments/swaption.hpp>
#include <ql/time/date.hpp>
#include <memory>
#include <string>
#include <vector>

namespace QuantLib {
    class VanillaSwap;
    class ShortRateModel;
    class TimeGrid;
}

class BermudanSwaptionPricer {
public:
    BermudanSwaptionPricer(
        const QuantLib::ext::shared_ptr<QuantLib::VanillaSwap>& swap,
        const QuantLib::ext::shared_ptr<QuantLib::ShortRateModel>& model,
        const std::string& engineType,
        const QuantLib::TimeGrid* grid = nullptr
    );

    double price();

private:
    QuantLib::ext::shared_ptr<QuantLib::VanillaSwap>   swap_;
    QuantLib::ext::shared_ptr<QuantLib::ShortRateModel> model_;
    std::string engineType_;
    const QuantLib::TimeGrid* grid_;
};

#endif // BERMUDAN_SWAPTION_PRICER_HPP

