#include "medforge/ui/SampleViewport.h"

#include "medforge/ui/GuiThread.h"

#include <QImage>
#include <QLabel>
#include <QShowEvent>
#include <QTimer>
#include <QVBoxLayout>

#ifdef MEDFORGE_HAS_VTK
#include <QSurfaceFormat>
#include <QVTKOpenGLNativeWidget.h>
#include <vtkActor.h>
#include <vtkAxesActor.h>
#include <vtkCamera.h>
#include <vtkCubeSource.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageData.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkNew.h>
#include <vtkPlaneSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkSphereSource.h>
#include <vtkWindowToImageFilter.h>
#endif

namespace medforge::ui {
void prepareSampleViewportSurface() {
#ifdef MEDFORGE_HAS_VTK
#if defined(Q_OS_LINUX)
    // QVTKOpenGLNativeWidget draws through a native GLX window. Qt on Ubuntu
    // otherwise selects Wayland, and the sample actors never appear.
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "xcb");
    }
#endif
    QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());
#endif
}
} // namespace medforge::ui

SampleViewport::SampleViewport(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("viewport3d"));
    setFocusPolicy(Qt::StrongFocus);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    label_ = new QLabel(QStringLiteral("No sample geometry"), this);
    label_->setObjectName(QStringLiteral("sampleViewportStatus"));
    label_->setWordWrap(true);
    label_->setAlignment(Qt::AlignCenter);
    layout->addWidget(label_);
    syncLabel();
}

void SampleViewport::realizeNativeWindow() {
#ifdef MEDFORGE_HAS_VTK
    if (qEnvironmentVariable("QT_QPA_PLATFORM") == QLatin1String("offscreen")) {
        return;
    }
    ensureScene();
    if (renderWidget_ != nullptr) {
        renderWidget_->winId();
    }
#endif
}

bool SampleViewport::showSample(QString* error) {
    if (!applyPose(medforge::rendering::homeCamera(), error)) {
        return false;
    }
    sampleVisible_ = true;
    if (renderWidget_ != nullptr) {
        label_->hide();
        renderWidget_->show();
    }
    syncLabel();
    pushPoseToRenderer();
    return true;
}

bool SampleViewport::saveSampleImage(const QString& filePath, QString* error) {
    if (!showSample(error)) {
        return false;
    }
#ifndef MEDFORGE_HAS_VTK
    if (error != nullptr) {
        *error = QStringLiteral("Sample rendering is not built into this executable");
    }
    return false;
#else
    auto* widget = qobject_cast<QVTKOpenGLNativeWidget*>(renderWidget_);
    if (widget == nullptr || widget->renderWindow() == nullptr) {
        if (error != nullptr) {
            *error = QStringLiteral("Sample view has no render window");
        }
        return false;
    }
    widget->renderWindow()->Render();
    vtkNew<vtkWindowToImageFilter> filter;
    filter->SetInput(widget->renderWindow());
    filter->SetInputBufferTypeToRGB();
    filter->ReadFrontBufferOff();
    filter->Update();
    vtkImageData* image = filter->GetOutput();
    if (image == nullptr || image->GetNumberOfScalarComponents() < 3) {
        if (error != nullptr) {
            *error = QStringLiteral("Sample view produced no image");
        }
        return false;
    }
    int dimensions[3] = {0, 0, 0};
    image->GetDimensions(dimensions);
    if (dimensions[0] <= 0 || dimensions[1] <= 0) {
        if (error != nullptr) {
            *error = QStringLiteral("Sample view produced an empty image");
        }
        return false;
    }
    const auto* pixels = static_cast<const unsigned char*>(image->GetScalarPointer());
    QImage frame(pixels, dimensions[0], dimensions[1], dimensions[0] * 3, QImage::Format_RGB888);
    frame = frame.flipped(Qt::Vertical);
    if (!frame.save(filePath)) {
        if (error != nullptr) {
            *error = QStringLiteral("Could not write the sample image");
        }
        return false;
    }
    return true;
#endif
}

bool SampleViewport::resetCamera(QString* error) {
    if (!sampleVisible_) {
        if (error != nullptr) {
            *error = QStringLiteral("Reset camera is available after the sample is shown");
        }
        return false;
    }
    return applyPose(medforge::rendering::homeCamera(), error);
}

bool SampleViewport::applyPose(const medforge::rendering::CameraPose& pose, QString* error) {
    if (!medforge::ui::onGuiThread()) {
        if (error != nullptr) {
            *error = QStringLiteral("Rendering must run on the GUI thread");
        }
        return false;
    }
    pose_ = pose;
    pushPoseToRenderer();
    return true;
}

void SampleViewport::ensureScene() {
#ifdef MEDFORGE_HAS_VTK
    if (renderWidget_ != nullptr ||
        qEnvironmentVariable("QT_QPA_PLATFORM") == QLatin1String("offscreen")) {
        return;
    }
    auto* widget = new QVTKOpenGLNativeWidget(this);
    vtkNew<vtkGenericOpenGLRenderWindow> renderWindow;
    widget->setRenderWindow(renderWindow);
    vtkNew<vtkRenderer> renderer;
    renderer->SetBackground(0.082, 0.098, 0.122);
    renderWindow->AddRenderer(renderer);

    vtkNew<vtkCubeSource> cube;
    cube->SetXLength(1.2);
    cube->SetYLength(1.2);
    cube->SetZLength(1.2);
    cube->SetCenter(-0.9, 0.0, 0.6);
    vtkNew<vtkPolyDataMapper> cubeMapper;
    cubeMapper->SetInputConnection(cube->GetOutputPort());
    vtkNew<vtkActor> cubeActor;
    cubeActor->SetMapper(cubeMapper);
    cubeActor->GetProperty()->SetColor(0.294, 0.639, 0.780);
    renderer->AddActor(cubeActor);

    vtkNew<vtkSphereSource> sphere;
    sphere->SetRadius(0.55);
    sphere->SetCenter(0.95, 0.2, 0.55);
    sphere->SetThetaResolution(32);
    sphere->SetPhiResolution(32);
    vtkNew<vtkPolyDataMapper> sphereMapper;
    sphereMapper->SetInputConnection(sphere->GetOutputPort());
    vtkNew<vtkActor> sphereActor;
    sphereActor->SetMapper(sphereMapper);
    sphereActor->GetProperty()->SetColor(0.396, 0.749, 0.631);
    renderer->AddActor(sphereActor);

    vtkNew<vtkPlaneSource> grid;
    grid->SetOrigin(-4.0, -4.0, 0.0);
    grid->SetPoint1(4.0, -4.0, 0.0);
    grid->SetPoint2(-4.0, 4.0, 0.0);
    grid->SetResolution(8, 8);
    vtkNew<vtkPolyDataMapper> gridMapper;
    gridMapper->SetInputConnection(grid->GetOutputPort());
    vtkNew<vtkActor> gridActor;
    gridActor->SetMapper(gridMapper);
    gridActor->GetProperty()->SetRepresentationToWireframe();
    gridActor->GetProperty()->SetColor(0.227, 0.275, 0.322);
    renderer->AddActor(gridActor);

    vtkNew<vtkAxesActor> axes;
    axes->SetTotalLength(2.0, 2.0, 2.0);
    axes->AxisLabelsOff();
    renderer->AddActor(axes);

    vtkNew<vtkInteractorStyleTrackballCamera> style;
    widget->interactor()->SetInteractorStyle(style);
    static_cast<QVBoxLayout*>(layout())->addWidget(widget, 1);
    widget->hide();
    renderWidget_ = widget;
    QTimer::singleShot(0, widget, [this] { pushPoseToRenderer(); });
#else
    return;
#endif
}

void SampleViewport::pushPoseToRenderer() {
#ifdef MEDFORGE_HAS_VTK
    ensureScene();
    auto* widget = qobject_cast<QVTKOpenGLNativeWidget*>(renderWidget_);
    if (widget == nullptr || widget->renderWindow() == nullptr) {
        return;
    }
    auto* renderer = widget->renderWindow()->GetRenderers()->GetFirstRenderer();
    if (renderer == nullptr) {
        return;
    }
    vtkCamera* camera = renderer->GetActiveCamera();
    camera->SetPosition(pose_.position[0], pose_.position[1], pose_.position[2]);
    camera->SetFocalPoint(pose_.focalPoint[0], pose_.focalPoint[1], pose_.focalPoint[2]);
    camera->SetViewUp(pose_.viewUp[0], pose_.viewUp[1], pose_.viewUp[2]);
    renderer->ResetCameraClippingRange();
    if (widget->isVisible()) {
        widget->renderWindow()->Render();
    }
#endif
}

void SampleViewport::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    if (!sampleVisible_) {
        return;
    }
    QTimer::singleShot(0, this, [this] { pushPoseToRenderer(); });
}

medforge::rendering::CameraPose SampleViewport::pose() const {
    return pose_;
}

bool SampleViewport::sampleVisible() const {
    return sampleVisible_;
}

void SampleViewport::syncLabel() {
    label_->setText(sampleVisible_ ? QStringLiteral("Sample cube and sphere")
                                   : QStringLiteral("No sample geometry"));
}
