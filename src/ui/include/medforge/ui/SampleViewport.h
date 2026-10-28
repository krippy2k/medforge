#pragma once

#include "medforge/rendering/CameraPose.h"

#include <QString>
#include <QWidget>

class QShowEvent;

namespace medforge::ui {
void prepareSampleViewportSurface();
}

class SampleViewport : public QWidget {
    Q_OBJECT

public:
    explicit SampleViewport(QWidget* parent = nullptr);

    // Create the native OpenGL child before the top-level window is shown.
    // Windows cannot change a window's pixel format, so creating that child
    // later makes Qt close the workspace and open a replacement.
    void realizeNativeWindow();

    bool showSample(QString* error);
    bool saveSampleImage(const QString& filePath, QString* error);
    bool resetCamera(QString* error);
    bool applyPose(const medforge::rendering::CameraPose& pose, QString* error);
    medforge::rendering::CameraPose pose() const;
    bool sampleVisible() const;

protected:
    void showEvent(QShowEvent* event) override;

private:
    void syncLabel();

    void ensureScene();
    void pushPoseToRenderer();

    medforge::rendering::CameraPose pose_ = medforge::rendering::homeCamera();
    bool sampleVisible_ = false;
    class QLabel* label_ = nullptr;
    class QWidget* renderWidget_ = nullptr;
};
