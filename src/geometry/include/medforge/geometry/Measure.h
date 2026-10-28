#pragma once

#include "medforge/core/Coordinates.h"
#include "medforge/core/EntityId.h"
#include "medforge/geometry/Surface.h"

#include <string>
#include <vector>

namespace medforge::geometry {

enum class MeasureKind { Distance, Angle, SectionArea };

struct Landmark {
    core::EntityId id;
    std::string name;
    core::Vec3 positionMm{};
    bool snapped = false;
};

struct MeasureDefinition {
    core::EntityId id;
    std::string name;
    MeasureKind kind = MeasureKind::Distance;
    std::vector<core::EntityId> landmarks;
};

struct MeasureValue {
    core::EntityId id;
    bool ok = false;
    double value = 0;
    std::string unit;
    std::string message;
};

struct SnapResult {
    core::Vec3 positionMm{};
    bool snapped = false;
};

struct MeasureReportContext {
    std::string seriesUid;
    core::Vec3 spacingMm{1, 1, 1};
};

struct MeasureDocument {
    std::vector<Landmark> landmarks;
    std::vector<MeasureDefinition> measures;
    std::uint64_t nextId = 1;

    const Landmark* findLandmark(core::EntityId id) const;
    Landmark* findLandmark(core::EntityId id);

    core::EntityId addLandmark(std::string name, core::Vec3 positionMm, bool snapped);
    bool moveLandmark(core::EntityId id, core::Vec3 positionMm, bool snapped);
    bool removeLandmark(core::EntityId id);

    core::EntityId addMeasure(std::string name, MeasureKind kind, std::vector<core::EntityId> landmarks);
    bool removeMeasure(core::EntityId id);
};

SnapResult snapToSurface(const TriangleMesh& mesh, const core::Vec3& pointMm, double toleranceMm);

std::vector<MeasureValue> evaluateMeasures(const MeasureDocument& document, const TriangleMesh* mesh);

bool saveMeasureDocument(const MeasureDocument& document,
                         const std::string& seriesUid,
                         const std::string& path,
                         std::string* error);
bool loadMeasureDocument(const std::string& path,
                         MeasureDocument* document,
                         std::string* seriesUid,
                         std::string* error);

std::string measureReportCsv(const MeasureDocument& document,
                             const TriangleMesh* mesh,
                             const MeasureReportContext& context);
std::string measureReportText(const MeasureDocument& document,
                              const TriangleMesh* mesh,
                              const MeasureReportContext& context);

} // namespace medforge::geometry
