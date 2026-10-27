#include "medforge/core/Coordinates.h"

#include <cmath>

namespace medforge::core {
namespace {

constexpr double kSingularDeterminant = 1e-8;

bool isFinite(const Vec3& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool isFinite(const Mat3& value) {
    for (const double component : value.m) {
        if (!std::isfinite(component)) {
            return false;
        }
    }
    return true;
}

bool hasPositiveSpacing(const Vec3& spacing) {
    return spacing.x > 0 && spacing.y > 0 && spacing.z > 0;
}

double determinant(const Mat3& matrix) {
    return (matrix(0, 0) * (matrix(1, 1) * matrix(2, 2) - matrix(1, 2) * matrix(2, 1))) -
           (matrix(0, 1) * (matrix(1, 0) * matrix(2, 2) - matrix(1, 2) * matrix(2, 0))) +
           (matrix(0, 2) * (matrix(1, 0) * matrix(2, 1) - matrix(1, 1) * matrix(2, 0)));
}

Vec3 multiply(const Mat3& matrix, const Vec3& vector) {
    return Vec3{
        matrix(0, 0) * vector.x + matrix(0, 1) * vector.y + matrix(0, 2) * vector.z,
        matrix(1, 0) * vector.x + matrix(1, 1) * vector.y + matrix(1, 2) * vector.z,
        matrix(2, 0) * vector.x + matrix(2, 1) * vector.y + matrix(2, 2) * vector.z,
    };
}

Vec3 scaleBySpacing(const Vec3& index, const Vec3& spacing) {
    return Vec3{index.x * spacing.x, index.y * spacing.y, index.z * spacing.z};
}

bool invert(const Mat3& matrix, Mat3& inverseOut) {
    const double det = determinant(matrix);
    if (!std::isfinite(det) || std::abs(det) < kSingularDeterminant) {
        return false;
    }
    const double invDet = 1.0 / det;
    inverseOut(0, 0) = (matrix(1, 1) * matrix(2, 2) - matrix(1, 2) * matrix(2, 1)) * invDet;
    inverseOut(0, 1) = (matrix(0, 2) * matrix(2, 1) - matrix(0, 1) * matrix(2, 2)) * invDet;
    inverseOut(0, 2) = (matrix(0, 1) * matrix(1, 2) - matrix(0, 2) * matrix(1, 1)) * invDet;
    inverseOut(1, 0) = (matrix(1, 2) * matrix(2, 0) - matrix(1, 0) * matrix(2, 2)) * invDet;
    inverseOut(1, 1) = (matrix(0, 0) * matrix(2, 2) - matrix(0, 2) * matrix(2, 0)) * invDet;
    inverseOut(1, 2) = (matrix(0, 2) * matrix(1, 0) - matrix(0, 0) * matrix(1, 2)) * invDet;
    inverseOut(2, 0) = (matrix(1, 0) * matrix(2, 1) - matrix(1, 1) * matrix(2, 0)) * invDet;
    inverseOut(2, 1) = (matrix(0, 1) * matrix(2, 0) - matrix(0, 0) * matrix(2, 1)) * invDet;
    inverseOut(2, 2) = (matrix(0, 0) * matrix(1, 1) - matrix(0, 1) * matrix(1, 0)) * invDet;
    return true;
}

CoordinateStatus validateGeometry(const VoxelGridGeometry& geometry) {
    if (!isFinite(geometry.originMm) || !isFinite(geometry.spacingMm) ||
        !isFinite(geometry.direction)) {
        return CoordinateStatus::NonFiniteInput;
    }
    if (!hasPositiveSpacing(geometry.spacingMm)) {
        return CoordinateStatus::NonPositiveSpacing;
    }
    if (std::abs(determinant(geometry.direction)) < kSingularDeterminant) {
        return CoordinateStatus::SingularDirection;
    }
    return CoordinateStatus::Ok;
}

} // namespace

CoordinateResult voxelIndexToPatientLps(const Vec3& voxelIndex, const VoxelGridGeometry& geometry) {
    if (!isFinite(voxelIndex)) {
        return CoordinateResult{{}, CoordinateStatus::NonFiniteInput};
    }
    const CoordinateStatus status = validateGeometry(geometry);
    if (status != CoordinateStatus::Ok) {
        return CoordinateResult{{}, status};
    }
    const Vec3 rotated =
        multiply(geometry.direction, scaleBySpacing(voxelIndex, geometry.spacingMm));
    return CoordinateResult{Vec3{geometry.originMm.x + rotated.x,
                                 geometry.originMm.y + rotated.y,
                                 geometry.originMm.z + rotated.z},
                            CoordinateStatus::Ok};
}

CoordinateResult patientLpsToVoxelIndex(const Vec3& patientLpsMm,
                                        const VoxelGridGeometry& geometry) {
    if (!isFinite(patientLpsMm)) {
        return CoordinateResult{{}, CoordinateStatus::NonFiniteInput};
    }
    const CoordinateStatus status = validateGeometry(geometry);
    if (status != CoordinateStatus::Ok) {
        return CoordinateResult{{}, status};
    }
    Mat3 inverse;
    if (!invert(geometry.direction, inverse)) {
        return CoordinateResult{{}, CoordinateStatus::SingularDirection};
    }
    const Vec3 delta{patientLpsMm.x - geometry.originMm.x,
                     patientLpsMm.y - geometry.originMm.y,
                     patientLpsMm.z - geometry.originMm.z};
    const Vec3 scaled = multiply(inverse, delta);
    return CoordinateResult{Vec3{scaled.x / geometry.spacingMm.x,
                                 scaled.y / geometry.spacingMm.y,
                                 scaled.z / geometry.spacingMm.z},
                            CoordinateStatus::Ok};
}

} // namespace medforge::core
