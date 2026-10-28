#include "medforge/rendering/Isosurface.h"

#include "medforge/core/Coordinates.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>

#ifdef MEDFORGE_HAS_VTK
#include <vtkAlgorithm.h>
#include <vtkCallbackCommand.h>
#include <vtkCellArray.h>
#include <vtkCommand.h>
#include <vtkFlyingEdges3D.h>
#include <vtkImageData.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#endif

namespace medforge::rendering {
namespace {

SurfaceResult makeResult(SurfaceStatus status, std::string message) {
    SurfaceResult result;
    result.status = status;
    result.message = std::move(message);
    return result;
}

bool waitOrCancel(const core::CancellationToken& token, int delayBeforeMs) {
    int remaining = delayBeforeMs;
    while (remaining > 0) {
        if (token.isCancelled()) {
            return false;
        }
        const int slice = std::min(remaining, 20);
        std::this_thread::sleep_for(std::chrono::milliseconds(slice));
        remaining -= slice;
    }
    return !token.isCancelled();
}

#ifdef MEDFORGE_HAS_VTK

core::Vec3 sub(const core::Vec3& left, const core::Vec3& right) {
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}

core::Vec3 mul(const core::Vec3& value, double scale) {
    return {value.x * scale, value.y * scale, value.z * scale};
}

double dot(const core::Vec3& left, const core::Vec3& right) {
    return (left.x * right.x) + (left.y * right.y) + (left.z * right.z);
}

core::Vec3 normalized(const core::Vec3& value) {
    const double magnitude = std::sqrt(dot(value, value));
    if (!(magnitude > 1e-12)) {
        return {0, 0, 1};
    }
    return mul(value, 1.0 / magnitude);
}

float sampleGrid(const geometry::ScalarGrid& grid, int i, int j, int k) {
    if (i < 0 || j < 0 || k < 0 || i >= grid.columns || j >= grid.rows || k >= grid.slices) {
        return 0;
    }
    const std::size_t index = (static_cast<std::size_t>(k) * static_cast<std::size_t>(grid.rows) +
                               static_cast<std::size_t>(j)) *
                                  static_cast<std::size_t>(grid.columns) +
                              static_cast<std::size_t>(i);
    if (index >= grid.values.size() || !std::isfinite(grid.values[index])) {
        return 0;
    }
    return grid.values[index];
}

float sampleTrilinear(const geometry::ScalarGrid& grid, double x, double y, double z) {
    const double maxI = static_cast<double>(grid.columns - 1);
    const double maxJ = static_cast<double>(grid.rows - 1);
    const double maxK = static_cast<double>(grid.slices - 1);
    x = std::clamp(x, 0.0, maxI);
    y = std::clamp(y, 0.0, maxJ);
    z = std::clamp(z, 0.0, maxK);
    const int i0 = static_cast<int>(std::floor(x));
    const int j0 = static_cast<int>(std::floor(y));
    const int k0 = static_cast<int>(std::floor(z));
    const int i1 = std::min(i0 + 1, grid.columns - 1);
    const int j1 = std::min(j0 + 1, grid.rows - 1);
    const int k1 = std::min(k0 + 1, grid.slices - 1);
    const double tx = x - i0;
    const double ty = y - j0;
    const double tz = z - k0;
    const auto mix = [&](int i, int j, int k) {
        return static_cast<double>(sampleGrid(grid, i, j, k));
    };
    const double c00 = mix(i0, j0, k0) * (1.0 - tx) + mix(i1, j0, k0) * tx;
    const double c10 = mix(i0, j1, k0) * (1.0 - tx) + mix(i1, j1, k0) * tx;
    const double c01 = mix(i0, j0, k1) * (1.0 - tx) + mix(i1, j0, k1) * tx;
    const double c11 = mix(i0, j1, k1) * (1.0 - tx) + mix(i1, j1, k1) * tx;
    const double c0 = c00 * (1.0 - ty) + c10 * ty;
    const double c1 = c01 * (1.0 - ty) + c11 * ty;
    return static_cast<float>(c0 * (1.0 - tz) + c1 * tz);
}

core::Vec3 indexGradient(const geometry::ScalarGrid& grid, const core::Vec3& index) {
    constexpr double step = 0.5;
    const auto at = [&](double x, double y, double z) {
        return static_cast<double>(sampleTrilinear(grid, x, y, z));
    };
    return {(at(index.x + step, index.y, index.z) - at(index.x - step, index.y, index.z)) / (2.0 * step),
            (at(index.x, index.y + step, index.z) - at(index.x, index.y - step, index.z)) / (2.0 * step),
            (at(index.x, index.y, index.z + step) - at(index.x, index.y, index.z - step)) / (2.0 * step)};
}

bool invert(const core::Mat3& matrix, core::Mat3& inverse) {
    const double a = matrix(0, 0);
    const double b = matrix(0, 1);
    const double c = matrix(0, 2);
    const double d = matrix(1, 0);
    const double e = matrix(1, 1);
    const double f = matrix(1, 2);
    const double g = matrix(2, 0);
    const double h = matrix(2, 1);
    const double i = matrix(2, 2);
    const double det = (a * (e * i - f * h)) - (b * (d * i - f * g)) + (c * (d * h - e * g));
    if (std::abs(det) < 1e-12) {
        return false;
    }
    const double inv = 1.0 / det;
    inverse(0, 0) = (e * i - f * h) * inv;
    inverse(0, 1) = (c * h - b * i) * inv;
    inverse(0, 2) = (b * f - c * e) * inv;
    inverse(1, 0) = (f * g - d * i) * inv;
    inverse(1, 1) = (a * i - c * g) * inv;
    inverse(1, 2) = (c * d - a * f) * inv;
    inverse(2, 0) = (d * h - e * g) * inv;
    inverse(2, 1) = (b * g - a * h) * inv;
    inverse(2, 2) = (a * e - b * d) * inv;
    return true;
}

core::Vec3 multiply(const core::Mat3& matrix, const core::Vec3& vector) {
    return {matrix(0, 0) * vector.x + matrix(0, 1) * vector.y + matrix(0, 2) * vector.z,
            matrix(1, 0) * vector.x + matrix(1, 1) * vector.y + matrix(1, 2) * vector.z,
            matrix(2, 0) * vector.x + matrix(2, 1) * vector.y + matrix(2, 2) * vector.z};
}

core::Mat3 jacobian(const core::VoxelGridGeometry& geometry) {
    core::Mat3 matrix;
    const core::Vec3 spacing = geometry.spacingMm;
    for (int row = 0; row < 3; ++row) {
        matrix(row, 0) = geometry.direction(row, 0) * spacing.x;
        matrix(row, 1) = geometry.direction(row, 1) * spacing.y;
        matrix(row, 2) = geometry.direction(row, 2) * spacing.z;
    }
    return matrix;
}

core::Vec3 outwardNormal(const geometry::ScalarGrid& grid, const core::Vec3& index) {
    const core::Vec3 gradient = indexGradient(grid, index);
    core::Mat3 inverse{};
    if (!invert(jacobian(grid.geometry), inverse)) {
        return {0, 0, 1};
    }
    core::Mat3 inverseTranspose;
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            inverseTranspose(row, column) = inverse(column, row);
        }
    }
    return normalized(mul(multiply(inverseTranspose, gradient), -1.0));
}

void onProgress(vtkObject* caller, unsigned long, void* client, void*) {
    const auto* token = static_cast<const core::CancellationToken*>(client);
    if (token != nullptr && token->isCancelled()) {
        static_cast<vtkAlgorithm*>(caller)->AbortExecuteOn();
    }
}

#endif

} // namespace

SurfaceResult extractIsosurface(const geometry::ScalarGrid& grid,
                                double threshold,
                                const core::CancellationToken& token,
                                int delayBeforeMs) {
    if (!waitOrCancel(token, delayBeforeMs)) {
        return makeResult(SurfaceStatus::Cancelled, "Surface extraction cancelled");
    }
    if (grid.columns < 2 || grid.rows < 2 || grid.slices < 2 ||
        grid.values.size() < static_cast<std::size_t>(grid.columns) *
                                 static_cast<std::size_t>(grid.rows) *
                                 static_cast<std::size_t>(grid.slices)) {
        return makeResult(SurfaceStatus::Rejected, "A surface needs a complete voxel grid");
    }
#ifndef MEDFORGE_HAS_VTK
    (void)threshold;
    return makeResult(SurfaceStatus::Unavailable, "VTK is not available");
#else
    const auto started = std::chrono::steady_clock::now();
    vtkNew<vtkImageData> image;
    image->SetDimensions(grid.columns, grid.rows, grid.slices);
    image->SetOrigin(0, 0, 0);
    image->SetSpacing(1, 1, 1);
    image->AllocateScalars(VTK_FLOAT, 1);
    auto* scalars = static_cast<float*>(image->GetScalarPointer());
    for (std::size_t index = 0; index < grid.values.size(); ++index) {
        const float value = grid.values[index];
        scalars[index] = std::isfinite(value) ? value : static_cast<float>(threshold - 1.0);
    }

    vtkNew<vtkFlyingEdges3D> edges;
    edges->SetInputData(image);
    edges->SetValue(0, threshold);
    edges->SetComputeNormals(false);
    edges->SetComputeGradients(false);
    edges->SetComputeScalars(false);
    vtkNew<vtkCallbackCommand> callback;
    callback->SetCallback(onProgress);
    callback->SetClientData(const_cast<core::CancellationToken*>(&token));
    edges->AddObserver(vtkCommand::ProgressEvent, callback);
    edges->Update();

    SurfaceResult result;
    result.volumeBytes = grid.values.size() * sizeof(float);
    if (token.isCancelled() || edges->GetAbortExecute() != 0) {
        result.status = SurfaceStatus::Cancelled;
        result.message = "Surface extraction cancelled";
        return result;
    }

    vtkPolyData* poly = edges->GetOutput();
    vtkPoints* points = poly == nullptr ? nullptr : poly->GetPoints();
    if (points == nullptr || points->GetNumberOfPoints() == 0 || poly->GetNumberOfPolys() == 0) {
        result.status = SurfaceStatus::Empty;
        result.message = "No surface at this threshold";
        result.statistics = geometry::measureMesh(result.mesh);
        const auto elapsed = std::chrono::steady_clock::now() - started;
        result.elapsedMs = std::chrono::duration<double, std::milli>(elapsed).count();
        return result;
    }

    result.mesh.positions.resize(static_cast<std::size_t>(points->GetNumberOfPoints()));
    result.mesh.normals.resize(result.mesh.positions.size());
    for (vtkIdType index = 0; index < points->GetNumberOfPoints(); ++index) {
        double point[3] = {0, 0, 0};
        points->GetPoint(index, point);
        const core::Vec3 voxel{point[0], point[1], point[2]};
        const core::CoordinateResult physical = core::voxelIndexToPatientLps(voxel, grid.geometry);
        result.mesh.positions[static_cast<std::size_t>(index)] =
            physical.ok() ? physical.value : voxel;
        result.mesh.normals[static_cast<std::size_t>(index)] = outwardNormal(grid, voxel);
    }
    vtkCellArray* cells = poly->GetPolys();
    cells->InitTraversal();
    vtkIdType cellSize = 0;
    const vtkIdType* cellPoints = nullptr;
    while (cells->GetNextCell(cellSize, cellPoints) != 0) {
        if (cellSize != 3 || cellPoints == nullptr) {
            continue;
        }
        result.mesh.indices.push_back(static_cast<std::uint32_t>(cellPoints[0]));
        result.mesh.indices.push_back(static_cast<std::uint32_t>(cellPoints[1]));
        result.mesh.indices.push_back(static_cast<std::uint32_t>(cellPoints[2]));
    }
    result.statistics = geometry::measureMesh(result.mesh);
    result.meshBytes = (result.mesh.positions.size() * sizeof(core::Vec3) * 2) +
                       (result.mesh.indices.size() * sizeof(std::uint32_t));
    result.status = SurfaceStatus::Ok;
    result.message.clear();
    const auto elapsed = std::chrono::steady_clock::now() - started;
    result.elapsedMs = std::chrono::duration<double, std::milli>(elapsed).count();
    return result;
#endif
}

} // namespace medforge::rendering
