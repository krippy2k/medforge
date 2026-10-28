#include "medforge/imaging/CtSeries.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace medforge::imaging {
namespace {

constexpr double kOrientationTolerance = 1e-4;
constexpr double kPositionToleranceMm = 1e-3;
constexpr double kIntegerTolerance = 1e-3;

bool isFinite(double value) {
    return std::isfinite(value);
}

bool isFinite(const core::Vec3& value) {
    return isFinite(value.x) && isFinite(value.y) && isFinite(value.z);
}

double dot(const core::Vec3& a, const core::Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

core::Vec3 cross(const core::Vec3& a, const core::Vec3& b) {
    return core::Vec3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

double length(const core::Vec3& value) {
    return std::sqrt(dot(value, value));
}

core::Vec3 rowDirection(const SliceGeometry& slice) {
    return core::Vec3{slice.imageOrientationPatient[0], slice.imageOrientationPatient[1],
                      slice.imageOrientationPatient[2]};
}

core::Vec3 columnDirection(const SliceGeometry& slice) {
    return core::Vec3{slice.imageOrientationPatient[3], slice.imageOrientationPatient[4],
                      slice.imageOrientationPatient[5]};
}

bool sameOrientation(const SliceGeometry& a, const SliceGeometry& b) {
    for (std::size_t index = 0; index < a.imageOrientationPatient.size(); ++index) {
        if (std::abs(a.imageOrientationPatient[index] - b.imageOrientationPatient[index]) >
            kOrientationTolerance) {
            return false;
        }
    }
    return true;
}

SeriesGeometry failure(SeriesGeometryStatus status) {
    SeriesGeometry result;
    result.status = status;
    return result;
}

} // namespace

WindowLevel WindowLevel::softTissue() {
    return WindowLevel{400, 40};
}

WindowLevel WindowLevel::lung() {
    return WindowLevel{1500, -600};
}

WindowLevel WindowLevel::bone() {
    return WindowLevel{2000, 300};
}

WindowLevel WindowLevel::brain() {
    return WindowLevel{80, 40};
}

SeriesGeometry assessSeriesGeometry(const std::vector<SliceGeometry>& slices) {
    if (slices.empty()) {
        return failure(SeriesGeometryStatus::Empty);
    }
    if (slices.size() < 2) {
        return failure(SeriesGeometryStatus::InsufficientSlices);
    }

    const core::Vec3 row = rowDirection(slices.front());
    const core::Vec3 column = columnDirection(slices.front());
    if (!isFinite(row) || !isFinite(column) ||
        std::abs(length(row) - 1.0) > kOrientationTolerance ||
        std::abs(length(column) - 1.0) > kOrientationTolerance ||
        std::abs(dot(row, column)) > kOrientationTolerance) {
        return failure(SeriesGeometryStatus::InvalidOrientation);
    }
    const core::Vec3 normal = cross(row, column);

    std::vector<double> positions;
    positions.reserve(slices.size());
    for (const SliceGeometry& slice : slices) {
        if (!sameOrientation(slices.front(), slice)) {
            return failure(SeriesGeometryStatus::InconsistentOrientation);
        }
        if (!isFinite(slice.imagePositionPatient) || !isFinite(slice.rowSpacingMm) ||
            !isFinite(slice.columnSpacingMm)) {
            return failure(SeriesGeometryStatus::InvalidOrientation);
        }
        if (slice.rowSpacingMm <= 0 || slice.columnSpacingMm <= 0) {
            return failure(SeriesGeometryStatus::NonPositiveSpacing);
        }
        if (std::abs(slice.rowSpacingMm - slices.front().rowSpacingMm) > kPositionToleranceMm ||
            std::abs(slice.columnSpacingMm - slices.front().columnSpacingMm) >
                kPositionToleranceMm) {
            return failure(SeriesGeometryStatus::InconsistentSpacing);
        }
        positions.push_back(dot(slice.imagePositionPatient, normal));
    }

    std::sort(positions.begin(), positions.end());
    for (std::size_t index = 1; index < positions.size(); ++index) {
        if (positions[index] - positions[index - 1] <= kPositionToleranceMm) {
            return failure(SeriesGeometryStatus::DuplicateSlice);
        }
    }

    double step = positions[1] - positions[0];
    for (std::size_t index = 2; index < positions.size(); ++index) {
        step = std::min(step, positions[index] - positions[index - 1]);
    }
    for (std::size_t index = 1; index < positions.size(); ++index) {
        const double multiples = (positions[index] - positions[index - 1]) / step;
        const double nearest = std::round(multiples);
        if (nearest < 1 || std::abs(multiples - nearest) > kIntegerTolerance) {
            return failure(SeriesGeometryStatus::InconsistentSpacing);
        }
    }
    const double span = (positions.back() - positions.front()) / step;
    const int spanSteps = static_cast<int>(std::round(span));

    SeriesGeometry result;
    result.status = SeriesGeometryStatus::Ok;
    result.sliceCount = spanSteps + 1;
    result.missingSlice = result.sliceCount != static_cast<int>(slices.size());
    result.geometry.originMm = slices.front().imagePositionPatient;
    double lowest = positions.front();
    for (const SliceGeometry& slice : slices) {
        const double position = dot(slice.imagePositionPatient, normal);
        if (position <= lowest + kPositionToleranceMm) {
            lowest = position;
            result.geometry.originMm = slice.imagePositionPatient;
        }
    }
    result.geometry.spacingMm =
        core::Vec3{slices.front().columnSpacingMm, slices.front().rowSpacingMm, step};
    result.geometry.direction(0, 0) = row.x;
    result.geometry.direction(1, 0) = row.y;
    result.geometry.direction(2, 0) = row.z;
    result.geometry.direction(0, 1) = column.x;
    result.geometry.direction(1, 1) = column.y;
    result.geometry.direction(2, 1) = column.z;
    result.geometry.direction(0, 2) = normal.x;
    result.geometry.direction(1, 2) = normal.y;
    result.geometry.direction(2, 2) = normal.z;
    return result;
}

double storedValueToHu(double storedValue, const Rescale& rescale) {
    if (!isFinite(storedValue) || !isFinite(rescale.slope) || !isFinite(rescale.intercept)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return storedValue * rescale.slope + rescale.intercept;
}

HuWindow windowBoundsHu(const WindowLevel& windowLevel) {
    const double half = windowLevel.widthHu / 2.0;
    return HuWindow{windowLevel.centerHu - half, windowLevel.centerHu + half};
}

TransferSyntaxClass classifyTransferSyntax(std::string_view uid) {
    if (uid == "1.2.840.10008.1.2" || uid == "1.2.840.10008.1.2.1" ||
        uid == "1.2.840.10008.1.2.2") {
        return TransferSyntaxClass::Uncompressed;
    }
    if (uid == "1.2.840.10008.1.2.4.50" || uid == "1.2.840.10008.1.2.4.70" ||
        uid == "1.2.840.10008.1.2.4.80" || uid == "1.2.840.10008.1.2.4.90" ||
        uid == "1.2.840.10008.1.2.4.91" || uid == "1.2.840.10008.1.2.5") {
        return TransferSyntaxClass::Compressed;
    }
    return TransferSyntaxClass::Unrecognized;
}

} // namespace medforge::imaging
