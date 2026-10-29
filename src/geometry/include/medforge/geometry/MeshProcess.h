#pragma once

#include "medforge/core/Cancellation.h"
#include "medforge/geometry/Surface.h"

#include <filesystem>
#include <string>

namespace medforge::geometry {

struct MeshTopology {
    int vertexCount = 0;
    int triangleCount = 0;
    core::Vec3 boundsMin{};
    core::Vec3 boundsMax{};
    double surfaceAreaMm2 = 0;
    double volumeMm3 = 0;
    bool volumeValid = false;
    int componentCount = 0;
    int openEdgeCount = 0;
    int nonManifoldEdgeCount = 0;
};

struct MeshEditResult {
    bool ok = false;
    bool cancelled = false;
    std::string message;
    TriangleMesh mesh;
};

MeshTopology inspectMesh(const TriangleMesh& mesh);

MeshEditResult smoothMesh(const TriangleMesh& mesh,
                          int iterations,
                          double factor,
                          const core::CancellationToken& token);

MeshEditResult decimateMesh(const TriangleMesh& mesh,
                            double keepRatio,
                            const core::CancellationToken& token);

MeshEditResult fillSmallHoles(const TriangleMesh& mesh,
                              int maxHoleEdges,
                              const core::CancellationToken& token);

bool writeBinaryStl(const TriangleMesh& mesh,
                    const std::filesystem::path& path,
                    std::string* error,
                    const char* headerLabel = nullptr);

bool readBinaryStl(const std::filesystem::path& path, TriangleMesh* mesh, std::string* error);

// STL has no unit field. Exports from this workstation are millimeters.
const char* stlUnitLabel();

} // namespace medforge::geometry
