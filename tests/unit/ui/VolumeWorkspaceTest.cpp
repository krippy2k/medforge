#include "medforge/imaging/SyntheticPhantom.h"
#include "medforge/ui/FoundationWidgets.h"
#include "medforge/ui/SampleViewport.h"
#include "medforge/ui/NumericEditor.h"
#include "medforge/ui/SegmentedControl.h"
#include "medforge/ui/VolumeViewport.h"
#include "medforge/ui/WorkspaceActions.h"
#include "medforge/ui/WorkspaceWindow.h"

#include <QDir>
#include <QLabel>
#include <QPushButton>
#include <QtTest>
#include <filesystem>

#ifdef MEDFORGE_HAS_VTK
#include <vtkAutoInit.h>
#include <vtkColorTransferFunction.h>
#include <vtkFixedPointVolumeRayCastMapper.h>
#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkNew.h>
#include <vtkOutputWindow.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRayCastImageDisplayHelper.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkStringOutputWindow.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

VTK_AUTOINIT_DECLARE(vtkRenderingOpenGL2)
VTK_AUTOINIT_DECLARE(vtkInteractionStyle)
#endif

namespace {

std::filesystem::path tempFolder(const char* name) {
    const std::filesystem::path folder = std::filesystem::temp_directory_path() / name;
    std::filesystem::remove_all(folder);
    std::filesystem::create_directories(folder);
    return folder;
}

QString phantomFolder(const char* folderName) {
    const std::filesystem::path folder = tempFolder(folderName);
    std::string error;
    if (!medforge::imaging::phantom::writeSyntheticAxialSeries(folder, &error)) {
        qFatal("synthetic CT: %s", error.c_str());
    }
    return QString::fromStdString(folder.string());
}

bool vtkIsBuilt(const VolumeViewport& viewport) {
    return !viewport.renderStatus().contains(QStringLiteral("not built"));
}

} // namespace

class VolumeWorkspaceTest : public QObject {
    Q_OBJECT

private slots:
    void importShowsTheVolumeAndHidesTheSample();
    void volumeMapperOverrideIsRegistered();
    void cpuRayCastPaintsTheViewport();
    void presetsTogglesAndCameraUsePatientBounds();
    void surfaceSitsOnTheMarkerInPatientSpace();
    void clipRemovesTrianglesAboveTheCrosshair();
    void cancelKeepsThePreviousSurface();
    void reloadClearsTheSurface();
    void highThresholdWarnsAndClearsTheMesh();
};

void VolumeWorkspaceTest::volumeMapperOverrideIsRegistered() {
#ifndef MEDFORGE_HAS_VTK
    QSKIP("VTK is not available");
#else
    registerVolumeRayCastHelper();
    vtkSmartPointer<vtkRayCastImageDisplayHelper> helper =
        vtkSmartPointer<vtkRayCastImageDisplayHelper>::Take(vtkRayCastImageDisplayHelper::New());
    QVERIFY(helper != nullptr);
    QCOMPARE(QString::fromLatin1(helper->GetClassName()),
             QStringLiteral("MedForgeRayCastImageDisplayHelper"));
#endif
}

void VolumeWorkspaceTest::cpuRayCastPaintsTheViewport() {
#ifndef MEDFORGE_HAS_VTK
    QSKIP("VTK is not available");
#else
    vtkRenderingOpenGL2_AutoInit_Construct();
    vtkInteractionStyle_AutoInit_Construct();
    registerVolumeRayCastHelper();

    vtkSmartPointer<vtkOutputWindow> previous = vtkOutputWindow::GetInstance();
    vtkNew<vtkStringOutputWindow> capture;
    vtkOutputWindow::SetInstance(capture);

    vtkNew<vtkImageData> image;
    image->SetDimensions(24, 8, 8);
    image->AllocateScalars(VTK_SHORT, 1);
    auto* scalars = static_cast<short*>(image->GetScalarPointer());
    for (int k = 0; k < 8; ++k) {
        for (int j = 0; j < 8; ++j) {
            for (int i = 0; i < 24; ++i) {
                const bool marker = (i >= 1 && i < 5) || (i >= 19 && i < 23);
                scalars[(k * 8 + j) * 24 + i] = marker ? 800 : 0;
            }
        }
    }

    vtkNew<vtkFixedPointVolumeRayCastMapper> mapper;
    mapper->SetInputData(image);
    mapper->SetAutoAdjustSampleDistances(0);
    mapper->SetSampleDistance(0.5f);
    mapper->SetImageSampleDistance(1.0f);
    mapper->SetBlendModeToComposite();

    vtkNew<vtkColorTransferFunction> color;
    color->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
    color->AddRGBPoint(800.0, 1.0, 1.0, 1.0);
    vtkNew<vtkPiecewiseFunction> opacity;
    opacity->AddPoint(0.0, 0.0);
    opacity->AddPoint(800.0, 1.0);
    vtkNew<vtkVolumeProperty> property;
    property->SetColor(color);
    property->SetScalarOpacity(opacity);
    property->SetInterpolationTypeToLinear();
    property->SetScalarOpacityUnitDistance(1.0);
    property->ShadeOff();

    vtkNew<vtkVolume> volume;
    volume->SetMapper(mapper);
    volume->SetProperty(property);

    vtkNew<vtkRenderer> renderer;
    renderer->SetBackground(0.0, 0.0, 0.0);
    renderer->AddVolume(volume);
    vtkNew<vtkRenderWindow> window;
    window->SetOffScreenRendering(1);
    window->SetSize(96, 96);
    window->AddRenderer(renderer);
    renderer->ResetCamera();
    window->Render();
    renderer->ResetCamera();
    window->Render();

    vtkNew<vtkFloatArray> pixels;
    const int wrote = window->GetRGBAPixelData(0, 0, 95, 95, 1, pixels);
    const QString log = QString::fromStdString(capture->GetOutput());
    vtkOutputWindow::SetInstance(previous);

    QVERIFY2(wrote != 0, qPrintable(log));
    QVERIFY(pixels->GetNumberOfValues() >= 96 * 96 * 4);
    const float* data = pixels->GetPointer(0);
    auto energyIn = [data](int left, int right) {
        double sum = 0.0;
        for (int y = 16; y < 80; ++y) {
            for (int x = left; x < right; ++x) {
                const vtkIdType index = (static_cast<vtkIdType>(y) * 96 + x) * 4;
                sum += static_cast<double>(data[index] + data[index + 1] + data[index + 2]);
            }
        }
        return sum;
    };
    const double leftEnergy = energyIn(0, 48);
    const double rightEnergy = energyIn(48, 96);
    QVERIFY2(!log.contains(QStringLiteral("no override found")), qPrintable(log));
    QVERIFY2(leftEnergy > 100.0 && rightEnergy > leftEnergy * 0.5,
             qPrintable(QStringLiteral("left %1 right %2 %3")
                            .arg(leftEnergy)
                            .arg(rightEnergy)
                            .arg(log)));
#endif
}

void VolumeWorkspaceTest::importShowsTheVolumeAndHidesTheSample() {
    WorkspaceWindow window(QDir::temp().filePath(QStringLiteral("medforge-m3-import.ini")));
    window.importFolder(phantomFolder("medforge-m3-ui-import"));
    QTRY_COMPARE(window.actions()->phase(), WorkspaceActions::Phase::Study);
    QVERIFY(window.volumeViewport()->hasVolume());
    QVERIFY(window.volumeViewport()->isVisibleTo(&window) || !window.volumeViewport()->isHidden());
    QVERIFY(window.sampleViewport()->isHidden());
    QVERIFY(window.volumeViewport()->renderStatus().contains(QStringLiteral("ray casting")));
    QCOMPARE(window.volumeViewport()->transferName(), QStringLiteral("Soft"));
    QVERIFY(window.cursorReadout()->text().contains(QStringLiteral("L 13.20")));
    QVERIFY(window.cursorReadout()->text().contains(QStringLiteral("P -18.50")));
    QVERIFY(window.cursorReadout()->text().contains(QStringLiteral("S 4.00")));
    auto* note = window.findChild<QLabel*>(QStringLiteral("meshStats"));
    QVERIFY(note != nullptr);
    QCOMPARE(note->text(), QStringLiteral("No surface"));
    QVERIFY(window.findChild<QLabel*>(QStringLiteral("taskStatus"))->text().contains(
        QStringLiteral("surface extraction")));
}

void VolumeWorkspaceTest::presetsTogglesAndCameraUsePatientBounds() {
    WorkspaceWindow window(QDir::temp().filePath(QStringLiteral("medforge-m3-camera.ini")));
    window.importFolder(phantomFolder("medforge-m3-ui-camera"));
    QTRY_COMPARE(window.actions()->phase(), WorkspaceActions::Phase::Study);
    const medforge::rendering::CameraPose anterior = window.volumeViewport()->pose();
    QVERIFY(anterior.position[1] < anterior.focalPoint[1]);
    QCOMPARE(anterior.viewUp[2], 1.0);

    auto* cameras = window.findChild<SegmentedControl*>(QStringLiteral("cameraViews"));
    QVERIFY(cameras != nullptr);
    QVERIFY(cameras->setCurrentIndex(4));
    const medforge::rendering::CameraPose superior = window.volumeViewport()->pose();
    QVERIFY(superior.position[2] > superior.focalPoint[2]);
    QCOMPARE(superior.viewUp[1], -1.0);

    auto* presets = window.findChild<SegmentedControl*>(QStringLiteral("volumePresets"));
    QVERIFY(presets->setCurrentIndex(2));
    QCOMPARE(window.volumeViewport()->transferName(), QStringLiteral("Bone"));

    auto* volume = window.findChild<QPushButton*>(QStringLiteral("volumeVisible"));
    volume->setChecked(false);
    QVERIFY(!window.volumeViewport()->volumeVisible());
    volume->setChecked(true);
    QVERIFY(window.volumeViewport()->volumeVisible());
}

void VolumeWorkspaceTest::surfaceSitsOnTheMarkerInPatientSpace() {
    WorkspaceWindow window(QDir::temp().filePath(QStringLiteral("medforge-m3-surface.ini")));
    window.importFolder(phantomFolder("medforge-m3-ui-surface"));
    QTRY_COMPARE(window.actions()->phase(), WorkspaceActions::Phase::Study);
    if (!vtkIsBuilt(*window.volumeViewport())) {
        window.findChild<QPushButton*>(QStringLiteral("buildSurface"))->click();
        QTRY_VERIFY(window.errorBanner()->message().contains(QStringLiteral("VTK is not available")));
        return;
    }
    window.findChild<QPushButton*>(QStringLiteral("buildSurface"))->click();
    QTRY_VERIFY(window.volumeViewport()->meshTriangleCount() > 0);
    QVERIFY(window.meshStatsLabel()->text().contains(QStringLiteral("triangles")));
    QVERIFY(window.meshStatsLabel()->text().contains(QStringLiteral("MB")));
    QVERIFY(window.meshStatsLabel()->text().contains(QStringLiteral("ms")));
    auto* surfaceRow = window.findChild<QWidget*>(QStringLiteral("surfaceRow"));
    QVERIFY(surfaceRow != nullptr);
    QVERIFY(!surfaceRow->isHidden());

    medforge::core::Vec3 low;
    medforge::core::Vec3 high;
    QVERIFY(window.volumeViewport()->meshBounds(&low, &high));
    QVERIFY(low.x > 11.0 && high.x < 15.0);
    QVERIFY(low.y > -21.0 && high.y < -17.0);
    QVERIFY(low.z > 0.0 && high.z < 6.0);

    auto* mesh = window.findChild<QPushButton*>(QStringLiteral("meshVisible"));
    mesh->setChecked(false);
    QCOMPARE(window.volumeViewport()->meshTriangleCount(), 0);
    mesh->setChecked(true);
    QVERIFY(window.volumeViewport()->meshTriangleCount() > 0);
}

void VolumeWorkspaceTest::clipRemovesTrianglesAboveTheCrosshair() {
    WorkspaceWindow window(QDir::temp().filePath(QStringLiteral("medforge-m3-clip.ini")));
    window.importFolder(phantomFolder("medforge-m3-ui-clip"));
    QTRY_COMPARE(window.actions()->phase(), WorkspaceActions::Phase::Study);
    if (!vtkIsBuilt(*window.volumeViewport())) {
        QSKIP("VTK is not available");
    }
    auto* threshold = window.findChild<NumericEditor*>(QStringLiteral("isoThreshold"));
    threshold->setValue(0);
    window.buildSurface();
    QTRY_VERIFY(window.volumeViewport()->meshTriangleCount() > 0);
    const int before = window.volumeViewport()->meshTriangleCount();
    window.findChild<QPushButton*>(QStringLiteral("clipPlane"))->setChecked(true);
    const int after = window.volumeViewport()->meshTriangleCount();
    QVERIFY(after > 0);
    QVERIFY(after < before);
}

void VolumeWorkspaceTest::cancelKeepsThePreviousSurface() {
    WorkspaceWindow window(QDir::temp().filePath(QStringLiteral("medforge-m3-cancel.ini")));
    window.importFolder(phantomFolder("medforge-m3-ui-cancel"));
    QTRY_COMPARE(window.actions()->phase(), WorkspaceActions::Phase::Study);
    if (!vtkIsBuilt(*window.volumeViewport())) {
        QSKIP("VTK is not available");
    }
    window.buildSurface();
    QTRY_VERIFY(window.volumeViewport()->meshTriangleCount() > 0);
    const QString kept = window.meshStatsLabel()->text();
    const int triangles = window.volumeViewport()->meshTriangleCount();

    window.buildSurface(400);
    auto* progress = window.findChild<TaskProgress*>(QStringLiteral("importProgress"));
    QVERIFY(progress->isRunning());
    QCOMPARE(window.findChild<QLabel*>(QStringLiteral("taskStatus"))->text(),
             QStringLiteral("Building surface"));
    window.cancelSurface();
    QTRY_VERIFY(window.errorBanner()->message().contains(QStringLiteral("cancelled")));
    QVERIFY(!progress->isRunning());
    QCOMPARE(window.volumeViewport()->meshTriangleCount(), triangles);
    QCOMPARE(window.meshStatsLabel()->text(), kept);
}

void VolumeWorkspaceTest::reloadClearsTheSurface() {
    const std::filesystem::path folder = tempFolder("medforge-m3-ui-reload");
    std::string error;
    QVERIFY(medforge::imaging::phantom::writeSyntheticAxialSeries(folder, &error));
    WorkspaceWindow window(QDir::temp().filePath(QStringLiteral("medforge-m3-reload.ini")));
    const QString path = QString::fromStdString(folder.string());
    window.importFolder(path);
    QTRY_COMPARE(window.actions()->phase(), WorkspaceActions::Phase::Study);
    if (!vtkIsBuilt(*window.volumeViewport())) {
        QSKIP("VTK is not available");
    }
    window.buildSurface(600);
    QVERIFY(window.findChild<TaskProgress*>(QStringLiteral("importProgress"))->isRunning());
    window.importFolder(path);
    QTRY_COMPARE(window.actions()->phase(), WorkspaceActions::Phase::Study);
    QCOMPARE(window.meshStatsLabel()->text(), QStringLiteral("No surface"));
    QCOMPARE(window.volumeViewport()->meshTriangleCount(), 0);
    QVERIFY(window.volumeViewport()->hasVolume());
    QVERIFY(window.sampleViewport()->isHidden());
}

void VolumeWorkspaceTest::highThresholdWarnsAndClearsTheMesh() {
    WorkspaceWindow window(QDir::temp().filePath(QStringLiteral("medforge-m3-empty.ini")));
    window.importFolder(phantomFolder("medforge-m3-ui-empty"));
    QTRY_COMPARE(window.actions()->phase(), WorkspaceActions::Phase::Study);
    if (!vtkIsBuilt(*window.volumeViewport())) {
        QSKIP("VTK is not available");
    }
    window.buildSurface();
    QTRY_VERIFY(window.volumeViewport()->meshTriangleCount() > 0);
    window.findChild<NumericEditor*>(QStringLiteral("isoThreshold"))->setValue(3000);
    window.buildSurface();
    QTRY_COMPARE(window.errorBanner()->message(), QStringLiteral("No surface at this threshold"));
    QCOMPARE(window.errorBanner()->severityText(), QStringLiteral("Warning"));
    QCOMPARE(window.volumeViewport()->meshTriangleCount(), 0);
    QCOMPARE(window.meshStatsLabel()->text(), QStringLiteral("No surface"));
}

QTEST_MAIN(VolumeWorkspaceTest)
#include "VolumeWorkspaceTest.moc"
