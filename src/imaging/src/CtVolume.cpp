#include "medforge/imaging/CtVolume.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace medforge::imaging {
namespace {

constexpr double kIndexTolerance = 1e-3;

core::Vec3 directionColumn(const core::Mat3& direction, int column) {
    return core::Vec3{direction(0, column), direction(1, column), direction(2, column)};
}

double dot(const core::Vec3& a, const core::Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

const char* geometryMessage(SeriesGeometryStatus status) {
    switch (status) {
    case SeriesGeometryStatus::Ok:
        return "";
    case SeriesGeometryStatus::Empty:
        return "No CT slices";
    case SeriesGeometryStatus::DuplicateSlice:
        return "Duplicate slice positions";
    case SeriesGeometryStatus::InconsistentOrientation:
        return "Inconsistent image orientation";
    case SeriesGeometryStatus::InconsistentSpacing:
        return "Inconsistent slice spacing";
    case SeriesGeometryStatus::InsufficientSlices:
        return "A CT series needs at least two slices";
    case SeriesGeometryStatus::NonPositiveSpacing:
        return "Pixel spacing must be positive";
    case SeriesGeometryStatus::InvalidOrientation:
        return "Invalid image orientation";
    }
    return "CT series was rejected";
}

VolumeBuild rejected(SeriesGeometryStatus status, const std::string& message) {
    VolumeBuild result;
    result.status = VolumeStatus::Rejected;
    result.geometryStatus = status;
    result.message = message;
    return result;
}

char patientLetter(const core::Vec3& direction) {
    const double ax = std::abs(direction.x);
    const double ay = std::abs(direction.y);
    const double az = std::abs(direction.z);
    if (ax >= ay && ax >= az) {
        return direction.x >= 0 ? 'L' : 'R';
    }
    if (ay >= ax && ay >= az) {
        return direction.y >= 0 ? 'P' : 'A';
    }
    return direction.z >= 0 ? 'S' : 'I';
}

char oppositeLetter(char letter) {
    switch (letter) {
    case 'L':
        return 'R';
    case 'R':
        return 'L';
    case 'A':
        return 'P';
    case 'P':
        return 'A';
    case 'S':
        return 'I';
    case 'I':
        return 'S';
    default:
        return '?';
    }
}

int clampIndex(int index, int size) {
    if (size <= 0) {
        return 0;
    }
    return std::clamp(index, 0, size - 1);
}

ViewLabels labelsForAxes(const core::Vec3& plusX, const core::Vec3& plusY) {
    ViewLabels labels;
    labels.right = patientLetter(plusX);
    labels.left = oppositeLetter(labels.right);
    labels.bottom = patientLetter(plusY);
    labels.top = oppositeLetter(labels.bottom);
    return labels;
}

} // namespace

float CtVolume::huAt(int i, int j, int k) const {
    if (i < 0 || j < 0 || k < 0 || i >= columns || j >= rows || k >= slices) {
        return std::numeric_limits<float>::quiet_NaN();
    }
    const std::size_t index = (static_cast<std::size_t>(k) * static_cast<std::size_t>(rows) +
                               static_cast<std::size_t>(j)) *
                                  static_cast<std::size_t>(columns) +
                              static_cast<std::size_t>(i);
    if (index >= hu.size()) {
        return std::numeric_limits<float>::quiet_NaN();
    }
    return hu[index];
}

VolumeBuild buildCtVolume(const std::vector<DicomCtInstance>& instances) {
    if (instances.empty()) {
        VolumeBuild result;
        result.status = VolumeStatus::Empty;
        result.geometryStatus = SeriesGeometryStatus::Empty;
        result.message = geometryMessage(SeriesGeometryStatus::Empty);
        return result;
    }
    for (const DicomCtInstance& instance : instances) {
        if (instance.status != DicomReadStatus::Ok) {
            return rejected(SeriesGeometryStatus::Empty, "A slice could not be read");
        }
        if (instance.seriesInstanceUid != instances.front().seriesInstanceUid) {
            return rejected(SeriesGeometryStatus::Empty, "Mixed series UIDs");
        }
    }

    std::vector<SliceGeometry> geometries;
    geometries.reserve(instances.size());
    for (const DicomCtInstance& instance : instances) {
        geometries.push_back(instance.geometry);
    }
    const SeriesGeometry geometry = assessSeriesGeometry(geometries);
    if (geometry.status != SeriesGeometryStatus::Ok) {
        return rejected(geometry.status, geometryMessage(geometry.status));
    }

    const int columns = instances.front().columns;
    const int rows = instances.front().rows;
    if (columns <= 0 || rows <= 0) {
        return rejected(SeriesGeometryStatus::Empty, "Inconsistent rows or columns");
    }
    for (const DicomCtInstance& instance : instances) {
        if (instance.columns != columns || instance.rows != rows ||
            static_cast<int>(instance.storedValues.size()) != columns * rows) {
            return rejected(SeriesGeometryStatus::Empty, "Inconsistent rows or columns");
        }
    }

    CtVolume volume;
    volume.seriesInstanceUid = instances.front().seriesInstanceUid;
    volume.geometry = geometry;
    volume.columns = columns;
    volume.rows = rows;
    volume.slices = geometry.sliceCount;
    volume.missingSlice = geometry.missingSlice;
    volume.hu.assign(static_cast<std::size_t>(columns) * static_cast<std::size_t>(rows) *
                         static_cast<std::size_t>(volume.slices),
                     std::numeric_limits<float>::quiet_NaN());

    const core::Vec3 normal = directionColumn(geometry.geometry.direction, 2);
    const double origin = dot(geometry.geometry.originMm, normal);
    const double step = geometry.geometry.spacingMm.z;
    for (const DicomCtInstance& instance : instances) {
        const double position = dot(instance.geometry.imagePositionPatient, normal);
        const double rawIndex = (position - origin) / step;
        const int k = static_cast<int>(std::llround(rawIndex));
        if (std::abs(rawIndex - static_cast<double>(k)) > kIndexTolerance || k < 0 ||
            k >= volume.slices) {
            return rejected(SeriesGeometryStatus::InconsistentSpacing,
                            "Slice does not lie on the series grid");
        }
        for (int j = 0; j < rows; ++j) {
            for (int i = 0; i < columns; ++i) {
                const std::int16_t stored =
                    instance.storedValues[static_cast<std::size_t>(j * columns + i)];
                const double hounsfield =
                    storedValueToHu(static_cast<double>(stored), instance.rescale);
                const float hu = static_cast<float>(hounsfield);
                const std::size_t index =
                    (static_cast<std::size_t>(k) * static_cast<std::size_t>(rows) +
                     static_cast<std::size_t>(j)) *
                        static_cast<std::size_t>(columns) +
                    static_cast<std::size_t>(i);
                volume.hu[index] = hu;
            }
        }
    }

    VolumeBuild result;
    result.status = VolumeStatus::Ok;
    result.geometryStatus = SeriesGeometryStatus::Ok;
    result.volume = std::move(volume);
    if (result.volume.missingSlice) {
        result.message = "Missing slice in the series";
    }
    return result;
}

MprImage extractPlane(const CtVolume& volume, MprPlane plane, int index) {
    MprImage image;
    if (volume.columns <= 0 || volume.rows <= 0 || volume.slices <= 0) {
        return image;
    }
    if (plane == MprPlane::Axial) {
        const int k = clampIndex(index, volume.slices);
        image.width = volume.columns;
        image.height = volume.rows;
        image.hu.resize(static_cast<std::size_t>(image.width * image.height));
        for (int j = 0; j < volume.rows; ++j) {
            for (int i = 0; i < volume.columns; ++i) {
                image.hu[static_cast<std::size_t>(j * image.width + i)] = volume.huAt(i, j, k);
            }
        }
        return image;
    }
    if (plane == MprPlane::Sagittal) {
        const int i = clampIndex(index, volume.columns);
        image.width = volume.rows;
        image.height = volume.slices;
        image.hu.resize(static_cast<std::size_t>(image.width * image.height));
        for (int k = 0; k < volume.slices; ++k) {
            for (int j = 0; j < volume.rows; ++j) {
                image.hu[static_cast<std::size_t>(k * image.width + j)] = volume.huAt(i, j, k);
            }
        }
        return image;
    }
    const int j = clampIndex(index, volume.rows);
    image.width = volume.columns;
    image.height = volume.slices;
    image.hu.resize(static_cast<std::size_t>(image.width * image.height));
    for (int k = 0; k < volume.slices; ++k) {
        for (int i = 0; i < volume.columns; ++i) {
            image.hu[static_cast<std::size_t>(k * image.width + i)] = volume.huAt(i, j, k);
        }
    }
    return image;
}

ViewLabels orientationLabels(const CtVolume& volume, MprPlane plane) {
    const core::VoxelGridGeometry& grid = volume.geometry.geometry;
    const core::Vec3 plusI = directionColumn(grid.direction, 0);
    const core::Vec3 plusJ = directionColumn(grid.direction, 1);
    const core::Vec3 plusK = directionColumn(grid.direction, 2);
    if (plane == MprPlane::Axial) {
        return labelsForAxes(plusI, plusJ);
    }
    if (plane == MprPlane::Sagittal) {
        return labelsForAxes(plusJ, plusK);
    }
    return labelsForAxes(plusI, plusK);
}

std::uint8_t huToDisplayGray(float hu, const WindowLevel& windowLevel) {
    if (!std::isfinite(hu) || !(windowLevel.widthHu > 0)) {
        return 0;
    }
    const double lower = windowLevel.centerHu - windowLevel.widthHu / 2.0;
    const double t = (static_cast<double>(hu) - lower) / windowLevel.widthHu;
    const double clamped = std::clamp(t, 0.0, 1.0);
    return static_cast<std::uint8_t>(std::lround(clamped * 255.0));
}

void scrollCrosshair(MprCrosshair& crosshair, MprPlane plane, int delta, const CtVolume& volume) {
    if (plane == MprPlane::Axial) {
        crosshair.k = clampIndex(crosshair.k + delta, volume.slices);
    } else if (plane == MprPlane::Sagittal) {
        crosshair.i = clampIndex(crosshair.i + delta, volume.columns);
    } else {
        crosshair.j = clampIndex(crosshair.j + delta, volume.rows);
    }
}

void moveCrosshairOnPlane(MprCrosshair& crosshair,
                          MprPlane plane,
                          int x,
                          int y,
                          const CtVolume& volume) {
    if (plane == MprPlane::Axial) {
        crosshair.i = clampIndex(x, volume.columns);
        crosshair.j = clampIndex(y, volume.rows);
    } else if (plane == MprPlane::Sagittal) {
        crosshair.j = clampIndex(x, volume.rows);
        crosshair.k = clampIndex(y, volume.slices);
    } else {
        crosshair.i = clampIndex(x, volume.columns);
        crosshair.k = clampIndex(y, volume.slices);
    }
}

PlanePixel crosshairPixel(const MprCrosshair& crosshair, MprPlane plane) {
    if (plane == MprPlane::Axial) {
        return PlanePixel{crosshair.i, crosshair.j};
    }
    if (plane == MprPlane::Sagittal) {
        return PlanePixel{crosshair.j, crosshair.k};
    }
    return PlanePixel{crosshair.i, crosshair.k};
}

core::CoordinateResult crosshairLps(const MprCrosshair& crosshair, const CtVolume& volume) {
    return core::voxelIndexToPatientLps(
        core::Vec3{static_cast<double>(crosshair.i), static_cast<double>(crosshair.j),
                   static_cast<double>(crosshair.k)},
        volume.geometry.geometry);
}

} // namespace medforge::imaging
