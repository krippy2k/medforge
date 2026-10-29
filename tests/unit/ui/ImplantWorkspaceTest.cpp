#include "medforge/cad/Kernel.h"
#include "medforge/imaging/SyntheticPhantom.h"
#include "medforge/ui/FoundationWidgets.h"
#include "medforge/ui/NumericEditor.h"
#include "medforge/ui/VolumeViewport.h"
#include "medforge/ui/WorkspaceActions.h"
#include "medforge/ui/WorkspaceWindow.h"

#include <QDir>
#include <QFile>
#include <QLabel>
#include <QtTest>

#include <cmath>
#include <filesystem>

namespace {

QString phantomFolder(const char* name) {
    const std::filesystem::path folder = std::filesystem::temp_directory_path() / name;
    std::filesystem::remove_all(folder);
    std::filesystem::create_directories(folder);
    std::string error;
    if (!medforge::imaging::phantom::writeSyntheticAxialSeries(folder, &error)) {
        qFatal("synthetic CT: %s", error.c_str());
    }
    return QString::fromStdString(folder.string());
}

} // namespace

class ImplantWorkspaceTest : public QObject {
    Q_OBJECT

private slots:
    void init() {
        if (!medforge::cad::available()) {
            QSKIP("Open CASCADE is not linked");
        }
    }

    void gizmoAndNumericPoseReportClearance();
    void guidePreviewApplyRevertAndExport();
    void invalidParametersAndMeshFallback();
};

void ImplantWorkspaceTest::gizmoAndNumericPoseReportClearance() {
    const QString layout = QDir::temp().filePath(QStringLiteral("medforge-m7-pose.ini"));
    QFile::remove(layout);
    WorkspaceWindow window(layout);
    window.importFolder(phantomFolder("medforge-m7-ui-pose"));
    QTRY_COMPARE(window.actions()->phase(), WorkspaceActions::Phase::Study);
    QVERIFY(window.placeCylinderImplant());
    QVERIFY(window.implantStatus().contains(QStringLiteral("BRepExtrema")));
    QVERIFY(window.implantStatus().contains(QStringLiteral("Tolerance")));
    QVERIFY(window.implantStatus().contains(QStringLiteral("No overlap")));
    QVERIFY(window.findChild<QLabel*>(QStringLiteral("implantNotice"))
                ->text()
                .contains(QStringLiteral("Not for manufacture")));

    window.resize(1280, 800);
    window.show();
    QTest::qWait(20);
    VolumeViewport* view = window.volumeViewport();
    QCOMPARE(view->gizmoAxisAt(view->gizmoTip(0)), 0);
    QVERIFY(std::abs(view->gizmoMillimeters(0, view->gizmoAnchor(), view->gizmoTip(0)) - 12.0) < 0.75);

    auto* alongL = window.findChild<NumericEditor*>(QStringLiteral("implantL"));
    const double before = alongL->value();
    window.dragImplantGizmo(0, 2);
    QVERIFY(std::abs(alongL->value() - (before + 2)) < 1e-6);

    alongL->setValue(14);
    QTRY_VERIFY(window.implantStatus().contains(QStringLiteral("Overlap")));
    QVERIFY(window.errorBanner()->message().contains(QStringLiteral("intersects the anatomy")));
}

void ImplantWorkspaceTest::guidePreviewApplyRevertAndExport() {
    const QString layout = QDir::temp().filePath(QStringLiteral("medforge-m7-guide.ini"));
    QFile::remove(layout);
    WorkspaceWindow window(layout);
    window.importFolder(phantomFolder("medforge-m7-ui-guide"));
    QTRY_COMPARE(window.actions()->phase(), WorkspaceActions::Phase::Study);
    QVERIFY(window.placeCylinderImplant());

    const QString stepPath = QDir::temp().filePath(QStringLiteral("medforge-m7-guide.step"));
    QString error;
    QVERIFY(window.previewGuide());
    QVERIFY(window.guidePreviewActive());
    QVERIFY(window.guideStatus().contains(QStringLiteral("Preview")));
    QVERIFY(!window.exportPrototype(stepPath, &error));
    QVERIFY(error.contains(QStringLiteral("preview")));

    window.applyGuide();
    QVERIFY(!window.guidePreviewActive());
    QVERIFY(window.guideStatus().contains(QStringLiteral("Not for manufacture")));
    QVERIFY(window.exportPrototype(stepPath, &error));
    QVERIFY(window.errorBanner()->message().contains(QStringLiteral("Not for manufacture")));
    QFile file(stepPath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QVERIFY(file.readAll().contains("Not for manufacture"));
    file.close();

    const medforge::cad::Shape reopened = medforge::cad::readStep(stepPath.toStdString(), nullptr);
    QVERIFY(reopened.isValidSolid());
    QVERIFY(std::abs(reopened.volumeMm3() - window.guideVolume()) < 0.05);

    window.revertGuide();
    QVERIFY(!window.guidePreviewActive());
    QCOMPARE(window.guideVolume(), 0);
    QVERIFY(window.guideStatus().contains(QStringLiteral("No guide")));
}

void ImplantWorkspaceTest::invalidParametersAndMeshFallback() {
    const QString layout = QDir::temp().filePath(QStringLiteral("medforge-m7-warn.ini"));
    QFile::remove(layout);
    WorkspaceWindow window(layout);
    const QString stlPath = QDir::temp().filePath(QStringLiteral("medforge-m7-fallback.stl"));
    QString error;
    QVERIFY(!window.exportPrototype(stlPath, &error));
    QVERIFY(error.contains(QStringLiteral("implant")) || error.contains(QStringLiteral("surface")));

    window.importFolder(phantomFolder("medforge-m7-ui-warn"));
    QTRY_COMPARE(window.actions()->phase(), WorkspaceActions::Phase::Study);
    window.findChild<NumericEditor*>(QStringLiteral("isoThreshold"))->setValue(0);
    window.buildSurface();
    QTRY_VERIFY_WITH_TIMEOUT(window.committedTriangleCount() > 0, 5000);
    QVERIFY(window.exportPrototype(stlPath, &error));
    QVERIFY(window.errorBanner()->message().contains(QStringLiteral("not an exact STEP")));
    QVERIFY(window.errorBanner()->message().contains(QStringLiteral("Not for manufacture")));
    QFile stl(stlPath);
    QVERIFY(stl.open(QIODevice::ReadOnly));
    const QByteArray header = stl.read(80);
    QVERIFY(header.contains("Not for manufacture"));
    QVERIFY(header.contains("millimeters"));
    stl.close();

    auto* offset = window.findChild<NumericEditor*>(QStringLiteral("guideOffset"));
    offset->setValue(-1);
    QVERIFY(!offset->isValid());
    QVERIFY(!window.previewGuide());
    QVERIFY(window.errorBanner()->message().contains(QStringLiteral("Value must be between")));
    offset->setValue(4);
    auto* hole = window.findChild<NumericEditor*>(QStringLiteral("guideHole"));
    hole->setValue(20);
    QVERIFY(!window.previewGuide());
    QVERIFY(window.errorBanner()->message().contains(QStringLiteral("hole"), Qt::CaseInsensitive));

    const QString junk = QDir::temp().filePath(QStringLiteral("medforge-m7-junk.step"));
    QFile junkFile(junk);
    QVERIFY(junkFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
    junkFile.write("not a cad file");
    junkFile.close();
    QVERIFY(!window.importImplantStep(junk, &error));
    QVERIFY(error.contains(QStringLiteral("STEP")));
}

QTEST_MAIN(ImplantWorkspaceTest)
#include "ImplantWorkspaceTest.moc"
