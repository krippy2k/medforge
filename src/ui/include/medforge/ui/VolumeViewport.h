#pragma once

#include "medforge/core/Coordinates.h"
#include "medforge/geometry/Surface.h"
#include "medforge/rendering/CameraPose.h"
#include "medforge/rendering/TransferFunction.h"

#include <QColor>
#include <QPoint>
#include <QString>
#include <QWidget>

class QMouseEvent;
class QPainter;
#include <memory>
#include <vector>

namespace medforge::imaging {
struct CtVolume;
}

class VolumeViewport : public QWidget {
    Q_OBJECT

public:
    explicit VolumeViewport(QWidget* parent = nullptr);
    ~VolumeViewport() override;

    void realizeNativeWindow();
    void clearStudy();
    bool showVolume(const medforge::imaging::CtVolume& volume, QString* error);
    void setTransferFunction(const medforge::rendering::TransferFunction& function);
    void setVolumeVisible(bool visible);
    void setMesh(const medforge::geometry::TriangleMesh& mesh);
    void clearMesh();
    void setCadOverlay(const medforge::geometry::TriangleMesh& implant,
                       const medforge::geometry::TriangleMesh& guide,
                       const medforge::core::Vec3& origin,
                       bool gizmo,
                       bool overlap);
    QPoint gizmoAnchor() const;
    QPoint gizmoTip(int axis) const;
    int gizmoAxisAt(const QPoint& point) const;
    double gizmoMillimeters(int axis, const QPoint& from, const QPoint& to) const;
    void setLandmarks(const std::vector<medforge::core::Vec3>& positions, int selected);
    void setClipPlane(bool enabled,
                      const medforge::core::Vec3& origin,
                      const medforge::core::Vec3& normal);
    bool applyPose(const medforge::rendering::CameraPose& pose, QString* error);
    bool applyAnatomicalView(medforge::rendering::AnatomicalView view, QString* error);
    bool resetToBounds(QString* error);

    medforge::rendering::CameraPose pose() const;
    QString renderStatus() const;
    QString transferName() const;
    bool hasVolume() const;
    bool volumeVisible() const;
    int meshTriangleCount() const;
    bool meshBounds(medforge::core::Vec3* minimum, medforge::core::Vec3* maximum) const;

signals:
    void gizmoDragged(int axis, double deltaMm);

protected:
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    struct Scene;
    void ensureScene();
    void chooseMapper();
    void pushPose();
    void pushTransfer();
    void pushClip();
    void rebuildMeshActor();
    void rebuildCadActors();
    void rebuildLandmarks();
    void paintTriangles(QPainter& painter,
                        const medforge::geometry::TriangleMesh& mesh,
                        const QColor& color,
                        const QRect& bounds) const;
    QPointF project(const medforge::core::Vec3& point, const QRect& bounds) const;
    medforge::core::Vec3 gizmoTipPoint(int axis) const;

    std::unique_ptr<Scene> scene_;
    medforge::rendering::CameraPose pose_ = medforge::rendering::homeCamera();
    medforge::rendering::TransferFunction transfer_ = medforge::rendering::softTissueTransfer();
    medforge::geometry::TriangleMesh mesh_;
    medforge::geometry::TriangleMesh implantMesh_;
    medforge::geometry::TriangleMesh guideMesh_;
    medforge::core::Vec3 gizmoOrigin_{};
    bool gizmoVisible_ = false;
    bool implantOverlaps_ = false;
    int dragAxis_ = -1;
    QPoint dragLast_{};
    std::vector<medforge::core::Vec3> landmarks_;
    int selectedLandmark_ = -1;
    medforge::core::VoxelGridGeometry grid_{};
    medforge::core::Vec3 boundsMin_{};
    medforge::core::Vec3 boundsMax_{};
    int columns_ = 0;
    int rows_ = 0;
    int slices_ = 0;
    bool hasVolume_ = false;
    bool volumeVisible_ = true;
    bool clipEnabled_ = false;
    medforge::core::Vec3 clipOrigin_{};
    medforge::core::Vec3 clipNormal_{0, 0, 1};
    QString renderStatus_ = QStringLiteral("Import a CT series to view the volume");
    class QWidget* glWidget_ = nullptr;
};

// This VTK package has no vtkOpenGLRayCastImageDisplayHelper. Register ours
// before creating vtkFixedPointVolumeRayCastMapper.
void registerVolumeRayCastHelper();
