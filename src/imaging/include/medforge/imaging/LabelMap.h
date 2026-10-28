#pragma once

#include "medforge/core/Cancellation.h"
#include "medforge/core/CommandHistory.h"
#include "medforge/imaging/CtVolume.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace medforge::imaging {

struct LabelMap {
    std::string seriesInstanceUid;
    core::VoxelGridGeometry geometry{};
    int columns = 0;
    int rows = 0;
    int slices = 0;
    std::vector<std::uint8_t> voxels;

    std::size_t voxelCount() const;
    std::uint8_t at(int i, int j, int k) const;
};

struct VoxelChange {
    std::uint32_t index = 0;
    std::uint8_t before = 0;
    std::uint8_t after = 0;
};

struct LabelEdit {
    std::vector<VoxelChange> changes;
};

struct SegmentResult {
    LabelEdit edit;
    bool cancelled = false;
    std::string message;
};

enum class Connectivity { Faces6, Neighborhood26 };

struct IndexBounds {
    bool limited = false;
    int iMin = 0;
    int iMax = 0;
    int jMin = 0;
    int jMax = 0;
    int kMin = 0;
    int kMax = 0;
};

struct BrushStamp {
    MprPlane plane = MprPlane::Axial;
    int slice = 0;
    int x = 0;
    int y = 0;
    double radiusMm = 1;
    std::uint8_t label = 1;
};

LabelMap emptyLabelMap(const CtVolume& volume);
std::size_t countLabel(const LabelMap& map, std::uint8_t label);
std::size_t countInHuRange(const CtVolume& volume, float minHu, float maxHu);

LabelEdit thresholdEdit(const CtVolume& volume,
                        const LabelMap& map,
                        std::uint8_t label,
                        float minHu,
                        float maxHu);
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
                             int delayBeforeMs = 0);
LabelEdit brushEdit(const LabelMap& map, const BrushStamp& stamp);

void applyLabelEdit(LabelMap& map, const LabelEdit& edit);
void revertLabelEdit(LabelMap& map, const LabelEdit& edit);

class LabelMapCommand : public core::Command {
public:
    LabelMapCommand(LabelMap* map, LabelEdit edit, const char* name);

    const char* name() const override;
    bool execute() override;
    bool undo() override;

private:
    LabelMap* map_ = nullptr;
    LabelEdit edit_;
    const char* name_ = "";
    bool applied_ = false;
};

bool saveLabelMap(const LabelMap& map, const std::filesystem::path& path, std::string* error);
bool loadLabelMap(const std::filesystem::path& path, LabelMap* map, std::string* error);

void extractLabelPlane(const LabelMap& map,
                       MprPlane plane,
                       int slice,
                       int* width,
                       int* height,
                       std::vector<std::uint8_t>* labels);

} // namespace medforge::imaging
