#include "SwaptionCalibrator.hpp"

#include <ql/models/model.hpp>                            // CalibratedModel
#include <ql/math/optimization/levenbergmarquardt.hpp>
#include <ql/math/optimization/endcriteria.hpp>
#include <ql/models/shortrate/calibrationhelpers/swaptionhelper.hpp>
#include <ql/math/optimization/constraint.hpp>            // Constraint (singular header in QL 1.25)
#include <algorithm>

using namespace QuantLib;

SwaptionCalibrator::SwaptionCalibrator(
    const std::vector<QuantLib::ext::shared_ptr<QuantLib::BlackCalibrationHelper>>& swaptions,
    const std::vector<double>& marketVols,
    const std::vector<int>& swapLengths)
    : swaptions_(swaptions),
      marketVols_(marketVols),
      swapLengths_(swapLengths) {}

void SwaptionCalibrator::calibrateModel(
    const QuantLib::ext::shared_ptr<QuantLib::CalibratedModel>& model,
    QuantLib::OptimizationMethod& method) {

    QL_REQUIRE(model, "Null model");
    QL_REQUIRE(!swaptions_.empty(), "No swaptions provided");

    std::vector<QuantLib::ext::shared_ptr<CalibrationHelper>> helpers(swaptions_.begin(),
                                                                      swaptions_.end());

    EndCriteria ec(400, 100, 1e-8, 1e-8, 1e-8);

    // Use full overload for QL 1.25
    model->calibrate(helpers, method, ec, Constraint(), std::vector<double>(), std::vector<bool>());
}

