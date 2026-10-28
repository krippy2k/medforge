#include "medforge/imaging/LabelMap.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <queue>
#include <thread>

namespace medforge::imaging {
namespace {

std::size_t voxelIndex(int columns, int rows, int i, int j, int k) {
    return (static_cast<std::size_t>(k) * static_cast<std::size_t>(rows) + static_cast<std::size_t>(j)) *
               static_cast<std::size_t>(columns) +
           static_cast<std::size_t>(i);
}

bool inside(const LabelMap& map, int i, int j, int k) {
    return i >= 0 && j >= 0 && k >= 0 && i < map.columns && j < map.rows && k < map.slices;
}

bool insideBounds(const IndexBounds& bounds, int i, int j, int k) {
    if (!bounds.limited) {
        return true;
    }
    return i >= bounds.iMin && i <= bounds.iMax && j >= bounds.jMin && j <= bounds.jMax &&
           k >= bounds.kMin && k <= bounds.kMax;
}

bool sameGrid(const CtVolume& volume, const LabelMap& map) {
    return volume.columns == map.columns && volume.rows == map.rows && volume.slices == map.slices &&
           volume.hu.size() == map.voxels.size();
}

bool inRange(float hu, float minHu, float maxHu) {
    return std::isfinite(hu) && hu >= minHu && hu <= maxHu;
}

bool waitOrCancel(const core::CancellationToken& token, int delayBeforeMs) {
    if (token.isCancelled()) {
        return true;
    }
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(delayBeforeMs);
    while (std::chrono::steady_clock::now() < deadline) {
        if (token.isCancelled()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return token.isCancelled();
}

void pushChange(LabelEdit* edit, std::uint32_t index, std::uint8_t before, std::uint8_t after) {
    if (before == after) {
        return;
    }
    edit->changes.push_back(VoxelChange{index, before, after});
}

bool planeCenter(const LabelMap& map, const BrushStamp& stamp, int* i, int* j, int* k) {
    if (stamp.plane == MprPlane::Axial) {
        *i = stamp.x;
        *j = stamp.y;
        *k = stamp.slice;
    } else if (stamp.plane == MprPlane::Sagittal) {
        *i = stamp.slice;
        *j = stamp.x;
        *k = stamp.y;
    } else {
        *i = stamp.x;
        *j = stamp.slice;
        *k = stamp.y;
    }
    return inside(map, *i, *j, *k);
}

void planeSpacing(const LabelMap& map, MprPlane plane, double* xMm, double* yMm) {
    const core::Vec3& spacing = map.geometry.spacingMm;
    if (plane == MprPlane::Axial) {
        *xMm = spacing.x;
        *yMm = spacing.y;
    } else if (plane == MprPlane::Sagittal) {
        *xMm = spacing.y;
        *yMm = spacing.z;
    } else {
        *xMm = spacing.x;
        *yMm = spacing.z;
    }
    if (!(*xMm > 0)) {
        *xMm = 1;
    }
    if (!(*yMm > 0)) {
        *yMm = 1;
    }
}

template <typename T>
bool writePod(std::ostream& out, const T& value) {
    out.write(reinterpret_cast<const char*>(&value), sizeof(T));
    return static_cast<bool>(out);
}

template <typename T>
bool readPod(std::istream& in, T* value) {
    in.read(reinterpret_cast<char*>(value), sizeof(T));
    return static_cast<bool>(in);
}

} // namespace

std::size_t LabelMap::voxelCount() const {
    return voxels.size();
}

std::uint8_t LabelMap::at(int i, int j, int k) const {
    if (!inside(*this, i, j, k)) {
        return 0;
    }
    return voxels[voxelIndex(columns, rows, i, j, k)];
}

LabelMap emptyLabelMap(const CtVolume& volume) {
    LabelMap map;
    map.seriesInstanceUid = volume.seriesInstanceUid;
    map.geometry = volume.geometry.geometry;
    map.columns = std::max(volume.columns, 0);
    map.rows = std::max(volume.rows, 0);
    map.slices = std::max(volume.slices, 0);
    map.voxels.assign(static_cast<std::size_t>(map.columns) * static_cast<std::size_t>(map.rows) *
                          static_cast<std::size_t>(map.slices),
                      0);
    return map;
}

std::size_t countLabel(const LabelMap& map, std::uint8_t label) {
    return static_cast<std::size_t>(std::count(map.voxels.begin(), map.voxels.end(), label));
}

std::size_t countInHuRange(const CtVolume& volume, float minHu, float maxHu) {
    std::size_t count = 0;
    for (float hu : volume.hu) {
        if (inRange(hu, minHu, maxHu)) {
            ++count;
        }
    }
    return count;
}

LabelEdit thresholdEdit(const CtVolume& volume,
                        const LabelMap& map,
                        std::uint8_t label,
                        float minHu,
                        float maxHu) {
    LabelEdit edit;
    if (!sameGrid(volume, map) || !(minHu <= maxHu)) {
        return edit;
    }
    for (int k = 0; k < map.slices; ++k) {
        for (int j = 0; j < map.rows; ++j) {
            for (int i = 0; i < map.columns; ++i) {
                const float hu = volume.huAt(i, j, k);
                if (!inRange(hu, minHu, maxHu)) {
                    continue;
                }
                const std::size_t index = voxelIndex(map.columns, map.rows, i, j, k);
                pushChange(&edit, static_cast<std::uint32_t>(index), map.voxels[index], label);
            }
        }
    }
    return edit;
}

SegmentResult regionGrowEdit(const CtVolume& volume,
                             const LabelMap& map,
                             int seedI,
                             int seedJ,
                             int seedK,
                             std::uint8_t label,
                             float minHu,
                             float maxHu,
                             Connectivity connectivity,
                             const IndexBounds& bounds,
                             const core::CancellationToken& token,
                             int delayBeforeMs) {
    SegmentResult result;
    if (waitOrCancel(token, std::max(delayBeforeMs, 0))) {
        result.cancelled = true;
        result.message = "Segmentation cancelled";
        return result;
    }
    if (!sameGrid(volume, map) || !(minHu <= maxHu)) {
        result.message = "Label map does not match the volume";
        return result;
    }
    if (!inside(map, seedI, seedJ, seedK)) {
        result.message = "Seed is outside the volume";
        return result;
    }
    if (!insideBounds(bounds, seedI, seedJ, seedK)) {
        result.message = "Seed is outside the grow bounds";
        return result;
    }
    const float seedHu = volume.huAt(seedI, seedJ, seedK);
    if (!inRange(seedHu, minHu, maxHu)) {
        result.message = "Seed is outside the HU range";
        return result;
    }
    const std::size_t seedIndex = voxelIndex(map.columns, map.rows, seedI, seedJ, seedK);
    if (label != 0 && map.voxels[seedIndex] != 0 && map.voxels[seedIndex] != label) {
        result.message = "Seed is already another label";
        return result;
    }

    std::vector<std::uint8_t> visited(map.voxels.size(), 0);
    std::queue<std::array<int, 3>> pending;
    pending.push({seedI, seedJ, seedK});
    visited[seedIndex] = 1;
    int visitedCount = 0;
    while (!pending.empty()) {
        if ((visitedCount++ & 63) == 0 && token.isCancelled()) {
            result.cancelled = true;
            result.edit.changes.clear();
            result.message = "Segmentation cancelled";
            return result;
        }
        const std::array<int, 3> current = pending.front();
        pending.pop();
        const std::size_t index =
            voxelIndex(map.columns, map.rows, current[0], current[1], current[2]);
        pushChange(&result.edit, static_cast<std::uint32_t>(index), map.voxels[index], label);
        for (int dk = -1; dk <= 1; ++dk) {
            for (int dj = -1; dj <= 1; ++dj) {
                for (int di = -1; di <= 1; ++di) {
                    if (di == 0 && dj == 0 && dk == 0) {
                        continue;
                    }
                    const int neighbors = (di != 0 ? 1 : 0) + (dj != 0 ? 1 : 0) + (dk != 0 ? 1 : 0);
                    if (connectivity == Connectivity::Faces6 && neighbors != 1) {
                        continue;
                    }
                    const int ni = current[0] + di;
                    const int nj = current[1] + dj;
                    const int nk = current[2] + dk;
                    if (!inside(map, ni, nj, nk) || !insideBounds(bounds, ni, nj, nk)) {
                        continue;
                    }
                    const std::size_t next = voxelIndex(map.columns, map.rows, ni, nj, nk);
                    if (visited[next] != 0) {
                        continue;
                    }
                    const std::uint8_t existing = map.voxels[next];
                    if (existing != 0 && existing != label) {
                        continue;
                    }
                    if (!inRange(volume.huAt(ni, nj, nk), minHu, maxHu)) {
                        continue;
                    }
                    visited[next] = 1;
                    pending.push({ni, nj, nk});
                }
            }
        }
    }
    return result;
}

LabelEdit brushEdit(const LabelMap& map, const BrushStamp& stamp) {
    LabelEdit edit;
    int centerI = 0;
    int centerJ = 0;
    int centerK = 0;
    if (!planeCenter(map, stamp, &centerI, &centerJ, &centerK) || !(stamp.radiusMm >= 0)) {
        return edit;
    }
    double xMm = 1;
    double yMm = 1;
    planeSpacing(map, stamp.plane, &xMm, &yMm);
    const int reachX = static_cast<int>(std::ceil(stamp.radiusMm / xMm));
    const int reachY = static_cast<int>(std::ceil(stamp.radiusMm / yMm));
    for (int dy = -reachY; dy <= reachY; ++dy) {
        for (int dx = -reachX; dx <= reachX; ++dx) {
            int i = centerI;
            int j = centerJ;
            int k = centerK;
            if (stamp.plane == MprPlane::Axial) {
                i += dx;
                j += dy;
            } else if (stamp.plane == MprPlane::Sagittal) {
                j += dx;
                k += dy;
            } else {
                i += dx;
                k += dy;
            }
            if (!inside(map, i, j, k)) {
                continue;
            }
            const double distance = std::hypot(static_cast<double>(dx) * xMm, static_cast<double>(dy) * yMm);
            if (distance > stamp.radiusMm) {
                continue;
            }
            const std::size_t index = voxelIndex(map.columns, map.rows, i, j, k);
            pushChange(&edit, static_cast<std::uint32_t>(index), map.voxels[index], stamp.label);
        }
    }
    return edit;
}

void applyLabelEdit(LabelMap& map, const LabelEdit& edit) {
    for (const VoxelChange& change : edit.changes) {
        if (change.index < map.voxels.size()) {
            map.voxels[change.index] = change.after;
        }
    }
}

void revertLabelEdit(LabelMap& map, const LabelEdit& edit) {
    for (const VoxelChange& change : edit.changes) {
        if (change.index < map.voxels.size()) {
            map.voxels[change.index] = change.before;
        }
    }
}

LabelMapCommand::LabelMapCommand(LabelMap* map, LabelEdit edit, const char* name)
    : map_(map), edit_(std::move(edit)), name_(name != nullptr ? name : "") {}

const char* LabelMapCommand::name() const {
    return name_;
}

bool LabelMapCommand::execute() {
    if (map_ == nullptr || applied_) {
        return false;
    }
    applyLabelEdit(*map_, edit_);
    applied_ = true;
    return true;
}

bool LabelMapCommand::undo() {
    if (map_ == nullptr || !applied_) {
        return false;
    }
    revertLabelEdit(*map_, edit_);
    applied_ = false;
    return true;
}

bool saveLabelMap(const LabelMap& map, const std::filesystem::path& path, std::string* error) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        if (error != nullptr) {
            *error = "Could not write the label map";
        }
        return false;
    }
    const char magic[8] = {'M', 'F', 'M', 'A', 'S', 'K', '1', 1};
    out.write(magic, 8);
    const auto columns = static_cast<std::uint32_t>(map.columns);
    const auto rows = static_cast<std::uint32_t>(map.rows);
    const auto slices = static_cast<std::uint32_t>(map.slices);
    const auto uidBytes = static_cast<std::uint32_t>(map.seriesInstanceUid.size());
    writePod(out, columns);
    writePod(out, rows);
    writePod(out, slices);
    writePod(out, uidBytes);
    out.write(map.seriesInstanceUid.data(), static_cast<std::streamsize>(map.seriesInstanceUid.size()));
    writePod(out, map.geometry.originMm.x);
    writePod(out, map.geometry.originMm.y);
    writePod(out, map.geometry.originMm.z);
    writePod(out, map.geometry.spacingMm.x);
    writePod(out, map.geometry.spacingMm.y);
    writePod(out, map.geometry.spacingMm.z);
    for (double value : map.geometry.direction.m) {
        writePod(out, value);
    }
    if (!map.voxels.empty()) {
        out.write(reinterpret_cast<const char*>(map.voxels.data()),
                  static_cast<std::streamsize>(map.voxels.size()));
    }
    if (!out) {
        if (error != nullptr) {
            *error = "Could not finish the label map";
        }
        return false;
    }
    return true;
}

bool loadLabelMap(const std::filesystem::path& path, LabelMap* map, std::string* error) {
    if (map == nullptr) {
        if (error != nullptr) {
            *error = "No label map destination";
        }
        return false;
    }
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        if (error != nullptr) {
            *error = "Could not read the label map";
        }
        return false;
    }
    char magic[8] = {};
    in.read(magic, 8);
    if (!in || std::string(magic, 7) != "MFMASK1" || magic[7] != 1) {
        if (error != nullptr) {
            *error = "Label map header was not recognized";
        }
        return false;
    }
    std::uint32_t columns = 0;
    std::uint32_t rows = 0;
    std::uint32_t slices = 0;
    std::uint32_t uidBytes = 0;
    if (!readPod(in, &columns) || !readPod(in, &rows) || !readPod(in, &slices) || !readPod(in, &uidBytes) ||
        uidBytes > 256) {
        if (error != nullptr) {
            *error = "Label map header was truncated";
        }
        return false;
    }
    LabelMap loaded;
    loaded.seriesInstanceUid.resize(uidBytes);
    in.read(loaded.seriesInstanceUid.data(), static_cast<std::streamsize>(uidBytes));
    if (!readPod(in, &loaded.geometry.originMm.x) || !readPod(in, &loaded.geometry.originMm.y) ||
        !readPod(in, &loaded.geometry.originMm.z) || !readPod(in, &loaded.geometry.spacingMm.x) ||
        !readPod(in, &loaded.geometry.spacingMm.y) || !readPod(in, &loaded.geometry.spacingMm.z)) {
        if (error != nullptr) {
            *error = "Label map geometry was truncated";
        }
        return false;
    }
    for (double& value : loaded.geometry.direction.m) {
        if (!readPod(in, &value)) {
            if (error != nullptr) {
                *error = "Label map direction was truncated";
            }
            return false;
        }
    }
    loaded.columns = static_cast<int>(columns);
    loaded.rows = static_cast<int>(rows);
    loaded.slices = static_cast<int>(slices);
    const std::size_t count = static_cast<std::size_t>(columns) * static_cast<std::size_t>(rows) *
                              static_cast<std::size_t>(slices);
    loaded.voxels.resize(count);
    if (count > 0) {
        in.read(reinterpret_cast<char*>(loaded.voxels.data()), static_cast<std::streamsize>(count));
    }
    if (!in) {
        if (error != nullptr) {
            *error = "Label map voxels were truncated";
        }
        return false;
    }
    *map = std::move(loaded);
    return true;
}

void extractLabelPlane(const LabelMap& map,
                       MprPlane plane,
                       int slice,
                       int* width,
                       int* height,
                       std::vector<std::uint8_t>* labels) {
    if (width == nullptr || height == nullptr || labels == nullptr) {
        return;
    }
    *width = 0;
    *height = 0;
    labels->clear();
    if (map.columns <= 0 || map.rows <= 0 || map.slices <= 0) {
        return;
    }
    if (plane == MprPlane::Axial) {
        const int k = std::clamp(slice, 0, map.slices - 1);
        *width = map.columns;
        *height = map.rows;
        labels->resize(static_cast<std::size_t>(*width * *height));
        for (int j = 0; j < map.rows; ++j) {
            for (int i = 0; i < map.columns; ++i) {
                (*labels)[static_cast<std::size_t>(j * *width + i)] = map.at(i, j, k);
            }
        }
        return;
    }
    if (plane == MprPlane::Sagittal) {
        const int i = std::clamp(slice, 0, map.columns - 1);
        *width = map.rows;
        *height = map.slices;
        labels->resize(static_cast<std::size_t>(*width * *height));
        for (int k = 0; k < map.slices; ++k) {
            for (int j = 0; j < map.rows; ++j) {
                (*labels)[static_cast<std::size_t>(k * *width + j)] = map.at(i, j, k);
            }
        }
        return;
    }
    const int j = std::clamp(slice, 0, map.rows - 1);
    *width = map.columns;
    *height = map.slices;
    labels->resize(static_cast<std::size_t>(*width * *height));
    for (int k = 0; k < map.slices; ++k) {
        for (int i = 0; i < map.columns; ++i) {
            (*labels)[static_cast<std::size_t>(k * *width + i)] = map.at(i, j, k);
        }
    }
}

} // namespace medforge::imaging
