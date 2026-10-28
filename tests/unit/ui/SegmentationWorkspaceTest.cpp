#include "medforge/imaging/SyntheticPhantom.h"
#include "medforge/ui/FoundationWidgets.h"
#include "medforge/ui/MprViewport.h"
#include "medforge/ui/NumericEditor.h"
#include "medforge/ui/SegmentedControl.h"
#include "medforge/ui/WorkspaceActions.h"
#include "medforge/ui/WorkspaceWindow.h"

#include <QDir>
#include <QFile>
#include <QLabel>
#include <QtTest>
#include <filesystem>

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

} // namespace

class SegmentationWorkspaceTest : public QObject {
    Q_OBJECT

private slots:
    void thresholdUndoRedoAndReload();
    void paintAndEraseAreSeparateModes();
    void toolSettingsPersist();
    void growFromAirWarnsAndCancelKeepsTheMask();
    void labelMeshRecordsProvenance();
};

void SegmentationWorkspaceTest::thresholdUndoRedoAndReload() {
    const QString layout = QDir::temp().filePath(QStringLiteral("medforge-m4-threshold.ini"));
    QFile::remove(layout);
    QFile::remove(layout + QStringLiteral(".segmentation.ini"));
    WorkspaceWindow window(layout);
    window.importFolder(phantomFolder("medforge-m4-ui-threshold"));
    QTRY_COMPARE(window.actions()->phase(), WorkspaceActions::Phase::Study);
    auto* unsaved = window.findChild<QLabel*>(QStringLiteral("segmentUnsaved"));
    QVERIFY(unsaved != nullptr);
    QCOMPARE(unsaved->text(), QStringLiteral("Mask saved"));
    window.applySegmentThreshold();
    QCOMPARE(window.labelAt(medforge::imaging::phantom::kMarkerI, medforge::imaging::phantom::kMarkerJ,
                            medforge::imaging::phantom::kMarkerK),
             1);
    QCOMPARE(window.labelAt(0, 0, 0), 0);
    QCOMPARE(unsaved->text(), QStringLiteral("Unsaved mask"));
    QVERIFY(window.axialViewport()->grayAt(medforge::imaging::phantom::kMarkerI,
                                           medforge::imaging::phantom::kMarkerJ) != 255);
    window.undoSegment();
    QCOMPARE(window.labelAt(medforge::imaging::phantom::kMarkerI, medforge::imaging::phantom::kMarkerJ,
                            medforge::imaging::phantom::kMarkerK),
             0);
    QCOMPARE(unsaved->text(), QStringLiteral("Mask saved"));
    window.redoSegment();
    QCOMPARE(window.labelAt(medforge::imaging::phantom::kMarkerI, medforge::imaging::phantom::kMarkerJ,
                            medforge::imaging::phantom::kMarkerK),
             1);
    const QString mask = QDir::temp().filePath(QStringLiteral("medforge-m4-ui.mfmask"));
    QString error;
    QVERIFY2(window.saveMask(mask, &error), qPrintable(error));
    QCOMPARE(unsaved->text(), QStringLiteral("Mask saved"));
    window.undoSegment();
    QVERIFY2(window.loadMask(mask, &error), qPrintable(error));
    QCOMPARE(window.labelAt(medforge::imaging::phantom::kMarkerI, medforge::imaging::phantom::kMarkerJ,
                            medforge::imaging::phantom::kMarkerK),
             1);
    QCOMPARE(unsaved->text(), QStringLiteral("Mask saved"));
}

void SegmentationWorkspaceTest::paintAndEraseAreSeparateModes() {
    WorkspaceWindow window(QDir::temp().filePath(QStringLiteral("medforge-m4-paint.ini")));
    window.importFolder(phantomFolder("medforge-m4-ui-paint"));
    QTRY_COMPARE(window.actions()->phase(), WorkspaceActions::Phase::Study);
    auto* tool = window.findChild<SegmentedControl*>(QStringLiteral("segmentTool"));
    QVERIFY(tool != nullptr);
    QVERIFY(tool->setCurrentIndex(0));
    window.paintSegment(medforge::imaging::MprPlane::Axial, 0, 0);
    QCOMPARE(window.labelAt(0, 0, 2), 0);
    QVERIFY(tool->setCurrentIndex(2));
    window.paintSegment(medforge::imaging::MprPlane::Axial, 0, 0);
    QCOMPARE(window.labelAt(0, 0, 2), 1);
    QVERIFY(tool->setCurrentIndex(3));
    window.paintSegment(medforge::imaging::MprPlane::Axial, 0, 0);
    QCOMPARE(window.labelAt(0, 0, 2), 0);
}

void SegmentationWorkspaceTest::toolSettingsPersist() {
    const QString layout = QDir::temp().filePath(QStringLiteral("medforge-m4-tools.ini"));
    QFile::remove(layout + QStringLiteral(".segmentation.ini"));
    {
        WorkspaceWindow window(layout);
        auto* radius = window.findChild<NumericEditor*>(QStringLiteral("segmentRadius"));
        QVERIFY(radius != nullptr);
        radius->setValue(2.5);
        QCOMPARE(radius->value(), 2.5);
        auto* connectivity = window.findChild<SegmentedControl*>(QStringLiteral("segmentConnectivity"));
        QVERIFY(connectivity->setCurrentIndex(1));
        QVERIFY2(QFile::exists(layout + QStringLiteral(".segmentation.ini")),
                 qPrintable(layout + QStringLiteral(".segmentation.ini")));
    }
    WorkspaceWindow restored(layout);
    QCOMPARE(restored.findChild<NumericEditor*>(QStringLiteral("segmentRadius"))->value(), 2.5);
    QCOMPARE(restored.findChild<SegmentedControl*>(QStringLiteral("segmentConnectivity"))->currentIndex(),
             1);
}

void SegmentationWorkspaceTest::growFromAirWarnsAndCancelKeepsTheMask() {
    WorkspaceWindow window(QDir::temp().filePath(QStringLiteral("medforge-m4-grow.ini")));
    window.importFolder(phantomFolder("medforge-m4-ui-grow"));
    QTRY_COMPARE(window.actions()->phase(), WorkspaceActions::Phase::Study);
    window.applySegmentThreshold();
    const int marked = window.labelAt(medforge::imaging::phantom::kMarkerI,
                                      medforge::imaging::phantom::kMarkerJ,
                                      medforge::imaging::phantom::kMarkerK);
    QCOMPARE(marked, 1);
    window.growFromCrosshair(400);
    window.cancelSegment();
    QTRY_COMPARE(window.errorBanner()->message(), QStringLiteral("Segmentation cancelled"));
    QCOMPARE(window.labelAt(medforge::imaging::phantom::kMarkerI, medforge::imaging::phantom::kMarkerJ,
                            medforge::imaging::phantom::kMarkerK),
             1);
}

void SegmentationWorkspaceTest::labelMeshRecordsProvenance() {
    WorkspaceWindow window(QDir::temp().filePath(QStringLiteral("medforge-m4-mesh.ini")));
    window.importFolder(phantomFolder("medforge-m4-ui-mesh"));
    QTRY_COMPARE(window.actions()->phase(), WorkspaceActions::Phase::Study);
    window.applySegmentThreshold();
    window.buildLabelMesh();
    QTRY_VERIFY_WITH_TIMEOUT(window.meshStatsLabel()->text().contains(QStringLiteral("triangles")) ||
                                 window.errorBanner()->message().contains(QStringLiteral("VTK")) ||
                                 window.errorBanner()->message().contains(QStringLiteral("not built")),
                             5000);
    if (window.errorBanner()->message().contains(QStringLiteral("VTK")) ||
        window.errorBanner()->message().contains(QStringLiteral("not built"))) {
        QSKIP("VTK is not available");
    }
    QVERIFY(window.meshStatsLabel()->text().contains(QStringLiteral("triangles")));
    auto* provenance = window.findChild<QLabel*>(QStringLiteral("segmentProvenance"));
    QVERIFY(provenance->text().contains(QStringLiteral("Label 1")));
    QVERIFY(provenance->text().contains(QString::fromUtf8(medforge::imaging::phantom::kSeriesUid)));
}

QTEST_MAIN(SegmentationWorkspaceTest)
#include "SegmentationWorkspaceTest.moc"
