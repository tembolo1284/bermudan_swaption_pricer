#ifndef SWAPTION_CALIBRATOR_HPP
#define SWAPTION_CALIBRATOR_HPP

#include <vector>
#include <ql/models/shortrate/calibrationhelpers/swaptionhelper.hpp>

namespace QuantLib {
    class CalibratedModel;      // declares calibrate(...)
    class OptimizationMethod;   // forward-declare
    class BlackCalibrationHelper;
}

class SwaptionCalibrator {
public:
    SwaptionCalibrator(
        const std::vector<QuantLib::ext::shared_ptr<QuantLib::BlackCalibrationHelper>>& swaptions,
        const std::vector<double>& marketVols,
        const std::vector<int>& swapLengths
    );

    // QL 1.25 requires non-const OptimizationMethod&
    void calibrateModel(const QuantLib::ext::shared_ptr<QuantLib::CalibratedModel>& model,
                        QuantLib::OptimizationMethod& method);

private:
    std::vector<QuantLib::ext::shared_ptr<QuantLib::BlackCalibrationHelper>> swaptions_;
    std::vector<double> marketVols_;
    std::vector<int> swapLengths_;
};

#endif // SWAPTION_CALIBRATOR_HPP

