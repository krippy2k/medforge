#include "medforge/rendering/CameraPose.h"

#include <cmath>

namespace medforge::rendering {
namespace {

struct Vec3 {
    double x = 0;
    double y = 0;
    double z = 0;
};

Vec3 from(const std::array<double, 3>& value) {
    return {value[0], value[1], value[2]};
}

std::array<double, 3> toArray(const Vec3& value) {
    return {value.x, value.y, value.z};
}

Vec3 add(const Vec3& left, const Vec3& right) {
    return {left.x + right.x, left.y + right.y, left.z + right.z};
}

Vec3 sub(const Vec3& left, const Vec3& right) {
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}

Vec3 mul(const Vec3& value, double scale) {
    return {value.x * scale, value.y * scale, value.z * scale};
}

double dot(const Vec3& left, const Vec3& right) {
    return (left.x * right.x) + (left.y * right.y) + (left.z * right.z);
}

Vec3 cross(const Vec3& left, const Vec3& right) {
    return {(left.y * right.z) - (left.z * right.y),
            (left.z * right.x) - (left.x * right.z),
            (left.x * right.y) - (left.y * right.x)};
}

Vec3 normalized(const Vec3& value) {
    const double length = std::sqrt(dot(value, value));
    if (length < 1e-12) {
        return {0, 0, 1};
    }
    return mul(value, 1.0 / length);
}

bool nearArray(const std::array<double, 3>& left,
               const std::array<double, 3>& right,
               double tolerance) {
    for (std::size_t index = 0; index < left.size(); ++index) {
        if (std::abs(left[index] - right[index]) > tolerance) {
            return false;
        }
    }
    return true;
}

} // namespace

CameraPose homeCamera() {
    CameraPose pose;
    pose.position = {4.0, -6.0, 4.0};
    pose.focalPoint = {0.0, 0.0, 0.0};
    pose.viewUp = {0.0, 0.0, 1.0};
    return pose;
}

CameraPose orbitYaw(const CameraPose& pose, double degrees) {
    const double radians = degrees * 0.017453292519943295;
    const Vec3 offset = sub(from(pose.position), from(pose.focalPoint));
    const double cosine = std::cos(radians);
    const double sine = std::sin(radians);
    const Vec3 rotated{
        (offset.x * cosine) - (offset.y * sine), (offset.x * sine) + (offset.y * cosine), offset.z};
    CameraPose next = pose;
    next.position = toArray(add(from(pose.focalPoint), rotated));
    return next;
}

CameraPose pan(const CameraPose& pose, double right, double up) {
    const Vec3 forward = normalized(sub(from(pose.focalPoint), from(pose.position)));
    const Vec3 upAxis = normalized(from(pose.viewUp));
    const Vec3 rightAxis = normalized(cross(forward, upAxis));
    const Vec3 cameraUp = normalized(cross(rightAxis, forward));
    const Vec3 delta = add(mul(rightAxis, right), mul(cameraUp, up));
    CameraPose next = pose;
    next.position = toArray(add(from(pose.position), delta));
    next.focalPoint = toArray(add(from(pose.focalPoint), delta));
    return next;
}

CameraPose zoom(const CameraPose& pose, double factor) {
    if (!(factor > 0.0) || !std::isfinite(factor)) {
        return pose;
    }
    const Vec3 offset = sub(from(pose.position), from(pose.focalPoint));
    CameraPose next = pose;
    next.position = toArray(add(from(pose.focalPoint), mul(offset, 1.0 / factor)));
    return next;
}

CameraPose anatomicalCamera(AnatomicalView view,
                            const core::Vec3& boundsMin,
                            const core::Vec3& boundsMax) {
    const Vec3 center{(boundsMin.x + boundsMax.x) * 0.5, (boundsMin.y + boundsMax.y) * 0.5,
                      (boundsMin.z + boundsMax.z) * 0.5};
    const Vec3 extent{std::abs(boundsMax.x - boundsMin.x), std::abs(boundsMax.y - boundsMin.y),
                      std::abs(boundsMax.z - boundsMin.z)};
    const double distance = std::max({extent.x, extent.y, extent.z, 1.0}) * 2.2;
    Vec3 offset{0, -1, 0};
    Vec3 up{0, 0, 1};
    switch (view) {
    case AnatomicalView::Anterior:
        offset = {0, -1, 0};
        break;
    case AnatomicalView::Posterior:
        offset = {0, 1, 0};
        break;
    case AnatomicalView::Left:
        offset = {1, 0, 0};
        break;
    case AnatomicalView::Right:
        offset = {-1, 0, 0};
        break;
    case AnatomicalView::Superior:
        offset = {0, 0, 1};
        up = {0, -1, 0};
        break;
    case AnatomicalView::Inferior:
        offset = {0, 0, -1};
        up = {0, -1, 0};
        break;
    }
    CameraPose pose;
    pose.focalPoint = toArray(center);
    pose.position = toArray(add(center, mul(offset, distance)));
    pose.viewUp = toArray(up);
    return pose;
}

bool cameraPosesNear(const CameraPose& left, const CameraPose& right, double tolerance) {
    return nearArray(left.position, right.position, tolerance) &&
           nearArray(left.focalPoint, right.focalPoint, tolerance) &&
           nearArray(left.viewUp, right.viewUp, tolerance);
}

} // namespace medforge::rendering
