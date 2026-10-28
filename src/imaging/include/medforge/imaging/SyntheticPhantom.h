#pragma once

#include <filesystem>
#include <string>

namespace medforge::imaging::phantom {

inline constexpr int kColumns = 8;
inline constexpr int kRows = 6;
inline constexpr int kSlices = 4;
inline constexpr double kOriginX = 10;
inline constexpr double kOriginY = -20;
inline constexpr double kOriginZ = 0;
inline constexpr double kColumnSpacingMm = 0.8;
inline constexpr double kRowSpacingMm = 0.5;
inline constexpr double kSliceSpacingMm = 2;
inline constexpr double kSlope = 2;
inline constexpr double kIntercept = -1024;
inline constexpr int kMarkerI = 3;
inline constexpr int kMarkerJ = 2;
inline constexpr int kMarkerK = 1;
inline constexpr double kMarkerHu = 400;
inline constexpr double kAirHu = -1000;
inline constexpr const char* kSeriesUid = "1.2.826.0.1.3680043.10.543.100";

// Four axial slices. Air is -1000 HU. Slice k has column i == k at 80 HU.
// Pixel (3, 2) on slice 1 is 400 HU. Spacing is 0.8 x 0.5 x 2 mm.
bool writeSyntheticAxialSeries(const std::filesystem::path& directory, std::string* error);

} // namespace medforge::imaging::phantom
