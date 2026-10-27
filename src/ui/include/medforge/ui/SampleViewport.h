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
