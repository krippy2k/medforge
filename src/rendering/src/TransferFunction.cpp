#include "medforge/rendering/TransferFunction.h"

#include <algorithm>

namespace medforge::rendering {
namespace {

TransferFunction makeFunction(const char* name, std::vector<TransferStop> stops) {
    TransferFunction function;
    function.name = name;
    function.stops = std::move(stops);
    return function;
}

} // namespace

TransferFunction softTissueTransfer() {
    return makeFunction("Soft",
                        {{-1000, 0.0, 0.0, 0.0, 0.0},
                         {-150, 0.75, 0.45, 0.40, 0.0},
                         {40, 0.86, 0.55, 0.48, 0.12},
                         {200, 0.90, 0.75, 0.65, 0.25},
                         {1500, 1.0, 1.0, 1.0, 0.02}});
}

TransferFunction lungTransfer() {
    return makeFunction("Lung",
                        {{-1000, 0.0, 0.0, 0.0, 0.0},
                         {-800, 0.40, 0.55, 0.65, 0.08},
                         {-500, 0.70, 0.75, 0.80, 0.18},
                         {-200, 0.80, 0.50, 0.45, 0.02},
                         {400, 1.0, 0.95, 0.90, 0.15}});
}

TransferFunction boneTransfer() {
    return makeFunction("Bone",
                        {{-1000, 0.0, 0.0, 0.0, 0.0},
                         {150, 0.90, 0.85, 0.75, 0.0},
                         {300, 0.95, 0.92, 0.85, 0.35},
                         {800, 1.0, 0.98, 0.94, 0.85},
                         {1500, 1.0, 1.0, 1.0, 0.95}});
}

double opacityAt(const TransferFunction& function, double hu) {
    if (function.stops.empty()) {
        return 0;
    }
    if (hu <= function.stops.front().hu) {
        return function.stops.front().opacity;
    }
    if (hu >= function.stops.back().hu) {
        return function.stops.back().opacity;
    }
    for (std::size_t index = 1; index < function.stops.size(); ++index) {
        const TransferStop& left = function.stops[index - 1];
        const TransferStop& right = function.stops[index];
        if (hu > right.hu) {
            continue;
        }
        const double span = right.hu - left.hu;
        const double t = span == 0 ? 0 : (hu - left.hu) / span;
        return left.opacity + (t * (right.opacity - left.opacity));
    }
    return function.stops.back().opacity;
}

} // namespace medforge::rendering
