#pragma once

#include <array>

namespace medforge::core {

struct Vec3 {
    double x = 0;
    double y = 0;
    double z = 0;
};

struct Mat3 {
    std::array<double, 9> m{};

    static Mat3 identity() {
        Mat3 result;
        result.m[0] = 1;
        result.m[4] = 1;
        result.m[8] = 1;
        return result;
    }

    double operator()(int row, int column) const {
        return m[(static_cast<std::size_t>(row) * 3U) + static_cast<std::size_t>(column)];
    }

    double& operator()(int row, int column) {
        return m[(static_cast<std::size_t>(row) * 3U) + static_cast<std::size_t>(column)];
    }
};

struct VoxelGridGeometry {
    Vec3 originMm{};
    Vec3 spacingMm{1, 1, 1};
    Mat3 direction = Mat3::identity();
};

enum class CoordinateStatus { Ok, NonFiniteInput, NonPositiveSpacing, SingularDirection };

struct CoordinateResult {
    Vec3 value{};
    CoordinateStatus status = CoordinateStatus::Ok;

    bool ok() const { return status == CoordinateStatus::Ok; }
};

// patient_lps_mm = origin_mm + direction * (voxel_index o spacing_mm)
// Direction columns are the LPS direction cosines of voxel axes i, j, and k.
// Indices may be fractional. Spacing is millimeters per index step and must be positive.
CoordinateResult voxelIndexToPatientLps(const Vec3& voxelIndex, const VoxelGridGeometry& geometry);
CoordinateResult patientLpsToVoxelIndex(const Vec3& patientLpsMm,
                                        const VoxelGridGeometry& geometry);

} // namespace medforge::core
