#ifndef SWAP_BUILDER_HPP
#define SWAP_BUILDER_HPP

#include <ql/handle.hpp>
#include <ql/indexes/ibor/euribor.hpp>
#include <ql/instruments/vanillaswap.hpp>
#include <ql/time/schedule.hpp>

class SwapBuilder {
public:
    explicit SwapBuilder(const QuantLib::Handle<QuantLib::YieldTermStructure>& termStructure);

    QuantLib::ext::shared_ptr<QuantLib::VanillaSwap> buildSwap(QuantLib::Rate fixedRate) const;

    QuantLib::Rate fairRate() const;
    QuantLib::Schedule fixedSchedule() const;
    QuantLib::Schedule floatSchedule() const;

private:
    QuantLib::Handle<QuantLib::YieldTermStructure> termStructure_;
    QuantLib::ext::shared_ptr<QuantLib::Euribor6M> indexSixMonths_;
    QuantLib::Schedule fixedSchedule_;
    QuantLib::Schedule floatSchedule_;
};

#endif // SWAP_BUILDER_HPP

