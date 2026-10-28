#pragma once

#include "medforge/core/Cancellation.h"
#include "medforge/geometry/Surface.h"

#include <cstdint>
#include <string>

namespace medforge::rendering {

enum class SurfaceStatus { Ok, Empty, Cancelled, Unavailable, Rejected };

struct SurfaceResult {
    SurfaceStatus status = SurfaceStatus::Rejected;
    std::string message;
    geometry::TriangleMesh mesh;
    geometry::MeshStatistics statistics;
    double elapsedMs = 0;
    std::uint64_t volumeBytes = 0;
    std::uint64_t meshBytes = 0;
};

// Flying-edges isosurface. Vertices are patient LPS millimeters.
// delayBeforeMs polls cancellation before the extraction starts.
SurfaceResult extractIsosurface(const geometry::ScalarGrid& grid,
                                double threshold,
                                const core::CancellationToken& token,
                                int delayBeforeMs = 0);

} // namespace medforge::rendering
