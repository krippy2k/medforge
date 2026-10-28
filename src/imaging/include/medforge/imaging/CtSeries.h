#pragma once

#include "medforge/core/Coordinates.h"

#include <array>
#include <string_view>
#include <vector>

namespace medforge::imaging {

enum class SeriesGeometryStatus {
    Ok,
    Empty,
    DuplicateSlice,
    InconsistentOrientation,
    InconsistentSpacing,
    InsufficientSlices,
    NonPositiveSpacing,
    InvalidOrientation
};

enum class TransferSyntaxClass { Uncompressed, Compressed, Unrecognized };

struct SliceGeometry {
    std::array<double, 6> imageOrientationPatient{};
    core::Vec3 imagePositionPatient{};
    double rowSpacingMm = 0;
    double columnSpacingMm = 0;
};

struct SeriesGeometry {
    SeriesGeometryStatus status = SeriesGeometryStatus::Empty;
    bool missingSlice = false;
    int sliceCount = 0;
    core::VoxelGridGeometry geometry{};
};

struct Rescale {
    double slope = 1;
    double intercept = 0;
};

struct WindowLevel {
    double widthHu = 1;
    double centerHu = 0;

    static WindowLevel softTissue();
    static WindowLevel lung();
    static WindowLevel bone();
    static WindowLevel brain();
};

struct HuWindow {
    double lowerHu = 0;
    double upperHu = 0;
};

SeriesGeometry assessSeriesGeometry(const std::vector<SliceGeometry>& slices);
double storedValueToHu(double storedValue, const Rescale& rescale);
HuWindow windowBoundsHu(const WindowLevel& windowLevel);
TransferSyntaxClass classifyTransferSyntax(std::string_view uid);

} // namespace medforge::imaging
