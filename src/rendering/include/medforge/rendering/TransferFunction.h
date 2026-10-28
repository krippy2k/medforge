#pragma once

#include <string>
#include <vector>

namespace medforge::rendering {

struct TransferStop {
    double hu = 0;
    double red = 0;
    double green = 0;
    double blue = 0;
    double opacity = 0;
};

struct TransferFunction {
    std::string name;
    std::vector<TransferStop> stops;
};

TransferFunction softTissueTransfer();
TransferFunction lungTransfer();
TransferFunction boneTransfer();
double opacityAt(const TransferFunction& function, double hu);

} // namespace medforge::rendering
