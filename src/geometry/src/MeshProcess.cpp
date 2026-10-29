#include "medforge/geometry/MeshProcess.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

namespace medforge::geometry {
namespace {

constexpr double kEpsilon = 1e-12;

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
    return {(left.y * right.z) - (left.z * right.y), (left.z * right.x) - (left.x * right.z),
            (left.x * right.y) - (left.y * right.x)};
}

double dot(const core::Vec3& left, const core::Vec3& right) {
    return (left.x * right.x) + (left.y * right.y) + (left.z * right.z);
}

double length(const core::Vec3& value) {
    return std::sqrt(dot(value, value));
}

using EdgeKey = std::pair<std::uint32_t, std::uint32_t>;

EdgeKey undirected(std::uint32_t a, std::uint32_t b) {
    return a < b ? EdgeKey{a, b} : EdgeKey{b, a};
}

bool usable(const TriangleMesh& mesh, std::uint32_t index) {
    return index < mesh.positions.size();
}

int findRoot(std::vector<int>& parent, int index) {
    while (parent[static_cast<std::size_t>(index)] != index) {
        parent[static_cast<std::size_t>(index)] =
            parent[static_cast<std::size_t>(parent[static_cast<std::size_t>(index)])];
        index = parent[static_cast<std::size_t>(index)];
    }
    return index;
}

void unite(std::vector<int>& parent, int left, int right) {
    const int a = findRoot(parent, left);
    const int b = findRoot(parent, right);
    if (a != b) {
        parent[static_cast<std::size_t>(b)] = a;
    }
}

int triangleCountOf(const TriangleMesh& mesh) {
    return static_cast<int>(mesh.indices.size() / 3);
}

MeshEditResult cancelledCopy(const TriangleMesh& mesh) {
    MeshEditResult result;
    result.cancelled = true;
    result.message = "Mesh edit cancelled";
    result.mesh = mesh;
    return result;
}

void recomputeNormals(TriangleMesh* mesh) {
    mesh->normals.assign(mesh->positions.size(), {});
    for (std::size_t index = 0; index + 2 < mesh->indices.size(); index += 3) {
        const std::uint32_t ids[3] = {mesh->indices[index], mesh->indices[index + 1],
                                      mesh->indices[index + 2]};
        if (!usable(*mesh, ids[0]) || !usable(*mesh, ids[1]) || !usable(*mesh, ids[2]) ||
            ids[0] == ids[1] || ids[1] == ids[2] || ids[2] == ids[0]) {
            continue;
        }
        const core::Vec3 normal = cross(sub(mesh->positions[ids[1]], mesh->positions[ids[0]]),
                                         sub(mesh->positions[ids[2]], mesh->positions[ids[0]]));
        for (std::uint32_t id : ids) {
            mesh->normals[id] = add(mesh->normals[id], normal);
        }
    }
    for (core::Vec3& normal : mesh->normals) {
        const double magnitude = length(normal);
        normal = magnitude > kEpsilon ? mul(normal, 1.0 / magnitude) : core::Vec3{0, 0, 1};
    }
}

struct EdgeMaps {
    std::map<EdgeKey, int> uses;
    std::map<std::uint32_t, std::uint32_t> holeNext;
};

EdgeMaps collectEdges(const TriangleMesh& mesh) {
    EdgeMaps maps;
    std::map<EdgeKey, EdgeKey> directed;
    for (std::size_t index = 0; index + 2 < mesh.indices.size(); index += 3) {
        const std::uint32_t ids[3] = {mesh.indices[index], mesh.indices[index + 1],
                                      mesh.indices[index + 2]};
        if (!usable(mesh, ids[0]) || !usable(mesh, ids[1]) || !usable(mesh, ids[2])) {
            continue;
        }
        for (int corner = 0; corner < 3; ++corner) {
            const std::uint32_t a = ids[corner];
            const std::uint32_t b = ids[(corner + 1) % 3];
            if (a == b) {
                continue;
            }
            const EdgeKey key = undirected(a, b);
            maps.uses[key] += 1;
            directed[key] = EdgeKey{a, b};
        }
    }
    for (const auto& [key, count] : maps.uses) {
        if (count != 1) {
            continue;
        }
        const EdgeKey edge = directed[key];
        maps.holeNext[edge.second] = edge.first;
    }
    return maps;
}

} // namespace

const char* stlUnitLabel() {
    return "millimeters";
}

MeshTopology inspectMesh(const TriangleMesh& mesh) {
    MeshTopology topology;
    topology.vertexCount = static_cast<int>(mesh.positions.size());
    topology.triangleCount = triangleCountOf(mesh);
    if (mesh.positions.empty()) {
        return topology;
    }
    topology.boundsMin = mesh.positions.front();
    topology.boundsMax = mesh.positions.front();
    for (const core::Vec3& position : mesh.positions) {
        topology.boundsMin.x = std::min(topology.boundsMin.x, position.x);
        topology.boundsMin.y = std::min(topology.boundsMin.y, position.y);
        topology.boundsMin.z = std::min(topology.boundsMin.z, position.z);
        topology.boundsMax.x = std::max(topology.boundsMax.x, position.x);
        topology.boundsMax.y = std::max(topology.boundsMax.y, position.y);
        topology.boundsMax.z = std::max(topology.boundsMax.z, position.z);
    }
    const EdgeMaps edges = collectEdges(mesh);
    for (const auto& [key, count] : edges.uses) {
        (void)key;
        if (count == 1) {
            ++topology.openEdgeCount;
        } else if (count > 2) {
            ++topology.nonManifoldEdgeCount;
        }
    }
    std::vector<int> parent(mesh.positions.size());
    for (int index = 0; index < static_cast<int>(parent.size()); ++index) {
        parent[static_cast<std::size_t>(index)] = index;
    }
    double area = 0;
    double signedVolume = 0;
    std::set<int> components;
    for (std::size_t index = 0; index + 2 < mesh.indices.size(); index += 3) {
        const std::uint32_t ids[3] = {mesh.indices[index], mesh.indices[index + 1],
                                      mesh.indices[index + 2]};
        if (!usable(mesh, ids[0]) || !usable(mesh, ids[1]) || !usable(mesh, ids[2]) ||
            ids[0] == ids[1] || ids[1] == ids[2] || ids[2] == ids[0]) {
            continue;
        }
        unite(parent, static_cast<int>(ids[0]), static_cast<int>(ids[1]));
        unite(parent, static_cast<int>(ids[1]), static_cast<int>(ids[2]));
        const core::Vec3& v0 = mesh.positions[ids[0]];
        const core::Vec3& v1 = mesh.positions[ids[1]];
        const core::Vec3& v2 = mesh.positions[ids[2]];
        area += 0.5 * length(cross(sub(v1, v0), sub(v2, v0)));
        signedVolume += dot(v0, cross(v1, v2)) / 6.0;
    }
    for (std::size_t index = 0; index + 2 < mesh.indices.size(); index += 3) {
        const std::uint32_t id = mesh.indices[index];
        if (usable(mesh, id)) {
            components.insert(findRoot(parent, static_cast<int>(id)));
        }
    }
    topology.componentCount = static_cast<int>(components.size());
    topology.surfaceAreaMm2 = area;
    topology.volumeValid = topology.triangleCount > 0 && topology.openEdgeCount == 0 &&
                           topology.nonManifoldEdgeCount == 0;
    topology.volumeMm3 = topology.volumeValid ? std::abs(signedVolume) : 0;
    return topology;
}

MeshEditResult smoothMesh(const TriangleMesh& mesh,
                          int iterations,
                          double factor,
                          const core::CancellationToken& token) {
    if (token.isCancelled()) {
        return cancelledCopy(mesh);
    }
    if (mesh.positions.empty() || mesh.indices.size() < 3) {
        MeshEditResult result;
        result.message = "Mesh has no triangles";
        result.mesh = mesh;
        return result;
    }
    iterations = std::clamp(iterations, 1, 50);
    factor = std::clamp(factor, 0.0, 1.0);
    const EdgeMaps edges = collectEdges(mesh);
    std::set<std::uint32_t> pinned;
    for (const auto& [from, to] : edges.holeNext) {
        pinned.insert(from);
        pinned.insert(to);
    }
    std::vector<std::set<std::uint32_t>> neighbors(mesh.positions.size());
    for (const auto& [key, count] : edges.uses) {
        (void)count;
        neighbors[key.first].insert(key.second);
        neighbors[key.second].insert(key.first);
    }
    TriangleMesh smoothed = mesh;
    for (int iteration = 0; iteration < iterations; ++iteration) {
        if (token.isCancelled()) {
            return cancelledCopy(mesh);
        }
        std::vector<core::Vec3> next = smoothed.positions;
        for (std::size_t index = 0; index < smoothed.positions.size(); ++index) {
            if (pinned.count(static_cast<std::uint32_t>(index)) != 0 ||
                neighbors[index].empty()) {
                continue;
            }
            core::Vec3 sum{};
            for (std::uint32_t neighbor : neighbors[index]) {
                sum = add(sum, smoothed.positions[neighbor]);
            }
            const core::Vec3 average = mul(sum, 1.0 / static_cast<double>(neighbors[index].size()));
            next[index] = add(smoothed.positions[index],
                              mul(sub(average, smoothed.positions[index]), factor));
        }
        smoothed.positions = std::move(next);
    }
    recomputeNormals(&smoothed);
    MeshEditResult result;
    result.ok = true;
    result.mesh = std::move(smoothed);
    result.message = "Smoothed";
    return result;
}

MeshEditResult decimateMesh(const TriangleMesh& mesh,
                            double keepRatio,
                            const core::CancellationToken& token) {
    if (token.isCancelled()) {
        return cancelledCopy(mesh);
    }
    if (!(keepRatio > 0.0) || keepRatio > 1.0) {
        MeshEditResult result;
        result.message = "Decimation keep ratio must be between 0 and 1";
        result.mesh = mesh;
        return result;
    }
    const int original = triangleCountOf(mesh);
    const int target = std::max(4, static_cast<int>(std::floor(original * keepRatio)));
    TriangleMesh work = mesh;
    if (original <= target) {
        MeshEditResult result;
        result.ok = true;
        result.mesh = std::move(work);
        result.message = "Mesh is already at the target size";
        return result;
    }
    int guard = original;
    while (triangleCountOf(work) > target && guard-- > 0) {
        if (token.isCancelled()) {
            return cancelledCopy(mesh);
        }
        double best = 0;
        EdgeKey edge{0, 0};
        bool found = false;
        for (std::size_t index = 0; index + 2 < work.indices.size(); index += 3) {
            const std::uint32_t ids[3] = {work.indices[index], work.indices[index + 1],
                                          work.indices[index + 2]};
            if (!usable(work, ids[0]) || !usable(work, ids[1]) || !usable(work, ids[2])) {
                continue;
            }
            for (int corner = 0; corner < 3; ++corner) {
                const std::uint32_t a = ids[corner];
                const std::uint32_t b = ids[(corner + 1) % 3];
                if (a == b) {
                    continue;
                }
                const double span = length(sub(work.positions[a], work.positions[b]));
                if (!found || span < best) {
                    found = true;
                    best = span;
                    edge = EdgeKey{a, b};
                }
            }
        }
        if (!found) {
            break;
        }
        const std::vector<core::Vec3> savedPositions = work.positions;
        const std::vector<std::uint32_t> savedIndices = work.indices;
        const std::uint32_t keep = edge.first;
        const std::uint32_t drop = edge.second;
        work.positions[keep] = mul(add(work.positions[keep], work.positions[drop]), 0.5);
        for (std::uint32_t& index : work.indices) {
            if (index == drop) {
                index = keep;
            }
        }
        std::vector<std::uint32_t> kept;
        kept.reserve(work.indices.size());
        for (std::size_t index = 0; index + 2 < work.indices.size(); index += 3) {
            const std::uint32_t a = work.indices[index];
            const std::uint32_t b = work.indices[index + 1];
            const std::uint32_t c = work.indices[index + 2];
            if (a != b && b != c && c != a) {
                kept.push_back(a);
                kept.push_back(b);
                kept.push_back(c);
            }
        }
        work.indices = std::move(kept);
        if (triangleCountOf(work) >= original && work.indices.size() >= savedIndices.size()) {
            work.positions = savedPositions;
            work.indices = savedIndices;
            break;
        }
        if (triangleCountOf(work) >= static_cast<int>(savedIndices.size() / 3)) {
            work.positions = savedPositions;
            work.indices = savedIndices;
            break;
        }
    }
    recomputeNormals(&work);
    MeshEditResult result;
    result.ok = triangleCountOf(work) < original && triangleCountOf(work) > 0;
    result.mesh = std::move(work);
    result.message = result.ok ? "Decimated" : "Decimation could not remove triangles";
    if (!result.ok) {
        result.mesh = mesh;
    }
    return result;
}

MeshEditResult fillSmallHoles(const TriangleMesh& mesh,
                              int maxHoleEdges,
                              const core::CancellationToken& token) {
    if (token.isCancelled()) {
        return cancelledCopy(mesh);
    }
    if (maxHoleEdges < 3) {
        MeshEditResult result;
        result.message = "Hole edge limit must be at least 3";
        result.mesh = mesh;
        return result;
    }
    const EdgeMaps edges = collectEdges(mesh);
    if (edges.holeNext.empty()) {
        MeshEditResult result;
        result.message = "No open holes";
        result.mesh = mesh;
        return result;
    }
    std::set<std::uint32_t> pending;
    for (const auto& [from, to] : edges.holeNext) {
        (void)to;
        pending.insert(from);
    }
    std::vector<std::vector<std::uint32_t>> loops;
    while (!pending.empty()) {
        if (token.isCancelled()) {
            return cancelledCopy(mesh);
        }
        const std::uint32_t start = *pending.begin();
        std::vector<std::uint32_t> loop;
        std::uint32_t cursor = start;
        bool closed = false;
        for (int step = 0; step < static_cast<int>(edges.holeNext.size()) + 1; ++step) {
            loop.push_back(cursor);
            pending.erase(cursor);
            const auto next = edges.holeNext.find(cursor);
            if (next == edges.holeNext.end()) {
                break;
            }
            cursor = next->second;
            if (cursor == start) {
                closed = true;
                break;
            }
            if (pending.count(cursor) == 0) {
                break;
            }
        }
        if (!closed) {
            MeshEditResult result;
            result.message = "Open boundary is not a closed hole. The mesh was not changed.";
            result.mesh = mesh;
            return result;
        }
        loops.push_back(std::move(loop));
    }
    for (const std::vector<std::uint32_t>& loop : loops) {
        if (static_cast<int>(loop.size()) > maxHoleEdges) {
            MeshEditResult result;
            result.message = "Hole has " + std::to_string(loop.size()) +
                             " edges; the limit is " + std::to_string(maxHoleEdges) +
                             ". The mesh was not changed.";
            result.mesh = mesh;
            return result;
        }
    }
    TriangleMesh filled = mesh;
    int added = 0;
    for (const std::vector<std::uint32_t>& loop : loops) {
        if (loop.size() < 3) {
            continue;
        }
        for (std::size_t index = 1; index + 1 < loop.size(); ++index) {
            filled.indices.push_back(loop[0]);
            filled.indices.push_back(loop[index]);
            filled.indices.push_back(loop[index + 1]);
            ++added;
        }
    }
    if (added == 0) {
        MeshEditResult result;
        result.message = "No open holes";
        result.mesh = mesh;
        return result;
    }
    recomputeNormals(&filled);
    MeshEditResult result;
    result.ok = true;
    result.mesh = std::move(filled);
    result.message = "Filled " + std::to_string(added) + " triangles";
    return result;
}

bool writeBinaryStl(const TriangleMesh& mesh,
                    const std::filesystem::path& path,
                    std::string* error,
                    const char* headerLabel) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        if (error != nullptr) {
            *error = "Could not write the STL";
        }
        return false;
    }
    char header[80] = {};
    const char fallback[] = "MedForge binary STL, units millimeters";
    const char* label = headerLabel != nullptr && headerLabel[0] != '\0' ? headerLabel : fallback;
    const std::size_t labelLength = std::min<std::size_t>(std::strlen(label), 79);
    std::memcpy(header, label, labelLength);
    out.write(header, 80);
    const auto count = static_cast<std::uint32_t>(mesh.indices.size() / 3);
    out.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (std::uint32_t triangle = 0; triangle < count; ++triangle) {
        const std::uint32_t ids[3] = {mesh.indices[triangle * 3], mesh.indices[triangle * 3 + 1],
                                      mesh.indices[triangle * 3 + 2]};
        core::Vec3 normal{};
        if (usable(mesh, ids[0]) && usable(mesh, ids[1]) && usable(mesh, ids[2])) {
            normal = cross(sub(mesh.positions[ids[1]], mesh.positions[ids[0]]),
                           sub(mesh.positions[ids[2]], mesh.positions[ids[0]]));
            const double magnitude = length(normal);
            if (magnitude > kEpsilon) {
                normal = mul(normal, 1.0 / magnitude);
            }
        }
        float values[12] = {static_cast<float>(normal.x), static_cast<float>(normal.y),
                            static_cast<float>(normal.z)};
        for (int corner = 0; corner < 3; ++corner) {
            const core::Vec3 position =
                usable(mesh, ids[corner]) ? mesh.positions[ids[corner]] : core::Vec3{};
            values[3 + corner * 3] = static_cast<float>(position.x);
            values[4 + corner * 3] = static_cast<float>(position.y);
            values[5 + corner * 3] = static_cast<float>(position.z);
        }
        out.write(reinterpret_cast<const char*>(values), sizeof(values));
        const std::uint16_t attribute = 0;
        out.write(reinterpret_cast<const char*>(&attribute), sizeof(attribute));
    }
    if (!out) {
        if (error != nullptr) {
            *error = "Could not write the STL";
        }
        return false;
    }
    return true;
}

bool readBinaryStl(const std::filesystem::path& path, TriangleMesh* mesh, std::string* error) {
    if (mesh == nullptr) {
        if (error != nullptr) {
            *error = "No mesh was provided";
        }
        return false;
    }
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        if (error != nullptr) {
            *error = "Could not read the STL";
        }
        return false;
    }
    const auto bytes = std::filesystem::file_size(path);
    char header[80] = {};
    in.read(header, 80);
    std::uint32_t count = 0;
    in.read(reinterpret_cast<char*>(&count), sizeof(count));
    const std::uintmax_t expected = static_cast<std::uintmax_t>(84) +
                                    static_cast<std::uintmax_t>(count) * 50u;
    if (!in || bytes != expected || count > 5000000u) {
        if (error != nullptr) {
            *error = "STL file is truncated or not a binary STL";
        }
        return false;
    }
    TriangleMesh loaded;
    std::map<std::tuple<float, float, float>, std::uint32_t> weld;
    const auto addVertex = [&loaded, &weld](float x, float y, float z) {
        const auto key = std::tuple<float, float, float>{x, y, z};
        const auto found = weld.find(key);
        if (found != weld.end()) {
            loaded.indices.push_back(found->second);
            return;
        }
        const auto index = static_cast<std::uint32_t>(loaded.positions.size());
        loaded.positions.push_back(
            {static_cast<double>(x), static_cast<double>(y), static_cast<double>(z)});
        weld.emplace(key, index);
        loaded.indices.push_back(index);
    };
    for (std::uint32_t triangle = 0; triangle < count; ++triangle) {
        float values[12] = {};
        in.read(reinterpret_cast<char*>(values), sizeof(values));
        std::uint16_t attribute = 0;
        in.read(reinterpret_cast<char*>(&attribute), sizeof(attribute));
        if (!in) {
            if (error != nullptr) {
                *error = "STL file is truncated or not a binary STL";
            }
            return false;
        }
        for (int corner = 0; corner < 3; ++corner) {
            addVertex(values[3 + corner * 3], values[4 + corner * 3], values[5 + corner * 3]);
        }
    }
    recomputeNormals(&loaded);
    *mesh = std::move(loaded);
    return true;
}

} // namespace medforge::geometry
