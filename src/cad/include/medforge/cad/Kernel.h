#pragma once

#include "medforge/core/Coordinates.h"
#include "medforge/geometry/Surface.h"

#include <memory>
#include <string>

namespace medforge::cad {

// Rotations are degrees about L, then P, then S, about the world origin.
// Translation in millimeters is applied after those rotations.
struct Pose {
    core::Vec3 translationMm{};
    core::Vec3 rotationDeg{};
};

struct Shape {
    Shape();
    ~Shape();
    Shape(const Shape&);
    Shape& operator=(const Shape&);
    Shape(Shape&&) noexcept;
    Shape& operator=(Shape&&) noexcept;

    bool isNull() const;
    bool isValidSolid() const;
    double volumeMm3() const;
    core::Vec3 boundsMin() const;
    core::Vec3 boundsMax() const;
    const geometry::TriangleMesh& mesh() const;
    const std::string& message() const;

    struct Impl;

private:
    std::shared_ptr<Impl> impl_;
    friend Shape adoptShape(std::shared_ptr<Impl> impl);
    friend struct KernelAccess;
};

struct Clearance {
    bool ok = false;
    double distanceMm = 0;
    double commonVolumeMm3 = 0;
    double toleranceMm = 0;
    bool overlap = false;
    std::string method;
    std::string message;
};

struct GuideSpec {
    core::Vec3 contactOriginMm{};
    double patchLengthMm = 10;
    double patchWidthMm = 8;
    double offsetMm = 4;
    double holeDiameterMm = 2;
};

inline constexpr const char* kResearchWatermark =
    "MedForge research prototype. Not for manufacture or clinical use.";

inline constexpr const char* kClearanceMethod =
    "Minimum distance is BRepExtrema_DistShapeShape. Deflection equals the stated tolerance "
    "in millimeters. Overlap is a common-solid volume above 0.0001 mm3. This is an "
    "approximation, not a clinical clearance.";

bool available();
const char* kernelName();

Shape makeBox(const core::Vec3& corner, const core::Vec3& size, std::string* error);
Shape makeCylinder(const core::Vec3& origin,
                   const core::Vec3& axis,
                   double radiusMm,
                   double heightMm,
                   std::string* error);
Shape transformShape(const Shape& shape, const Pose& pose, std::string* error);
Shape booleanCut(const Shape& target, const Shape& tool, std::string* error);
Clearance measureClearance(const Shape& first, const Shape& second, double toleranceMm);
Shape readStep(const std::string& path, std::string* error);
Shape readIges(const std::string& path, std::string* error);
bool writeStep(const Shape& shape, const std::string& path, std::string* error);
bool writeIges(const Shape& shape, const std::string& path, std::string* error);
Shape buildGuide(const GuideSpec& spec, std::string* error);

} // namespace medforge::cad
