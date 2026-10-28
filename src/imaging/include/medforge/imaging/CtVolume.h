#pragma once

#include "medforge/imaging/DicomRead.h"

#include <cstdint>
#include <string>
#include <vector>

namespace medforge::imaging {

enum class VolumeStatus { Ok, Empty, Rejected };

struct CtVolume {
    std::string seriesInstanceUid;
    SeriesGeometry geometry{};
    int columns = 0;
    int rows = 0;
    int slices = 0;
    std::vector<float> hu;
    bool missingSlice = false;

    float huAt(int i, int j, int k) const;
};

struct VolumeBuild {
    VolumeStatus status = VolumeStatus::Empty;
    SeriesGeometryStatus geometryStatus = SeriesGeometryStatus::Empty;
    std::string message;
    CtVolume volume;
};

VolumeBuild buildCtVolume(const std::vector<DicomCtInstance>& instances);

enum class MprPlane { Axial, Sagittal, Coronal };

struct MprImage {
    int width = 0;
    int height = 0;
    std::vector<float> hu;
};

struct ViewLabels {
    char left = '?';
    char right = '?';
    char top = '?';
    char bottom = '?';
};

struct MprCrosshair {
    int i = 0;
    int j = 0;
    int k = 0;
};

struct PlanePixel {
    int x = 0;
    int y = 0;
};

MprImage extractPlane(const CtVolume& volume, MprPlane plane, int index);
ViewLabels orientationLabels(const CtVolume& volume, MprPlane plane);
std::uint8_t huToDisplayGray(float hu, const WindowLevel& windowLevel);
void scrollCrosshair(MprCrosshair& crosshair, MprPlane plane, int delta, const CtVolume& volume);
void moveCrosshairOnPlane(MprCrosshair& crosshair,
                          MprPlane plane,
                          int x,
                          int y,
                          const CtVolume& volume);
PlanePixel crosshairPixel(const MprCrosshair& crosshair, MprPlane plane);
core::CoordinateResult crosshairLps(const MprCrosshair& crosshair, const CtVolume& volume);

} // namespace medforge::imaging
