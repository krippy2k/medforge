#pragma once

#include <array>

namespace medforge::rendering {

struct CameraPose {
    std::array<double, 3> position{{0, 0, 0}};
    std::array<double, 3> focalPoint{{0, 0, 0}};
    std::array<double, 3> viewUp{{0, 0, 1}};
};

CameraPose homeCamera();
CameraPose orbitYaw(const CameraPose& pose, double degrees);
CameraPose pan(const CameraPose& pose, double right, double up);
CameraPose zoom(const CameraPose& pose, double factor);
bool cameraPosesNear(const CameraPose& left, const CameraPose& right, double tolerance);

} // namespace medforge::rendering
