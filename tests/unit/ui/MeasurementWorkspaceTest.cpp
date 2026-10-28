#include "medforge/imaging/SyntheticPhantom.h"
#include "medforge/ui/MprViewport.h"
#include "medforge/ui/NumericEditor.h"
#include "medforge/ui/SegmentedControl.h"
#include "medforge/ui/VolumeViewport.h"
#include "medforge/ui/WorkspaceActions.h"
#include "medforge/ui/WorkspaceWindow.h"

#include <QDir>
#include <QFile>
#include <QLabel>
#include <QMetaObject>
#include <QPushButton>
#include <QTableWidget>
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

class MeasurementWorkspaceTest : public QObject {
    Q_OBJECT

private slots:
    void placeDistanceUndoDeleteAndExport();
    void reloadRestoresDerivedMillimeters();
    void movingALandmarkRecalculates();
    void angleAndAreaToolsReportDegreesAndSquareMillimeters();
    void areaRecalculatesWhenTheSurfaceAppears();
};

bool clickPlane(MprViewport* view, int x, int y) {
    return QMetaObject::invokeMethod(view, "imageClicked", Q_ARG(int, x), Q_ARG(int, y));
}

void MeasurementWorkspaceTest::placeDistanceUndoDeleteAndExport() {
    const QString layout = QDir::temp().filePath(QStringLiteral("medforge-m5-measure.ini"));
    QFile::remove(layout);
    WorkspaceWindow window(layout);
    window.importFolder(phantomFolder("medforge-m5-ui-measure"));
    QTRY_COMPARE(window.actions()->phase(), WorkspaceActions::Phase::Study);
    auto* tool = window.findChild<SegmentedControl*>(QStringLiteral("measureTool"));
    QVERIFY(tool != nullptr);
    window.activateMeasureAtCrosshair();
    QCOMPARE(window.landmarkCount(), 0);
    QVERIFY(tool->setCurrentIndex(1));
    window.activateMeasureAtCrosshair();
    QCOMPARE(window.landmarkCount(), 1);
    QCOMPARE(window.placeLandmarkAtVoxel(0, 0, 0), 1);
    QCOMPARE(window.defineDistance(0, 1), 0);
    QCOMPARE(window.measureUnit(0), QStringLiteral("mm"));
    QVERIFY(window.measureValue(0) > 0);
    auto* table = window.findChild<QTableWidget*>(QStringLiteral("measureTable"));
    QVERIFY(table != nullptr);
    QCOMPARE(table->rowCount(), 1);
    const QString report = QDir::temp().filePath(QStringLiteral("medforge-m5-report.txt"));
    QString error;
    QVERIFY2(window.exportMeasureReport(report, &error), qPrintable(error));
    QFile file(report);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QString text = QString::fromUtf8(file.readAll());
    QVERIFY(text.contains(QStringLiteral("not for clinical use")));
    QVERIFY(text.contains(QStringLiteral("Not a validated measurement")));
    QVERIFY(text.contains(QStringLiteral("mm")));
    QVERIFY(text.contains(QString::fromUtf8(medforge::imaging::phantom::kSeriesUid)));
    QFile csv(report + QStringLiteral(".csv"));
    QVERIFY(csv.open(QIODevice::ReadOnly));
    QVERIFY(QString::fromUtf8(csv.readAll()).contains(QStringLiteral("mm")));
    window.findChild<QPushButton*>(QStringLiteral("undoMeasure"))->click();
    QCOMPARE(table->rowCount(), 0);
    window.findChild<QPushButton*>(QStringLiteral("redoMeasure"))->click();
    QCOMPARE(table->rowCount(), 1);
    window.findChild<QPushButton*>(QStringLiteral("undoMeasure"))->click();
    QCOMPARE(table->rowCount(), 0);
    window.selectLandmark(0);
    window.deleteSelectedLandmark();
    QCOMPARE(window.landmarkCount(), 1);
}

void MeasurementWorkspaceTest::reloadRestoresDerivedMillimeters() {
    const QString layout = QDir::temp().filePath(QStringLiteral("medforge-m5-reload.ini"));
    QFile::remove(layout);
    const QString folder = phantomFolder("medforge-m5-ui-reload");
    const QString path = QDir::temp().filePath(QStringLiteral("medforge-m5-document.txt"));
    {
        WorkspaceWindow window(layout);
        window.importFolder(folder);
        QTRY_COMPARE(window.actions()->phase(), WorkspaceActions::Phase::Study);
        window.placeLandmarkAtVoxel(0, 0, 0);
        window.placeLandmarkAtVoxel(4, 0, 0);
        QCOMPARE(window.defineDistance(0, 1), 0);
        QString error;
        QVERIFY2(window.saveMeasures(path, &error), qPrintable(error));
    }
    WorkspaceWindow restored(layout);
    restored.importFolder(folder);
    QTRY_COMPARE(restored.actions()->phase(), WorkspaceActions::Phase::Study);
    QString error;
    QVERIFY2(restored.loadMeasures(path, &error), qPrintable(error));
    QCOMPARE(restored.landmarkCount(), 2);
    QCOMPARE(restored.measureUnit(0), QStringLiteral("mm"));
    QVERIFY(std::abs(restored.measureValue(0) - 3.2) < 1e-6);
}

void MeasurementWorkspaceTest::movingALandmarkRecalculates() {
    const QString layout = QDir::temp().filePath(QStringLiteral("medforge-m5-move.ini"));
    QFile::remove(layout);
    WorkspaceWindow window(layout);
    window.importFolder(phantomFolder("medforge-m5-ui-move"));
    QTRY_COMPARE(window.actions()->phase(), WorkspaceActions::Phase::Study);
    window.placeLandmarkAtVoxel(0, 0, 0);
    window.placeLandmarkAtVoxel(4, 0, 0);
    window.defineDistance(0, 1);
    window.selectLandmark(1);
    auto* alongL = window.findChild<NumericEditor*>(QStringLiteral("measureL"));
    QVERIFY(alongL != nullptr);
    alongL->setValue(alongL->value() + 0.8);
    QVERIFY(std::abs(window.measureValue(0) - 4.0) < 1e-4);
}

void MeasurementWorkspaceTest::angleAndAreaToolsReportDegreesAndSquareMillimeters() {
    const QString layout = QDir::temp().filePath(QStringLiteral("medforge-m5-angle.ini"));
    QFile::remove(layout);
    WorkspaceWindow window(layout);
    window.importFolder(phantomFolder("medforge-m5-ui-angle"));
    QTRY_COMPARE(window.actions()->phase(), WorkspaceActions::Phase::Study);
    auto* tool = window.findChild<SegmentedControl*>(QStringLiteral("measureTool"));
    QVERIFY(tool->setCurrentIndex(1));
    QVERIFY(clickPlane(window.axialViewport(), 4, 3));
    QVERIFY(clickPlane(window.axialViewport(), 0, 3));
    QVERIFY(clickPlane(window.axialViewport(), 4, 0));
    QCOMPARE(window.landmarkCount(), 3);
    QVERIFY(tool->setCurrentIndex(2));
    QVERIFY(clickPlane(window.axialViewport(), 4, 3));
    QCOMPARE(window.measureValue(0), 0);
    QVERIFY(tool->setCurrentIndex(3));
    QVERIFY(clickPlane(window.axialViewport(), 0, 3));
    QVERIFY(clickPlane(window.axialViewport(), 4, 3));
    QVERIFY(clickPlane(window.axialViewport(), 4, 0));
    QCOMPARE(window.measureUnit(0), QStringLiteral("deg"));
    QVERIFY(std::abs(window.measureValue(0) - 90.0) < 1e-3);
    auto* table = window.findChild<QTableWidget*>(QStringLiteral("measureTable"));
    QCOMPARE(table->item(0, 1)->text(), QStringLiteral("Angle"));
    QVERIFY(tool->setCurrentIndex(4));
    QVERIFY(clickPlane(window.axialViewport(), 0, 3));
    QVERIFY(clickPlane(window.axialViewport(), 4, 3));
    QVERIFY(clickPlane(window.axialViewport(), 4, 0));
    QCOMPARE(window.measureUnit(1), QStringLiteral("mm^2"));
    QCOMPARE(table->item(1, 2)->text(), QStringLiteral("No surface"));
}

void MeasurementWorkspaceTest::areaRecalculatesWhenTheSurfaceAppears() {
    const QString layout = QDir::temp().filePath(QStringLiteral("medforge-m5-area.ini"));
    QFile::remove(layout);
    WorkspaceWindow window(layout);
    window.importFolder(phantomFolder("medforge-m5-ui-area"));
    QTRY_COMPARE(window.actions()->phase(), WorkspaceActions::Phase::Study);
    QCOMPARE(window.placeLandmarkAtVoxel(3, 2, 1), 0);
    QCOMPARE(window.placeLandmarkAtVoxel(3, 2, 0), 1);
    QCOMPARE(window.placeLandmarkAtVoxel(3, 5, 1), 2);
    auto* tool = window.findChild<SegmentedControl*>(QStringLiteral("measureTool"));
    QVERIFY(tool->setCurrentIndex(4));
    QVERIFY(clickPlane(window.coronalViewport(), 3, 0));
    QVERIFY(clickPlane(window.coronalViewport(), 3, 1));
    QVERIFY(clickPlane(window.sagittalViewport(), 5, 1));
    auto* table = window.findChild<QTableWidget*>(QStringLiteral("measureTable"));
    QCOMPARE(table->rowCount(), 1);
    QCOMPARE(table->item(0, 2)->text(), QStringLiteral("No surface"));
    window.buildSurface();
    QTRY_VERIFY_WITH_TIMEOUT(window.volumeViewport()->meshTriangleCount() > 0, 5000);
    QVERIFY(window.measureValue(0) > 0);
    QCOMPARE(window.measureUnit(0), QStringLiteral("mm^2"));
    QVERIFY(table->item(0, 2)->text() != QStringLiteral("No surface"));
}

QTEST_MAIN(MeasurementWorkspaceTest)
#include "MeasurementWorkspaceTest.moc"
