#include "medforge/geometry/Surface.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace medforge::geometry {
namespace {

core::Vec3 add(const core::Vec3& left, const core::Vec3& right) {
    return {left.x + right.x, left.y + right.y, left.z + right.z};
}

core::Vec3 sub(const core::Vec3& left, const core::Vec3& right) {
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}

core::Vec3 mul(const core::Vec3& value, double scale) {
    return {value.x * scale, value.y * scale, value.z * scale};
}

core::Vec3 cross(const core::Vec3& left, const core::Vec3& right) {
    return {(left.y * right.z) - (left.z * right.y),
            (left.z * right.x) - (left.x * right.z),
            (left.x * right.y) - (left.y * right.x)};
}

double dot(const core::Vec3& left, const core::Vec3& right) {
    return (left.x * right.x) + (left.y * right.y) + (left.z * right.z);
}

double length(const core::Vec3& value) {
    return std::sqrt(dot(value, value));
}

core::Vec3 normalized(const core::Vec3& value) {
    const double magnitude = length(value);
    if (!(magnitude > 1e-12)) {
        return {0, 0, 1};
    }
    return mul(value, 1.0 / magnitude);
}

struct Vertex {
    core::Vec3 position;
    core::Vec3 normal;
};

double signedDistance(const core::Vec3& point, const core::Vec3& origin, const core::Vec3& normal) {
    return dot(sub(point, origin), normal);
}

Vertex interpolated(const Vertex& inside, const Vertex& outside, double insideDistance,
                    double outsideDistance) {
    const double span = insideDistance - outsideDistance;
    const double t = std::abs(span) < 1e-12 ? 0.0 : insideDistance / span;
    const double clamped = std::clamp(t, 0.0, 1.0);
    return {add(inside.position, mul(sub(outside.position, inside.position), clamped)),
            normalized(add(inside.normal, mul(sub(outside.normal, inside.normal), clamped)))};
}

} // namespace

MeshStatistics measureMesh(const TriangleMesh& mesh) {
    MeshStatistics stats;
    stats.vertexCount = static_cast<int>(mesh.positions.size());
    stats.triangleCount = static_cast<int>(mesh.indices.size() / 3);
    if (mesh.positions.empty()) {
        return stats;
    }
    stats.boundsMin = mesh.positions.front();
    stats.boundsMax = mesh.positions.front();
    core::Vec3 sum{};
    for (const core::Vec3& position : mesh.positions) {
        stats.boundsMin.x = std::min(stats.boundsMin.x, position.x);
        stats.boundsMin.y = std::min(stats.boundsMin.y, position.y);
        stats.boundsMin.z = std::min(stats.boundsMin.z, position.z);
        stats.boundsMax.x = std::max(stats.boundsMax.x, position.x);
        stats.boundsMax.y = std::max(stats.boundsMax.y, position.y);
        stats.boundsMax.z = std::max(stats.boundsMax.z, position.z);
        sum = add(sum, position);
    }
    const double count = static_cast<double>(mesh.positions.size());
    stats.centroid = mul(sum, 1.0 / count);
    double radiusSum = 0;
    for (const core::Vec3& position : mesh.positions) {
        radiusSum += length(sub(position, stats.centroid));
    }
    stats.meanRadiusMm = radiusSum / count;

    double signedVolume = 0;
    for (std::size_t index = 0; index + 2 < mesh.indices.size(); index += 3) {
        const std::uint32_t i0 = mesh.indices[index];
        const std::uint32_t i1 = mesh.indices[index + 1];
        const std::uint32_t i2 = mesh.indices[index + 2];
        if (i0 >= mesh.positions.size() || i1 >= mesh.positions.size() ||
            i2 >= mesh.positions.size()) {
            continue;
        }
        const core::Vec3& v0 = mesh.positions[i0];
        const core::Vec3& v1 = mesh.positions[i1];
        const core::Vec3& v2 = mesh.positions[i2];
        signedVolume += dot(v0, cross(v1, v2)) / 6.0;
    }
    stats.volumeMm3 = std::abs(signedVolume);
    return stats;
}

TriangleMesh clipHalfSpace(const TriangleMesh& mesh,
                           const core::Vec3& origin,
                           const core::Vec3& normal) {
    TriangleMesh clipped;
    const core::Vec3 planeNormal = normalized(normal);
    for (std::size_t index = 0; index + 2 < mesh.indices.size(); index += 3) {
        const std::uint32_t ids[3] = {mesh.indices[index], mesh.indices[index + 1],
                                      mesh.indices[index + 2]};
        if (ids[0] >= mesh.positions.size() || ids[1] >= mesh.positions.size() ||
            ids[2] >= mesh.positions.size()) {
            continue;
        }
        Vertex polygon[3];
        double distance[3];
        for (int corner = 0; corner < 3; ++corner) {
            polygon[corner].position = mesh.positions[ids[corner]];
            polygon[corner].normal = ids[corner] < mesh.normals.size()
                                         ? mesh.normals[ids[corner]]
                                         : core::Vec3{0, 0, 1};
            distance[corner] = signedDistance(polygon[corner].position, origin, planeNormal);
        }
        std::vector<Vertex> output;
        output.reserve(4);
        for (int corner = 0; corner < 3; ++corner) {
            const int previous = (corner + 2) % 3;
            const bool currentInside = distance[corner] <= 1e-8;
            const bool previousInside = distance[previous] <= 1e-8;
            if (currentInside) {
                if (!previousInside) {
                    output.push_back(interpolated(polygon[previous], polygon[corner],
                                                   distance[previous], distance[corner]));
                }
                output.push_back(polygon[corner]);
            } else if (previousInside) {
                output.push_back(interpolated(polygon[previous], polygon[corner],
                                               distance[previous], distance[corner]));
            }
        }
        if (output.size() < 3) {
            continue;
        }
        const auto pushVertex = [&clipped](const Vertex& vertex) {
            clipped.positions.push_back(vertex.position);
            clipped.normals.push_back(vertex.normal);
            clipped.indices.push_back(static_cast<std::uint32_t>(clipped.positions.size() - 1));
        };
        pushVertex(output[0]);
        pushVertex(output[1]);
        pushVertex(output[2]);
        if (output.size() >= 4) {
            pushVertex(output[0]);
            pushVertex(output[2]);
            pushVertex(output[3]);
        }
    }
    return clipped;
}

} // namespace medforge::geometry
