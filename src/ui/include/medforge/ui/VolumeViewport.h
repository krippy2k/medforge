#pragma once

#include "medforge/core/Coordinates.h"
#include "medforge/geometry/Surface.h"
#include "medforge/rendering/CameraPose.h"
#include "medforge/rendering/TransferFunction.h"

#include <QString>
#include <QWidget>
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

protected:
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    struct Scene;
    void ensureScene();
    void chooseMapper();
    void pushPose();
    void pushTransfer();
    void pushClip();
    void rebuildMeshActor();
    void rebuildLandmarks();
    QPointF project(const medforge::core::Vec3& point, const QRect& bounds) const;

    std::unique_ptr<Scene> scene_;
    medforge::rendering::CameraPose pose_ = medforge::rendering::homeCamera();
    medforge::rendering::TransferFunction transfer_ = medforge::rendering::softTissueTransfer();
    medforge::geometry::TriangleMesh mesh_;
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
