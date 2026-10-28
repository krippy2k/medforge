#pragma once

#include "medforge/core/Coordinates.h"

#include <cstdint>
#include <vector>

namespace medforge::geometry {

struct TriangleMesh {
    std::vector<core::Vec3> positions;
    std::vector<core::Vec3> normals;
    std::vector<std::uint32_t> indices;
};

struct MeshStatistics {
    int vertexCount = 0;
    int triangleCount = 0;
    core::Vec3 boundsMin{};
    core::Vec3 boundsMax{};
    core::Vec3 centroid{};
    double volumeMm3 = 0;
    double meanRadiusMm = 0;
};

struct ScalarGrid {
    int columns = 0;
    int rows = 0;
    int slices = 0;
    std::vector<float> values;
    core::VoxelGridGeometry geometry{};
};

MeshStatistics measureMesh(const TriangleMesh& mesh);

// Keeps the half-space where dot(point - origin, normal) <= 0.
TriangleMesh clipHalfSpace(const TriangleMesh& mesh,
                           const core::Vec3& origin,
                           const core::Vec3& normal);

} // namespace medforge::geometry
