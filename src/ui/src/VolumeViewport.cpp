#include "medforge/ui/VolumeViewport.h"

#include "medforge/core/Coordinates.h"
#include "medforge/imaging/CtVolume.h"
#include "medforge/ui/DesignTokens.h"
#include "medforge/ui/GuiThread.h"

#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QShowEvent>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <vector>

#ifdef MEDFORGE_HAS_VTK
#include <QVTKOpenGLNativeWidget.h>
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkColorTransferFunction.h>
#include <vtkAutoInit.h>
#include <vtkFixedPointVolumeRayCastMapper.h>

VTK_AUTOINIT_DECLARE(vtkRenderingOpenGL2)
VTK_AUTOINIT_DECLARE(vtkInteractionStyle)
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageData.h>
#include <vtkMatrix3x3.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkVolumeMapper.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPlane.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkLineSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkSphereSource.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#endif

namespace {

medforge::core::Vec3 cornerOf(const medforge::core::VoxelGridGeometry& grid,
                              int i,
                              int j,
                              int k) {
    const medforge::core::CoordinateResult point =
        medforge::core::voxelIndexToPatientLps(medforge::core::Vec3{static_cast<double>(i),
                                                                    static_cast<double>(j),
                                                                    static_cast<double>(k)},
                                               grid);
    return point.ok() ? point.value : medforge::core::Vec3{};
}

} // namespace

struct VolumeViewport::Scene {
#ifdef MEDFORGE_HAS_VTK
    vtkSmartPointer<vtkRenderer> renderer;
    vtkSmartPointer<vtkImageData> image;
    vtkSmartPointer<vtkVolume> volume;
    vtkSmartPointer<vtkActor> meshActor;
    vtkSmartPointer<vtkActor> implantActor;
    vtkSmartPointer<vtkActor> guideActor;
    std::vector<vtkSmartPointer<vtkActor>> gizmoActors;
    std::vector<vtkSmartPointer<vtkActor>> landmarkActors;
    vtkSmartPointer<vtkPlane> clip;
    vtkSmartPointer<vtkVolumeMapper> mapper;
#endif
};

VolumeViewport::VolumeViewport(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("volumeViewport"));
    setFocusPolicy(Qt::StrongFocus);
    setAutoFillBackground(false);
    scene_ = std::make_unique<Scene>();
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
}

VolumeViewport::~VolumeViewport() = default;

void VolumeViewport::realizeNativeWindow() {
    ensureScene();
    if (glWidget_ != nullptr) {
        glWidget_->winId();
    }
}

void VolumeViewport::ensureScene() {
#ifdef MEDFORGE_HAS_VTK
    if (glWidget_ != nullptr ||
        qEnvironmentVariable("QT_QPA_PLATFORM") == QLatin1String("offscreen")) {
        if (glWidget_ == nullptr) {
            renderStatus_ = QStringLiteral("Volume ray casting needs a display");
        }
        return;
    }
    auto* widget = new QVTKOpenGLNativeWidget(this);
    vtkNew<vtkGenericOpenGLRenderWindow> window;
    widget->setRenderWindow(window);
    scene_->renderer = vtkSmartPointer<vtkRenderer>::New();
    scene_->renderer->SetBackground(0.082, 0.098, 0.122);
    window->AddRenderer(scene_->renderer);
    scene_->clip = vtkSmartPointer<vtkPlane>::New();
    layout()->addWidget(widget);
    glWidget_ = widget;
    renderStatus_ = QStringLiteral("Volume ray casting needs a display");
#else
    renderStatus_ = QStringLiteral("Volume ray casting is not built into this executable");
#endif
}

void VolumeViewport::chooseMapper() {
#ifdef MEDFORGE_HAS_VTK
    if (scene_->mapper != nullptr || glWidget_ == nullptr) {
        return;
    }
    // A static library drops VTK's autoinit unless something calls it. The GPU
    // volume mapper is not registered in this package, and neither is the CPU
    // mapper's display helper. Actors still need the OpenGL factory.
    vtkRenderingOpenGL2_AutoInit_Construct();
    vtkInteractionStyle_AutoInit_Construct();
    registerVolumeRayCastHelper();
    scene_->mapper = vtkSmartPointer<vtkFixedPointVolumeRayCastMapper>::New();
    if (scene_->mapper == nullptr) {
        renderStatus_ = QStringLiteral("Volume ray casting is unavailable");
        return;
    }
    renderStatus_ = QStringLiteral("CPU ray casting");
#endif
}

void VolumeViewport::clearStudy() {
    landmarks_.clear();
    selectedLandmark_ = -1;
    rebuildLandmarks();
    hasVolume_ = false;
    columns_ = 0;
    rows_ = 0;
    slices_ = 0;
    clearMesh();
    implantMesh_ = {};
    guideMesh_ = {};
    gizmoVisible_ = false;
    implantOverlaps_ = false;
    dragAxis_ = -1;
    rebuildCadActors();
#ifdef MEDFORGE_HAS_VTK
    if (scene_->renderer != nullptr && scene_->volume != nullptr) {
        scene_->renderer->RemoveVolume(scene_->volume);
        scene_->volume = nullptr;
    }
#endif
    renderStatus_ = QStringLiteral("Import a CT series to view the volume");
    update();
}

bool VolumeViewport::showVolume(const medforge::imaging::CtVolume& volume, QString* error) {
    if (!medforge::ui::onGuiThread()) {
        if (error != nullptr) {
            *error = QStringLiteral("Rendering must run on the GUI thread");
        }
        return false;
    }
    if (volume.columns < 2 || volume.rows < 2 || volume.slices < 2 || volume.hu.empty()) {
        if (error != nullptr) {
            *error = QStringLiteral("The CT volume is empty");
        }
        return false;
    }
    ensureScene();
    chooseMapper();
    grid_ = volume.geometry.geometry;
    columns_ = volume.columns;
    rows_ = volume.rows;
    slices_ = volume.slices;
    bool first = true;
    for (const int i : {0, columns_ - 1}) {
        for (const int j : {0, rows_ - 1}) {
            for (const int k : {0, slices_ - 1}) {
                const medforge::core::Vec3 point = cornerOf(grid_, i, j, k);
                if (first) {
                    boundsMin_ = point;
                    boundsMax_ = point;
                    first = false;
                } else {
                    boundsMin_.x = std::min(boundsMin_.x, point.x);
                    boundsMin_.y = std::min(boundsMin_.y, point.y);
                    boundsMin_.z = std::min(boundsMin_.z, point.z);
                    boundsMax_.x = std::max(boundsMax_.x, point.x);
                    boundsMax_.y = std::max(boundsMax_.y, point.y);
                    boundsMax_.z = std::max(boundsMax_.z, point.z);
                }
            }
        }
    }
    hasVolume_ = true;
#ifdef MEDFORGE_HAS_VTK
    if (scene_->mapper != nullptr && scene_->renderer != nullptr) {
        scene_->image = vtkSmartPointer<vtkImageData>::New();
        scene_->image->SetDimensions(columns_, rows_, slices_);
        scene_->image->SetSpacing(grid_.spacingMm.x, grid_.spacingMm.y, grid_.spacingMm.z);
        scene_->image->SetOrigin(grid_.originMm.x, grid_.originMm.y, grid_.originMm.z);
        vtkNew<vtkMatrix3x3> direction;
        for (int row = 0; row < 3; ++row) {
            for (int column = 0; column < 3; ++column) {
                direction->SetElement(row, column, grid_.direction(row, column));
            }
        }
        scene_->image->SetDirectionMatrix(direction);
        scene_->image->AllocateScalars(VTK_FLOAT, 1);
        auto* scalars = static_cast<float*>(scene_->image->GetScalarPointer());
        for (std::size_t index = 0; index < volume.hu.size(); ++index) {
            const float value = volume.hu[index];
            scalars[index] = std::isfinite(value) ? value : -1000.0f;
        }
        scene_->mapper->SetInputData(scene_->image);
        scene_->mapper->RemoveAllClippingPlanes();
        if (scene_->volume == nullptr) {
            scene_->volume = vtkSmartPointer<vtkVolume>::New();
            scene_->volume->SetMapper(scene_->mapper);
            scene_->renderer->AddVolume(scene_->volume);
        }
        pushTransfer();
        pushClip();
        if (renderStatus_.contains(QStringLiteral("needs a display"))) {
            renderStatus_ = QStringLiteral("Volume ray casting needs a display");
        }
    }
#endif
    resetToBounds(nullptr);
    update();
    return true;
}

void VolumeViewport::setTransferFunction(const medforge::rendering::TransferFunction& function) {
    transfer_ = function;
    pushTransfer();
    update();
}

void VolumeViewport::setVolumeVisible(bool visible) {
    volumeVisible_ = visible;
#ifdef MEDFORGE_HAS_VTK
    if (scene_->volume != nullptr) {
        scene_->volume->SetVisibility(visible ? 1 : 0);
        if (glWidget_ != nullptr) {
            glWidget_->update();
        }
    }
#endif
    update();
}

void VolumeViewport::setMesh(const medforge::geometry::TriangleMesh& mesh) {
    mesh_ = mesh;
    rebuildMeshActor();
    update();
}

void VolumeViewport::clearMesh() {
    mesh_ = {};
    rebuildMeshActor();
    update();
}

#ifdef MEDFORGE_HAS_VTK
namespace {

vtkSmartPointer<vtkActor> actorFromMesh(const medforge::geometry::TriangleMesh& mesh,
                                        double red,
                                        double green,
                                        double blue) {
    if (mesh.positions.empty() || mesh.indices.size() < 3) {
        return {};
    }
    vtkNew<vtkPoints> points;
    points->SetNumberOfPoints(static_cast<vtkIdType>(mesh.positions.size()));
    for (std::size_t index = 0; index < mesh.positions.size(); ++index) {
        const medforge::core::Vec3& position = mesh.positions[index];
        points->SetPoint(static_cast<vtkIdType>(index), position.x, position.y, position.z);
    }
    vtkNew<vtkCellArray> triangles;
    for (std::size_t index = 0; index + 2 < mesh.indices.size(); index += 3) {
        vtkIdType ids[3] = {static_cast<vtkIdType>(mesh.indices[index]),
                            static_cast<vtkIdType>(mesh.indices[index + 1]),
                            static_cast<vtkIdType>(mesh.indices[index + 2])};
        triangles->InsertNextCell(3, ids);
    }
    vtkNew<vtkPolyData> poly;
    poly->SetPoints(points);
    poly->SetPolys(triangles);
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputData(poly);
    mapper->ScalarVisibilityOff();
    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(red, green, blue);
    actor->GetProperty()->SetAmbient(0.35);
    actor->GetProperty()->SetDiffuse(0.65);
    return actor;
}

} // namespace
#endif

void VolumeViewport::setCadOverlay(const medforge::geometry::TriangleMesh& implant,
                                   const medforge::geometry::TriangleMesh& guide,
                                   const medforge::core::Vec3& origin,
                                   bool gizmo,
                                   bool overlap) {
    implantMesh_ = implant;
    guideMesh_ = guide;
    gizmoOrigin_ = origin;
    gizmoVisible_ = gizmo;
    implantOverlaps_ = overlap;
    rebuildCadActors();
    update();
}

medforge::core::Vec3 VolumeViewport::gizmoTipPoint(int axis) const {
    medforge::core::Vec3 tip = gizmoOrigin_;
    constexpr double length = 12;
    if (axis == 0) {
        tip.x += length;
    } else if (axis == 1) {
        tip.y += length;
    } else if (axis == 2) {
        tip.z += length;
    }
    return tip;
}

QPoint VolumeViewport::gizmoAnchor() const {
    const QRect bounds = rect().adjusted(12, 12, -12, -28);
    return project(gizmoOrigin_, bounds).toPoint();
}

QPoint VolumeViewport::gizmoTip(int axis) const {
    const QRect bounds = rect().adjusted(12, 12, -12, -28);
    return project(gizmoTipPoint(axis), bounds).toPoint();
}

int VolumeViewport::gizmoAxisAt(const QPoint& point) const {
    if (!gizmoVisible_) {
        return -1;
    }
    const QRect bounds = rect().adjusted(12, 12, -12, -28);
    const QPointF origin = project(gizmoOrigin_, bounds);
    int best = -1;
    double bestDistance = 8;
    for (int axis = 0; axis < 3; ++axis) {
        const QPointF tip = project(gizmoTipPoint(axis), bounds);
        const QPointF span = tip - origin;
        const double spanLength = std::hypot(span.x(), span.y());
        if (spanLength < 4) {
            continue;
        }
        const double lengthSquared = (span.x() * span.x()) + (span.y() * span.y());
        double along = ((point.x() - origin.x()) * span.x() + (point.y() - origin.y()) * span.y()) /
                       lengthSquared;
        along = std::clamp(along, 0.0, 1.0);
        const QPointF closest(origin.x() + (span.x() * along), origin.y() + (span.y() * along));
        const double distance = std::hypot(point.x() - closest.x(), point.y() - closest.y());
        if (distance <= bestDistance) {
            bestDistance = distance;
            best = axis;
        }
    }
    return best;
}

double VolumeViewport::gizmoMillimeters(int axis, const QPoint& from, const QPoint& to) const {
    if (axis < 0 || axis > 2) {
        return 0;
    }
    const QRect bounds = rect().adjusted(12, 12, -12, -28);
    const QPointF origin = project(gizmoOrigin_, bounds);
    const QPointF tip = project(gizmoTipPoint(axis), bounds);
    const QPointF span = tip - origin;
    const double spanLength = std::hypot(span.x(), span.y());
    if (spanLength < 1) {
        return 0;
    }
    const double pixels =
        ((to.x() - from.x()) * span.x() + (to.y() - from.y()) * span.y()) / spanLength;
    return pixels / spanLength * 12.0;
}

void VolumeViewport::setLandmarks(const std::vector<medforge::core::Vec3>& positions, int selected) {
    landmarks_ = positions;
    selectedLandmark_ = selected;
    rebuildLandmarks();
    update();
}

void VolumeViewport::rebuildLandmarks() {
#ifdef MEDFORGE_HAS_VTK
    if (scene_->renderer == nullptr) {
        return;
    }
    for (const auto& actor : scene_->landmarkActors) {
        scene_->renderer->RemoveActor(actor);
    }
    scene_->landmarkActors.clear();
    for (std::size_t index = 0; index < landmarks_.size(); ++index) {
        vtkNew<vtkSphereSource> sphere;
        sphere->SetRadius(1.5);
        sphere->SetThetaResolution(16);
        sphere->SetPhiResolution(16);
        sphere->SetCenter(landmarks_[index].x, landmarks_[index].y, landmarks_[index].z);
        vtkNew<vtkPolyDataMapper> mapper;
        mapper->SetInputConnection(sphere->GetOutputPort());
        auto actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        const bool selected = static_cast<int>(index) == selectedLandmark_;
        actor->GetProperty()->SetColor(selected ? 0.29 : 0.96, selected ? 0.64 : 0.96, selected ? 0.78 : 0.97);
        scene_->renderer->AddActor(actor);
        scene_->landmarkActors.push_back(actor);
    }
    if (glWidget_ != nullptr) {
        glWidget_->update();
    }
#else
    Q_UNUSED(landmarks_);
#endif
}

void VolumeViewport::setClipPlane(bool enabled,
                                  const medforge::core::Vec3& origin,
                                  const medforge::core::Vec3& normal) {
    clipEnabled_ = enabled;
    clipOrigin_ = origin;
    clipNormal_ = normal;
    pushClip();
    update();
}

bool VolumeViewport::applyPose(const medforge::rendering::CameraPose& pose, QString* error) {
    if (!medforge::ui::onGuiThread()) {
        if (error != nullptr) {
            *error = QStringLiteral("Rendering must run on the GUI thread");
        }
        return false;
    }
    pose_ = pose;
    pushPose();
    update();
    return true;
}

bool VolumeViewport::applyAnatomicalView(medforge::rendering::AnatomicalView view, QString* error) {
    if (!hasVolume_) {
        if (error != nullptr) {
            *error = QStringLiteral("Camera presets are available after a CT volume is loaded");
        }
        return false;
    }
    return applyPose(medforge::rendering::anatomicalCamera(view, boundsMin_, boundsMax_), error);
}

bool VolumeViewport::resetToBounds(QString* error) {
    if (!hasVolume_) {
        if (error != nullptr) {
            *error = QStringLiteral("Reset camera is available after a CT volume is loaded");
        }
        return false;
    }
    return applyPose(
        medforge::rendering::anatomicalCamera(medforge::rendering::AnatomicalView::Anterior,
                                              boundsMin_,
                                              boundsMax_),
        error);
}

medforge::rendering::CameraPose VolumeViewport::pose() const {
    return pose_;
}

QString VolumeViewport::renderStatus() const {
    return renderStatus_;
}

QString VolumeViewport::transferName() const {
    return QString::fromStdString(transfer_.name);
}

bool VolumeViewport::hasVolume() const {
    return hasVolume_;
}

bool VolumeViewport::volumeVisible() const {
    return volumeVisible_;
}

int VolumeViewport::meshTriangleCount() const {
    return static_cast<int>(mesh_.indices.size() / 3);
}

bool VolumeViewport::meshBounds(medforge::core::Vec3* minimum,
                                medforge::core::Vec3* maximum) const {
    if (mesh_.positions.empty() || minimum == nullptr || maximum == nullptr) {
        return false;
    }
    medforge::core::Vec3 low = mesh_.positions.front();
    medforge::core::Vec3 high = low;
    for (const medforge::core::Vec3& position : mesh_.positions) {
        low.x = std::min(low.x, position.x);
        low.y = std::min(low.y, position.y);
        low.z = std::min(low.z, position.z);
        high.x = std::max(high.x, position.x);
        high.y = std::max(high.y, position.y);
        high.z = std::max(high.z, position.z);
    }
    *minimum = low;
    *maximum = high;
    return true;
}

void VolumeViewport::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    ensureScene();
    chooseMapper();
}

void VolumeViewport::pushPose() {
#ifdef MEDFORGE_HAS_VTK
    if (scene_->renderer == nullptr || scene_->renderer->GetActiveCamera() == nullptr) {
        return;
    }
    vtkCamera* camera = scene_->renderer->GetActiveCamera();
    camera->SetPosition(pose_.position[0], pose_.position[1], pose_.position[2]);
    camera->SetFocalPoint(pose_.focalPoint[0], pose_.focalPoint[1], pose_.focalPoint[2]);
    camera->SetViewUp(pose_.viewUp[0], pose_.viewUp[1], pose_.viewUp[2]);
    scene_->renderer->ResetCameraClippingRange();
    if (glWidget_ != nullptr) {
        glWidget_->update();
    }
#endif
}

void VolumeViewport::pushTransfer() {
#ifdef MEDFORGE_HAS_VTK
    if (scene_->volume == nullptr) {
        return;
    }
    vtkNew<vtkColorTransferFunction> color;
    vtkNew<vtkPiecewiseFunction> opacity;
    for (const medforge::rendering::TransferStop& stop : transfer_.stops) {
        color->AddRGBPoint(stop.hu, stop.red, stop.green, stop.blue);
        opacity->AddPoint(stop.hu, stop.opacity);
    }
    vtkNew<vtkVolumeProperty> property;
    property->SetColor(color);
    property->SetScalarOpacity(opacity);
    property->SetInterpolationTypeToLinear();
    property->ShadeOn();
    const double spacing = std::min({grid_.spacingMm.x, grid_.spacingMm.y, grid_.spacingMm.z});
    property->SetScalarOpacityUnitDistance(spacing > 0 ? spacing : 1.0);
    scene_->volume->SetProperty(property);
    scene_->volume->SetVisibility(volumeVisible_ ? 1 : 0);
    if (glWidget_ != nullptr) {
        glWidget_->update();
    }
#endif
}

void VolumeViewport::pushClip() {
#ifdef MEDFORGE_HAS_VTK
    if (scene_->mapper == nullptr || scene_->clip == nullptr) {
        return;
    }
    scene_->mapper->RemoveAllClippingPlanes();
    if (clipEnabled_) {
        scene_->clip->SetOrigin(clipOrigin_.x, clipOrigin_.y, clipOrigin_.z);
        scene_->clip->SetNormal(clipNormal_.x, clipNormal_.y, clipNormal_.z);
        scene_->mapper->AddClippingPlane(scene_->clip);
    }
    if (glWidget_ != nullptr) {
        glWidget_->update();
    }
#endif
}

void VolumeViewport::rebuildCadActors() {
#ifdef MEDFORGE_HAS_VTK
    if (scene_->renderer == nullptr) {
        return;
    }
    if (scene_->implantActor != nullptr) {
        scene_->renderer->RemoveActor(scene_->implantActor);
        scene_->implantActor = nullptr;
    }
    if (scene_->guideActor != nullptr) {
        scene_->renderer->RemoveActor(scene_->guideActor);
        scene_->guideActor = nullptr;
    }
    for (const auto& actor : scene_->gizmoActors) {
        scene_->renderer->RemoveActor(actor);
    }
    scene_->gizmoActors.clear();
    const double red = implantOverlaps_ ? 0.77 : 0.91;
    const double green = implantOverlaps_ ? 0.36 : 0.66;
    const double blue = implantOverlaps_ ? 0.29 : 0.38;
    scene_->implantActor = actorFromMesh(implantMesh_, red, green, blue);
    if (scene_->implantActor != nullptr) {
        scene_->renderer->AddActor(scene_->implantActor);
    }
    scene_->guideActor = actorFromMesh(guideMesh_, 0.49, 0.66, 0.73);
    if (scene_->guideActor != nullptr) {
        scene_->renderer->AddActor(scene_->guideActor);
    }
    if (gizmoVisible_) {
        const QColor colors[3] = {QColor(196, 72, 64), QColor(64, 150, 96), QColor(64, 112, 196)};
        for (int axis = 0; axis < 3; ++axis) {
            const medforge::core::Vec3 tip = gizmoTipPoint(axis);
            vtkNew<vtkLineSource> line;
            line->SetPoint1(gizmoOrigin_.x, gizmoOrigin_.y, gizmoOrigin_.z);
            line->SetPoint2(tip.x, tip.y, tip.z);
            vtkNew<vtkPolyDataMapper> mapper;
            mapper->SetInputConnection(line->GetOutputPort());
            auto actor = vtkSmartPointer<vtkActor>::New();
            actor->SetMapper(mapper);
            actor->GetProperty()->SetLineWidth(3);
            actor->GetProperty()->SetColor(colors[axis].redF(), colors[axis].greenF(),
                                           colors[axis].blueF());
            scene_->renderer->AddActor(actor);
            scene_->gizmoActors.push_back(actor);
        }
    }
    if (glWidget_ != nullptr) {
        glWidget_->update();
    }
#endif
}

void VolumeViewport::rebuildMeshActor() {
#ifdef MEDFORGE_HAS_VTK
    if (scene_->renderer == nullptr) {
        return;
    }
    if (scene_->meshActor != nullptr) {
        scene_->renderer->RemoveActor(scene_->meshActor);
        scene_->meshActor = nullptr;
    }
    if (mesh_.positions.empty() || mesh_.indices.size() < 3) {
        return;
    }
    vtkNew<vtkPoints> points;
    points->SetNumberOfPoints(static_cast<vtkIdType>(mesh_.positions.size()));
    for (std::size_t index = 0; index < mesh_.positions.size(); ++index) {
        const medforge::core::Vec3& position = mesh_.positions[index];
        points->SetPoint(static_cast<vtkIdType>(index), position.x, position.y, position.z);
    }
    vtkNew<vtkCellArray> triangles;
    for (std::size_t index = 0; index + 2 < mesh_.indices.size(); index += 3) {
        vtkIdType ids[3] = {static_cast<vtkIdType>(mesh_.indices[index]),
                            static_cast<vtkIdType>(mesh_.indices[index + 1]),
                            static_cast<vtkIdType>(mesh_.indices[index + 2])};
        triangles->InsertNextCell(3, ids);
    }
    vtkNew<vtkPolyData> poly;
    poly->SetPoints(points);
    poly->SetPolys(triangles);
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputData(poly);
    mapper->ScalarVisibilityOff();
    scene_->meshActor = vtkSmartPointer<vtkActor>::New();
    scene_->meshActor->SetMapper(mapper);
    scene_->meshActor->GetProperty()->SetColor(0.93, 0.90, 0.82);
    scene_->meshActor->GetProperty()->SetAmbient(0.3);
    scene_->meshActor->GetProperty()->SetDiffuse(0.7);
    scene_->renderer->AddActor(scene_->meshActor);
    if (glWidget_ != nullptr) {
        glWidget_->update();
    }
#endif
}

QPointF VolumeViewport::project(const medforge::core::Vec3& point, const QRect& bounds) const {
    const medforge::core::Vec3 forward{
        pose_.focalPoint[0] - pose_.position[0],
        pose_.focalPoint[1] - pose_.position[1],
        pose_.focalPoint[2] - pose_.position[2],
    };
    const double forwardLength =
        std::sqrt((forward.x * forward.x) + (forward.y * forward.y) + (forward.z * forward.z));
    const medforge::core::Vec3 f = forwardLength > 1e-9
                                       ? medforge::core::Vec3{forward.x / forwardLength,
                                                              forward.y / forwardLength,
                                                              forward.z / forwardLength}
                                       : medforge::core::Vec3{0, 1, 0};
    const medforge::core::Vec3 up{pose_.viewUp[0], pose_.viewUp[1], pose_.viewUp[2]};
    medforge::core::Vec3 right{(f.y * up.z) - (f.z * up.y), (f.z * up.x) - (f.x * up.z),
                               (f.x * up.y) - (f.y * up.x)};
    const double rightLength =
        std::sqrt((right.x * right.x) + (right.y * right.y) + (right.z * right.z));
    if (rightLength > 1e-9) {
        right.x /= rightLength;
        right.y /= rightLength;
        right.z /= rightLength;
    }
    const medforge::core::Vec3 offset{point.x - pose_.focalPoint[0], point.y - pose_.focalPoint[1],
                                      point.z - pose_.focalPoint[2]};
    const double x = (offset.x * right.x) + (offset.y * right.y) + (offset.z * right.z);
    const double y = (offset.x * up.x) + (offset.y * up.y) + (offset.z * up.z);
    const double span = std::max({std::abs(boundsMax_.x - boundsMin_.x),
                                  std::abs(boundsMax_.y - boundsMin_.y),
                                  std::abs(boundsMax_.z - boundsMin_.z),
                                  1.0});
    const double scale = 0.45 * std::min(bounds.width(), bounds.height()) / span;
    return QPointF(bounds.center().x() + (x * scale), bounds.center().y() - (y * scale));
}

void VolumeViewport::paintTriangles(QPainter& painter,
                                   const medforge::geometry::TriangleMesh& mesh,
                                   const QColor& color,
                                   const QRect& bounds) const {
    if (mesh.indices.size() < 3) {
        return;
    }
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    const std::size_t triangles = std::min<std::size_t>(mesh.indices.size() / 3, 4000);
    for (std::size_t index = 0; index < triangles; ++index) {
        const std::uint32_t v0 = mesh.indices[index * 3];
        const std::uint32_t v1 = mesh.indices[index * 3 + 1];
        const std::uint32_t v2 = mesh.indices[index * 3 + 2];
        if (v0 >= mesh.positions.size() || v1 >= mesh.positions.size() ||
            v2 >= mesh.positions.size()) {
            continue;
        }
        const QPointF projected[3] = {project(mesh.positions[v0], bounds),
                                      project(mesh.positions[v1], bounds),
                                      project(mesh.positions[v2], bounds)};
        painter.drawPolygon(projected, 3);
    }
}

void VolumeViewport::mousePressEvent(QMouseEvent* event) {
    dragAxis_ = gizmoAxisAt(event->position().toPoint());
    if (dragAxis_ < 0) {
        QWidget::mousePressEvent(event);
        return;
    }
    dragLast_ = event->position().toPoint();
    event->accept();
}

void VolumeViewport::mouseMoveEvent(QMouseEvent* event) {
    if (dragAxis_ < 0) {
        QWidget::mouseMoveEvent(event);
        return;
    }
    const QPoint next = event->position().toPoint();
    const double delta = gizmoMillimeters(dragAxis_, dragLast_, next);
    dragLast_ = next;
    if (std::abs(delta) > 1e-6) {
        emit gizmoDragged(dragAxis_, delta);
    }
    event->accept();
}

void VolumeViewport::mouseReleaseEvent(QMouseEvent* event) {
    if (dragAxis_ < 0) {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    dragAxis_ = -1;
    event->accept();
}

void VolumeViewport::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    if (glWidget_ != nullptr) {
        return;
    }
    QPainter painter(this);
    painter.fillRect(rect(), medforge::ui::tokens::raised);
    painter.setPen(medforge::ui::tokens::textSecondary);
    const QRect bounds = rect().adjusted(12, 12, -12, -28);
    if (!hasVolume_) {
        painter.drawText(rect().adjusted(12, 12, -12, -12),
                         Qt::AlignCenter | Qt::TextWordWrap,
                         renderStatus_);
        return;
    }
    painter.setPen(QPen(medforge::ui::tokens::border, 1));
    const int i1 = std::max(columns_ - 1, 0);
    const int j1 = std::max(rows_ - 1, 0);
    const int k1 = std::max(slices_ - 1, 0);
    const medforge::core::Vec3 corners[8] = {
        cornerOf(grid_, 0, 0, 0),  cornerOf(grid_, i1, 0, 0), cornerOf(grid_, i1, j1, 0),
        cornerOf(grid_, 0, j1, 0), cornerOf(grid_, 0, 0, k1), cornerOf(grid_, i1, 0, k1),
        cornerOf(grid_, i1, j1, k1), cornerOf(grid_, 0, j1, k1)};
    const int edges[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
                              {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
    for (const auto& edge : edges) {
        painter.drawLine(project(corners[edge[0]], bounds), project(corners[edge[1]], bounds));
    }
    paintTriangles(painter, mesh_, QColor(242, 235, 218, 180), bounds);
    paintTriangles(painter, guideMesh_, QColor(126, 168, 186, 200), bounds);
    paintTriangles(painter,
                   implantMesh_,
                   implantOverlaps_ ? QColor(196, 92, 74, 210) : QColor(232, 168, 96, 210),
                   bounds);
    if (gizmoVisible_) {
        const QColor axisColors[3] = {QColor(196, 72, 64), QColor(64, 150, 96), QColor(64, 112, 196)};
        const QPointF origin = project(gizmoOrigin_, bounds);
        for (int axis = 0; axis < 3; ++axis) {
            painter.setPen(QPen(axisColors[axis], 2));
            painter.drawLine(origin, project(gizmoTipPoint(axis), bounds));
        }
    }
    for (std::size_t index = 0; index < landmarks_.size(); ++index) {
        const QPointF center = project(landmarks_[index], bounds);
        const bool selected = static_cast<int>(index) == selectedLandmark_;
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(8, 12, 16));
        painter.drawEllipse(center, selected ? 7 : 5, selected ? 7 : 5);
        painter.setBrush(selected ? medforge::ui::tokens::accent : QColor(244, 246, 248));
        painter.drawEllipse(center, 3, 3);
    }
    painter.setPen(medforge::ui::tokens::textSecondary);
    painter.drawText(QRect(8, height() - 22, width() - 16, 18), Qt::AlignLeft | Qt::AlignVCenter,
                     renderStatus_);
}
