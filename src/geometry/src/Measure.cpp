#include "medforge/geometry/Measure.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <tuple>
#include <unordered_map>

namespace medforge::geometry {
namespace {

constexpr double kEpsilon = 1e-9;

struct Vec {
    double x = 0;
    double y = 0;
    double z = 0;
};

Vec asVec(const core::Vec3& value) {
    return {value.x, value.y, value.z};
}

core::Vec3 asCore(const Vec& value) {
    return {value.x, value.y, value.z};
}

Vec operator-(const Vec& left, const Vec& right) {
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}

Vec operator+(const Vec& left, const Vec& right) {
    return {left.x + right.x, left.y + right.y, left.z + right.z};
}

Vec operator*(const Vec& value, double scale) {
    return {value.x * scale, value.y * scale, value.z * scale};
}

double dot(const Vec& left, const Vec& right) {
    return (left.x * right.x) + (left.y * right.y) + (left.z * right.z);
}

Vec cross(const Vec& left, const Vec& right) {
    return {(left.y * right.z) - (left.z * right.y), (left.z * right.x) - (left.x * right.z),
            (left.x * right.y) - (left.y * right.x)};
}

double length(const Vec& value) {
    return std::sqrt(dot(value, value));
}

Vec normalize(const Vec& value) {
    const double scale = length(value);
    if (scale < kEpsilon) {
        return {};
    }
    return value * (1.0 / scale);
}

bool finite(const Vec& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

std::string kindName(MeasureKind kind) {
    switch (kind) {
    case MeasureKind::Angle:
        return "angle";
    case MeasureKind::SectionArea:
        return "area";
    case MeasureKind::Distance:
        break;
    }
    return "distance";
}

bool kindFromName(const std::string& name, MeasureKind* kind) {
    if (name == "distance") {
        *kind = MeasureKind::Distance;
        return true;
    }
    if (name == "angle") {
        *kind = MeasureKind::Angle;
        return true;
    }
    if (name == "area") {
        *kind = MeasureKind::SectionArea;
        return true;
    }
    return false;
}

std::string unitFor(MeasureKind kind) {
    switch (kind) {
    case MeasureKind::Angle:
        return "deg";
    case MeasureKind::SectionArea:
        return "mm^2";
    case MeasureKind::Distance:
        break;
    }
    return "mm";
}

const Landmark* lookup(const MeasureDocument& document, core::EntityId id) {
    return document.findLandmark(id);
}

core::Vec3 closestOnTriangle(const core::Vec3& point,
                             const core::Vec3& a,
                             const core::Vec3& b,
                             const core::Vec3& c) {
    const Vec p = asVec(point);
    const Vec va = asVec(a);
    const Vec vb = asVec(b);
    const Vec vc = asVec(c);
    const Vec ab = vb - va;
    const Vec ac = vc - va;
    const Vec ap = p - va;
    const double d1 = dot(ab, ap);
    const double d2 = dot(ac, ap);
    if (d1 <= 0 && d2 <= 0) {
        return a;
    }
    const Vec bp = p - vb;
    const double d3 = dot(ab, bp);
    const double d4 = dot(ac, bp);
    if (d3 >= 0 && d4 <= d3) {
        return b;
    }
    const double vcEdge = (d1 * d4) - (d3 * d2);
    if (vcEdge <= 0 && d1 >= 0 && d3 <= 0) {
        const double v = d1 / (d1 - d3);
        return asCore(va + (ab * v));
    }
    const Vec cp = p - vc;
    const double d5 = dot(ab, cp);
    const double d6 = dot(ac, cp);
    if (d6 >= 0 && d5 <= d6) {
        return c;
    }
    const double vbEdge = (d5 * d2) - (d1 * d6);
    if (vbEdge <= 0 && d2 >= 0 && d6 <= 0) {
        const double w = d2 / (d2 - d6);
        return asCore(va + (ac * w));
    }
    const double vaEdge = (d3 * d6) - (d5 * d4);
    if (vaEdge <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0) {
        const double w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return asCore(vb + ((vc - vb) * w));
    }
    const double denom = 1.0 / (vaEdge + vbEdge + vcEdge);
    const double v = vbEdge * denom;
    const double w = vcEdge * denom;
    return asCore(va + (ab * v) + (ac * w));
}

bool edgeHit(const Vec& p, const Vec& q, double dp, double dq, Vec* hit) {
    if (std::abs(dp) <= 1e-8 && std::abs(dq) <= 1e-8) {
        return false;
    }
    if (std::abs(dp) <= 1e-8) {
        *hit = p;
        return true;
    }
    if (std::abs(dq) <= 1e-8) {
        *hit = q;
        return true;
    }
    if ((dp > 0 && dq > 0) || (dp < 0 && dq < 0)) {
        return false;
    }
    const double t = dp / (dp - dq);
    *hit = p + ((q - p) * t);
    return true;
}

struct Key {
    long long x = 0;
    long long y = 0;
    long long z = 0;

    bool operator==(const Key& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

struct KeyHash {
    std::size_t operator()(const Key& key) const {
        return static_cast<std::size_t>(key.x * 1315423911LL) ^
               static_cast<std::size_t>(key.y * 2654435761LL) ^
               static_cast<std::size_t>(key.z);
    }
};

Key keyOf(const Vec& point) {
    return {std::llround(point.x * 100000.0), std::llround(point.y * 100000.0),
            std::llround(point.z * 100000.0)};
}

double polygonArea(const std::vector<Vec>& loop, const Vec& origin, const Vec& normal) {
    const Vec n = normalize(normal);
    const Vec helper = std::abs(n.x) < 0.9 ? Vec{1, 0, 0} : Vec{0, 1, 0};
    const Vec u = normalize(cross(n, helper));
    const Vec v = cross(n, u);
    double area = 0;
    for (std::size_t index = 0; index < loop.size(); ++index) {
        const Vec& current = loop[index];
        const Vec& next = loop[(index + 1) % loop.size()];
        const double x1 = dot(current - origin, u);
        const double y1 = dot(current - origin, v);
        const double x2 = dot(next - origin, u);
        const double y2 = dot(next - origin, v);
        area += (x1 * y2) - (x2 * y1);
    }
    return std::abs(area) * 0.5;
}

MeasureValue sectionArea(const TriangleMesh& mesh, const Vec& origin, const Vec& normal) {
    MeasureValue value;
    value.unit = "mm^2";
    struct Segment {
        Key from;
        Key to;
        Vec start;
        Vec end;
        bool used = false;
    };
    std::vector<Segment> segments;
    std::unordered_map<Key, Vec, KeyHash> positions;
    const Vec n = normalize(normal);
    for (std::size_t index = 0; index + 2 < mesh.indices.size(); index += 3) {
        const std::uint32_t ids[3] = {mesh.indices[index], mesh.indices[index + 1],
                                      mesh.indices[index + 2]};
        if (ids[0] >= mesh.positions.size() || ids[1] >= mesh.positions.size() ||
            ids[2] >= mesh.positions.size()) {
            continue;
        }
        const Vec corners[3] = {asVec(mesh.positions[ids[0]]), asVec(mesh.positions[ids[1]]),
                                asVec(mesh.positions[ids[2]])};
        const double distance[3] = {dot(corners[0] - origin, n), dot(corners[1] - origin, n),
                                    dot(corners[2] - origin, n)};
        Vec hits[3];
        int hitCount = 0;
        for (int edge = 0; edge < 3; ++edge) {
            const int next = (edge + 1) % 3;
            Vec hit;
            if (!edgeHit(corners[edge], corners[next], distance[edge], distance[next], &hit)) {
                continue;
            }
            bool duplicate = false;
            for (int existing = 0; existing < hitCount; ++existing) {
                if (length(hits[existing] - hit) < 1e-6) {
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate && hitCount < 3) {
                hits[hitCount++] = hit;
            }
        }
        if (hitCount != 2) {
            continue;
        }
        Key first = keyOf(hits[0]);
        Key second = keyOf(hits[1]);
        if (first == second) {
            continue;
        }
        if (std::tie(second.x, second.y, second.z) < std::tie(first.x, first.y, first.z)) {
            std::swap(first, second);
            std::swap(hits[0], hits[1]);
        }
        bool seen = false;
        for (const Segment& segment : segments) {
            if (segment.from == first && segment.to == second) {
                seen = true;
                break;
            }
        }
        if (seen) {
            continue;
        }
        segments.push_back(Segment{first, second, hits[0], hits[1], false});
        positions[first] = hits[0];
        positions[second] = hits[1];
    }
    if (segments.empty()) {
        value.message = "Plane misses the surface";
        return value;
    }
    double area = 0;
    bool closed = false;
    for (std::size_t start = 0; start < segments.size(); ++start) {
        if (segments[start].used) {
            continue;
        }
        std::vector<Vec> loop;
        loop.push_back(segments[start].start);
        Vec current = segments[start].end;
        Key currentKey = segments[start].to;
        segments[start].used = true;
        bool loopClosed = false;
        for (std::size_t step = 0; step < segments.size(); ++step) {
            if (length(current - loop.front()) < 1e-4) {
                loopClosed = true;
                break;
            }
            loop.push_back(current);
            bool found = false;
            for (Segment& segment : segments) {
                if (segment.used) {
                    continue;
                }
                if (segment.from == currentKey) {
                    segment.used = true;
                    current = segment.end;
                    currentKey = segment.to;
                    found = true;
                    break;
                }
                if (segment.to == currentKey) {
                    segment.used = true;
                    current = segment.start;
                    currentKey = segment.from;
                    found = true;
                    break;
                }
            }
            if (!found) {
                break;
            }
        }
        if (loopClosed && loop.size() >= 3) {
            area += polygonArea(loop, origin, n);
            closed = true;
        }
    }
    if (!closed) {
        value.message = "Open cross-section";
        return value;
    }
    value.ok = true;
    value.value = area;
    return value;
}

std::string csvField(const std::string& text) {
    if (text.find_first_of(",\"\n") == std::string::npos) {
        return text;
    }
    std::string escaped = "\"";
    for (char character : text) {
        if (character == '"') {
            escaped += "\"\"";
        } else {
            escaped += character;
        }
    }
    escaped += "\"";
    return escaped;
}

std::string landmarkNames(const MeasureDocument& document, const MeasureDefinition& measure) {
    std::string names;
    for (core::EntityId id : measure.landmarks) {
        const Landmark* landmark = lookup(document, id);
        if (!names.empty()) {
            names += " ";
        }
        names += landmark != nullptr ? landmark->name : "?";
    }
    return names;
}

} // namespace

const Landmark* MeasureDocument::findLandmark(core::EntityId id) const {
    for (const Landmark& landmark : landmarks) {
        if (landmark.id == id) {
            return &landmark;
        }
    }
    return nullptr;
}

Landmark* MeasureDocument::findLandmark(core::EntityId id) {
    for (Landmark& landmark : landmarks) {
        if (landmark.id == id) {
            return &landmark;
        }
    }
    return nullptr;
}

core::EntityId MeasureDocument::addLandmark(std::string name, core::Vec3 positionMm, bool snapped) {
    Landmark landmark;
    landmark.id = core::EntityId::fromValue(nextId++);
    landmark.name = std::move(name);
    landmark.positionMm = positionMm;
    landmark.snapped = snapped;
    landmarks.push_back(landmark);
    return landmark.id;
}

bool MeasureDocument::moveLandmark(core::EntityId id, core::Vec3 positionMm, bool snapped) {
    Landmark* landmark = findLandmark(id);
    if (landmark == nullptr) {
        return false;
    }
    landmark->positionMm = positionMm;
    landmark->snapped = snapped;
    return true;
}

bool MeasureDocument::removeLandmark(core::EntityId id) {
    const auto found = std::find_if(landmarks.begin(), landmarks.end(), [&](const Landmark& landmark) {
        return landmark.id == id;
    });
    if (found == landmarks.end()) {
        return false;
    }
    landmarks.erase(found);
    return true;
}

core::EntityId MeasureDocument::addMeasure(std::string name,
                                          MeasureKind kind,
                                          std::vector<core::EntityId> landmarkIds) {
    MeasureDefinition measure;
    measure.id = core::EntityId::fromValue(nextId++);
    measure.name = std::move(name);
    measure.kind = kind;
    measure.landmarks = std::move(landmarkIds);
    measures.push_back(measure);
    return measure.id;
}

bool MeasureDocument::removeMeasure(core::EntityId id) {
    const auto found = std::find_if(measures.begin(), measures.end(), [&](const MeasureDefinition& measure) {
        return measure.id == id;
    });
    if (found == measures.end()) {
        return false;
    }
    measures.erase(found);
    return true;
}

SnapResult snapToSurface(const TriangleMesh& mesh, const core::Vec3& pointMm, double toleranceMm) {
    SnapResult result;
    result.positionMm = pointMm;
    if (!(toleranceMm >= 0) || mesh.indices.size() < 3) {
        return result;
    }
    double best = toleranceMm;
    core::Vec3 closest = pointMm;
    bool found = false;
    for (std::size_t index = 0; index + 2 < mesh.indices.size(); index += 3) {
        const std::uint32_t i0 = mesh.indices[index];
        const std::uint32_t i1 = mesh.indices[index + 1];
        const std::uint32_t i2 = mesh.indices[index + 2];
        if (i0 >= mesh.positions.size() || i1 >= mesh.positions.size() || i2 >= mesh.positions.size()) {
            continue;
        }
        const core::Vec3 point =
            closestOnTriangle(pointMm, mesh.positions[i0], mesh.positions[i1], mesh.positions[i2]);
        const double distance = length(asVec(point) - asVec(pointMm));
        if (distance <= best) {
            best = distance;
            closest = point;
            found = true;
        }
    }
    if (found) {
        result.positionMm = closest;
        result.snapped = true;
    }
    return result;
}

std::vector<MeasureValue> evaluateMeasures(const MeasureDocument& document, const TriangleMesh* mesh) {
    std::vector<MeasureValue> values;
    for (const MeasureDefinition& measure : document.measures) {
        MeasureValue value;
        value.id = measure.id;
        value.unit = unitFor(measure.kind);
        const auto need = measure.kind == MeasureKind::Distance ? 2 : 3;
        if (static_cast<int>(measure.landmarks.size()) < need) {
            value.message = "Missing landmark";
            values.push_back(value);
            continue;
        }
        const Landmark* points[3] = {};
        bool missing = false;
        for (int index = 0; index < need; ++index) {
            points[index] = lookup(document, measure.landmarks[static_cast<std::size_t>(index)]);
            if (points[index] == nullptr || !finite(asVec(points[index]->positionMm))) {
                missing = true;
            }
        }
        if (missing) {
            value.message = "Missing landmark";
            values.push_back(value);
            continue;
        }
        if (measure.kind == MeasureKind::Distance) {
            value.ok = true;
            value.value = length(asVec(points[1]->positionMm) - asVec(points[0]->positionMm));
            values.push_back(value);
            continue;
        }
        if (measure.kind == MeasureKind::Angle) {
            const Vec fromVertex = asVec(points[0]->positionMm) - asVec(points[1]->positionMm);
            const Vec toOther = asVec(points[2]->positionMm) - asVec(points[1]->positionMm);
            const double left = length(fromVertex);
            const double right = length(toOther);
            if (left < kEpsilon || right < kEpsilon) {
                value.message = "Need two distinct directions";
                values.push_back(value);
                continue;
            }
            const double cosine = std::clamp(dot(fromVertex, toOther) / (left * right), -1.0, 1.0);
            value.ok = true;
            value.value = std::acos(cosine) * 180.0 / 3.14159265358979323846;
            values.push_back(value);
            continue;
        }
        const Vec ab = asVec(points[1]->positionMm) - asVec(points[0]->positionMm);
        const Vec ac = asVec(points[2]->positionMm) - asVec(points[0]->positionMm);
        const Vec normal = cross(ab, ac);
        if (length(normal) < kEpsilon) {
            value.message = "Landmarks are collinear";
            values.push_back(value);
            continue;
        }
        if (mesh == nullptr) {
            value.message = "No surface";
            values.push_back(value);
            continue;
        }
        MeasureValue area = sectionArea(*mesh, asVec(points[0]->positionMm), normal);
        area.id = measure.id;
        values.push_back(area);
    }
    return values;
}

bool saveMeasureDocument(const MeasureDocument& document,
                         const std::string& seriesUid,
                         const std::string& path,
                         std::string* error) {
    std::ofstream out(path, std::ios::trunc);
    if (!out) {
        if (error != nullptr) {
            *error = "Could not write measurements";
        }
        return false;
    }
    out << "MFMEASURE 1\n";
    out << "series=" << seriesUid << "\n";
    out << "next=" << document.nextId << "\n";
    for (const Landmark& landmark : document.landmarks) {
        out << "landmark\n";
        out << "id=" << landmark.id.value() << "\n";
        out << "name=" << landmark.name << "\n";
        out << "x=" << landmark.positionMm.x << "\n";
        out << "y=" << landmark.positionMm.y << "\n";
        out << "z=" << landmark.positionMm.z << "\n";
        out << "snapped=" << (landmark.snapped ? 1 : 0) << "\n";
    }
    for (const MeasureDefinition& measure : document.measures) {
        out << "measure\n";
        out << "id=" << measure.id.value() << "\n";
        out << "name=" << measure.name << "\n";
        out << "kind=" << kindName(measure.kind) << "\n";
        out << "landmarks=";
        for (std::size_t index = 0; index < measure.landmarks.size(); ++index) {
            if (index > 0) {
                out << ",";
            }
            out << measure.landmarks[index].value();
        }
        out << "\n";
    }
    return true;
}

bool loadMeasureDocument(const std::string& path,
                         MeasureDocument* document,
                         std::string* seriesUid,
                         std::string* error) {
    std::ifstream in(path);
    if (!in || document == nullptr) {
        if (error != nullptr) {
            *error = "Could not read measurements";
        }
        return false;
    }
    std::string header;
    std::getline(in, header);
    if (header != "MFMEASURE 1") {
        if (error != nullptr) {
            *error = "Unrecognized measurement file";
        }
        return false;
    }
    MeasureDocument loaded;
    std::string series;
    Landmark landmark;
    MeasureDefinition measure;
    bool inLandmark = false;
    bool inMeasure = false;
    std::string line;
    while (std::getline(in, line)) {
        if (line == "landmark") {
            if (inLandmark) {
                loaded.landmarks.push_back(landmark);
            }
            if (inMeasure) {
                loaded.measures.push_back(measure);
            }
            landmark = {};
            inLandmark = true;
            inMeasure = false;
            continue;
        }
        if (line == "measure") {
            if (inLandmark) {
                loaded.landmarks.push_back(landmark);
            }
            if (inMeasure) {
                loaded.measures.push_back(measure);
            }
            measure = {};
            inMeasure = true;
            inLandmark = false;
            continue;
        }
        const auto split = line.find('=');
        if (split == std::string::npos) {
            continue;
        }
        const std::string key = line.substr(0, split);
        const std::string value = line.substr(split + 1);
        if (key == "series") {
            series = value;
        } else if (key == "next") {
            loaded.nextId = std::stoull(value);
        } else if (inLandmark && key == "id") {
            landmark.id = core::EntityId::fromValue(std::stoull(value));
        } else if (inLandmark && key == "name") {
            landmark.name = value;
        } else if (inLandmark && key == "x") {
            landmark.positionMm.x = std::stod(value);
        } else if (inLandmark && key == "y") {
            landmark.positionMm.y = std::stod(value);
        } else if (inLandmark && key == "z") {
            landmark.positionMm.z = std::stod(value);
        } else if (inLandmark && key == "snapped") {
            landmark.snapped = value != "0";
        } else if (inMeasure && key == "id") {
            measure.id = core::EntityId::fromValue(std::stoull(value));
        } else if (inMeasure && key == "name") {
            measure.name = value;
        } else if (inMeasure && key == "kind") {
            if (!kindFromName(value, &measure.kind)) {
                if (error != nullptr) {
                    *error = "Unrecognized measurement";
                }
                return false;
            }
        } else if (inMeasure && key == "landmarks") {
            std::stringstream stream(value);
            std::string item;
            while (std::getline(stream, item, ',')) {
                if (!item.empty()) {
                    measure.landmarks.push_back(core::EntityId::fromValue(std::stoull(item)));
                }
            }
        }
    }
    if (inLandmark) {
        loaded.landmarks.push_back(landmark);
    }
    if (inMeasure) {
        loaded.measures.push_back(measure);
    }
    *document = std::move(loaded);
    if (seriesUid != nullptr) {
        *seriesUid = series;
    }
    return true;
}

std::string measureReportCsv(const MeasureDocument& document,
                             const TriangleMesh* mesh,
                             const MeasureReportContext& context) {
    std::ostringstream out;
    out << "name,kind,value,unit,landmarks,series\n";
    const auto values = evaluateMeasures(document, mesh);
    for (std::size_t index = 0; index < document.measures.size() && index < values.size(); ++index) {
        const MeasureDefinition& measure = document.measures[index];
        const MeasureValue& value = values[index];
        out << csvField(measure.name) << "," << kindName(measure.kind) << ",";
        if (value.ok) {
            out << value.value;
        }
        out << "," << value.unit << "," << csvField(landmarkNames(document, measure)) << ","
            << csvField(context.seriesUid) << "\n";
    }
    return out.str();
}

std::string measureReportText(const MeasureDocument& document,
                              const TriangleMesh* mesh,
                              const MeasureReportContext& context) {
    std::ostringstream out;
    out << "MedForge research measurements\n";
    out << "Research/demo only — not for clinical use.\n";
    out << "Not a validated measurement. Limited by CT resolution, segmentation, and landmark placement.\n";
    out << "Spacing " << context.spacingMm.x << " x " << context.spacingMm.y << " x " << context.spacingMm.z
        << " mm.\n";
    out << "Series " << context.seriesUid << "\n";
    const auto values = evaluateMeasures(document, mesh);
    for (std::size_t index = 0; index < document.measures.size() && index < values.size(); ++index) {
        const MeasureDefinition& measure = document.measures[index];
        const MeasureValue& value = values[index];
        out << measure.name << " (" << kindName(measure.kind) << ", " << landmarkNames(document, measure)
            << "): ";
        if (!value.ok) {
            out << value.message << "\n";
            continue;
        }
        const char* unit = value.unit.c_str();
        if (measure.kind == MeasureKind::Angle) {
            unit = "degrees";
        } else if (measure.kind == MeasureKind::SectionArea) {
            unit = "mm^2";
        }
        out << value.value << " " << unit << "\n";
    }
    return out.str();
}

} // namespace medforge::geometry
