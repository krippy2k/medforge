#pragma once

#include "medforge/core/Coordinates.h"

#include <array>

namespace medforge::rendering {

enum class AnatomicalView { Anterior, Posterior, Left, Right, Superior, Inferior };

struct CameraPose {
    std::array<double, 3> position{{0, 0, 0}};
    std::array<double, 3> focalPoint{{0, 0, 0}};
    std::array<double, 3> viewUp{{0, 0, 1}};
};

CameraPose anatomicalCamera(AnatomicalView view,
                            const core::Vec3& boundsMin,
                            const core::Vec3& boundsMax);
CameraPose homeCamera();
CameraPose orbitYaw(const CameraPose& pose, double degrees);
CameraPose pan(const CameraPose& pose, double right, double up);
CameraPose zoom(const CameraPose& pose, double factor);
bool cameraPosesNear(const CameraPose& left, const CameraPose& right, double tolerance);

} // namespace medforge::rendering
