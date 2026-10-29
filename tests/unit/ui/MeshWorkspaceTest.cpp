#include "medforge/imaging/SyntheticPhantom.h"
#include "medforge/ui/FoundationWidgets.h"
#include "medforge/ui/NumericEditor.h"
#include "medforge/ui/SegmentedControl.h"
#include "medforge/ui/VolumeViewport.h"
#include "medforge/ui/WorkspaceActions.h"
#include "medforge/ui/WorkspaceWindow.h"

#include <QDir>
#include <QFile>
#include <QLabel>
#include <QPushButton>
#include <QtTest>
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

class MeshWorkspaceTest : public QObject {
    Q_OBJECT

private slots:
    void previewCancelAndUndoDoNotReplaceTheSource();
    void largeHoleRefusalAndExportStayInMillimeters();
    void cancellingTheRepairKeepsTheCommittedMesh();
};

void MeshWorkspaceTest::previewCancelAndUndoDoNotReplaceTheSource() {
    const QString layout = QDir::temp().filePath(QStringLiteral("medforge-m6-preview.ini"));
    QFile::remove(layout);
    WorkspaceWindow window(layout);
    window.importFolder(phantomFolder("medforge-m6-ui-preview"));
    QTRY_COMPARE(window.actions()->phase(), WorkspaceActions::Phase::Study);
    window.findChild<SegmentedControl*>(QStringLiteral("meshRepair"))->setCurrentIndex(0);
    window.previewMeshRepair();
    QVERIFY(!window.meshPreviewActive());
    QCOMPARE(window.committedTriangleCount(), 0);

    window.buildSurface();
    QTRY_VERIFY_WITH_TIMEOUT(window.volumeViewport()->meshTriangleCount() > 0, 5000);
    const int committed = window.committedTriangleCount();
    const QString before = window.meshDiagnostics();
    QVERIFY(before.contains(QStringLiteral("millimeters")));
    QVERIFY(window.findChild<QLabel*>(QStringLiteral("meshDiagnostics"))->text().contains(
        QStringLiteral("millimeters")));

    window.findChild<QPushButton*>(QStringLiteral("previewMesh"))->click();
    QTRY_VERIFY(window.meshPreviewActive());
    QCOMPARE(window.committedTriangleCount(), committed);
    QVERIFY(window.meshDiagnostics().contains(QStringLiteral("Preview")));
    window.findChild<QPushButton*>(QStringLiteral("cancelMeshPreview"))->click();
    QVERIFY(!window.meshPreviewActive());
    QCOMPARE(window.meshDiagnostics(), before);

    window.previewMeshRepair();
    QTRY_VERIFY(window.meshPreviewActive());
    window.findChild<QPushButton*>(QStringLiteral("applyMesh"))->click();
    QVERIFY(!window.meshPreviewActive());
    QVERIFY(window.meshDiagnostics() != before);
    window.findChild<QPushButton*>(QStringLiteral("undoMesh"))->click();
    QCOMPARE(window.meshDiagnostics(), before);
    window.findChild<QPushButton*>(QStringLiteral("redoMesh"))->click();
    QVERIFY(window.meshDiagnostics() != before);
    window.undoMesh();
    QCOMPARE(window.committedTriangleCount(), committed);
}

void MeshWorkspaceTest::largeHoleRefusalAndExportStayInMillimeters() {
    const QString layout = QDir::temp().filePath(QStringLiteral("medforge-m6-export.ini"));
    QFile::remove(layout);
    WorkspaceWindow window(layout);
    QString error;
    QVERIFY(!window.exportBinaryStl(QDir::temp().filePath(QStringLiteral("medforge-m6-empty.stl")),
                                    &error));
    QVERIFY(error.contains(QStringLiteral("surface")));

    window.importFolder(phantomFolder("medforge-m6-ui-export"));
    QTRY_COMPARE(window.actions()->phase(), WorkspaceActions::Phase::Study);
    window.buildSurface();
    QTRY_VERIFY_WITH_TIMEOUT(window.committedTriangleCount() > 0, 5000);
    const int committed = window.committedTriangleCount();
    window.findChild<SegmentedControl*>(QStringLiteral("meshRepair"))->setCurrentIndex(2);
    window.findChild<NumericEditor*>(QStringLiteral("holeEdges"))->setValue(3);
    window.previewMeshRepair();
    QTRY_VERIFY(window.meshPreviewActive() ||
                window.errorBanner()->message().contains(QStringLiteral("hole"), Qt::CaseInsensitive));
    if (window.meshPreviewActive()) {
        window.cancelMeshPreview();
    }
    QCOMPARE(window.committedTriangleCount(), committed);
    QVERIFY(!window.meshPreviewActive());

    window.findChild<SegmentedControl*>(QStringLiteral("meshRepair"))->setCurrentIndex(0);
    window.previewMeshRepair();
    QTRY_VERIFY(window.meshPreviewActive());
    error.clear();
    QVERIFY(!window.exportBinaryStl(QDir::temp().filePath(QStringLiteral("medforge-m6-preview.stl")),
                                    &error));
    QVERIFY(error.contains(QStringLiteral("preview")));
    window.cancelMeshPreview();

    const QString path = QDir::temp().filePath(QStringLiteral("medforge-m6-bone.stl"));
    QVERIFY2(window.exportBinaryStl(path, &error), qPrintable(error));
    QVERIFY(QFile::exists(path));
    QVERIFY(window.errorBanner()->message().contains(QStringLiteral("millimeters")));
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QVERIFY(file.read(80).contains("millimeters"));
}

void MeshWorkspaceTest::cancellingTheRepairKeepsTheCommittedMesh() {
    const QString layout = QDir::temp().filePath(QStringLiteral("medforge-m6-cancel.ini"));
    QFile::remove(layout);
    WorkspaceWindow window(layout);
    window.importFolder(phantomFolder("medforge-m6-ui-cancel"));
    QTRY_COMPARE(window.actions()->phase(), WorkspaceActions::Phase::Study);
    window.buildSurface();
    QTRY_VERIFY_WITH_TIMEOUT(window.committedTriangleCount() > 0, 5000);
    const int committed = window.committedTriangleCount();
    window.findChild<SegmentedControl*>(QStringLiteral("meshRepair"))->setCurrentIndex(1);
    window.previewMeshRepair(400);
    QVERIFY(window.findChild<TaskProgress*>(QStringLiteral("importProgress"))->isRunning());
    window.cancelMeshOperation();
    QTRY_VERIFY(window.errorBanner()->message().contains(QStringLiteral("cancelled")));
    QVERIFY(!window.meshPreviewActive());
    QCOMPARE(window.committedTriangleCount(), committed);
}

QTEST_MAIN(MeshWorkspaceTest)
#include "MeshWorkspaceTest.moc"
