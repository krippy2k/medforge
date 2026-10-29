#include "medforge/geometry/MeshProcess.h"
#include "medforge/imaging/SyntheticPhantom.h"
#include "medforge/ui/FoundationWidgets.h"
#include "medforge/ui/NumericEditor.h"
#include "medforge/ui/WorkspaceActions.h"
#include "medforge/ui/WorkspaceWindow.h"

#include <QAction>
#include <QDir>
#include <QFile>
#include <QLineEdit>
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

class ProjectWorkspaceTest : public QObject {
    Q_OBJECT

private slots:
    void saveWithoutStudyIsRefused();
    void projectRoundTripRestoresMaskMeasuresAndExport();
    void missingSeriesCanBeRetried();
    void checksumMismatchRefusesTheStudy();
    void closingDuringImportJoinsTheWorker();
    void projectActionsAreKeyboardAccessible();
};

void ProjectWorkspaceTest::saveWithoutStudyIsRefused() {
    const QString layout = QDir::temp().filePath(QStringLiteral("medforge-m8-empty.ini"));
    QFile::remove(layout);
    WorkspaceWindow window(layout);
    QString error;
    QVERIFY(!window.saveProject(
        QDir::temp().filePath(QStringLiteral("medforge-m8-empty.mfproject")), &error));
    QVERIFY(error.contains(QStringLiteral("Import")));
}

void ProjectWorkspaceTest::projectRoundTripRestoresMaskMeasuresAndExport() {
    const QString series = phantomFolder("medforge-m8-roundtrip-ct");
    const QString layout = QDir::temp().filePath(QStringLiteral("medforge-m8-roundtrip.ini"));
    const QString project =
        QDir::temp().filePath(QStringLiteral("medforge-m8-roundtrip.mfproject"));
    const QString stl = QDir::temp().filePath(QStringLiteral("medforge-m8-roundtrip.stl"));
    QFile::remove(layout);
    QFile::remove(project);
    QFile::remove(stl);

    WorkspaceWindow window(layout);
    window.importFolder(series);
    QTRY_COMPARE(window.actions()->phase(), WorkspaceActions::Phase::Study);
    window.findChild<NumericEditor*>(QStringLiteral("windowWidth"))->setValue(1600);
    window.findChild<NumericEditor*>(QStringLiteral("isoThreshold"))->setValue(0);
    window.applySegmentThreshold();
    QCOMPARE(window.labelAt(3, 2, 1), 1);
    QCOMPARE(window.placeLandmarkAtVoxel(0, 0, 0), 0);
    QCOMPARE(window.placeLandmarkAtVoxel(4, 0, 0), 1);
    QCOMPARE(window.defineDistance(0, 1), 0);
    QVERIFY(std::abs(window.measureValue(0) - 3.2) < 1e-6);

    window.buildSurface();
    QTRY_VERIFY_WITH_TIMEOUT(window.committedTriangleCount() > 0 ||
                                 window.errorBanner()->message().contains(QStringLiteral("VTK")),
                             20000);
    const int triangles = window.committedTriangleCount();
    if (triangles > 0) {
        QString exportError;
        QVERIFY2(window.exportBinaryStl(stl, &exportError), qPrintable(exportError));
    }

    QString saveError;
    QVERIFY2(window.saveProject(project, &saveError), qPrintable(saveError));
    QVERIFY(window.errorBanner()->message().contains(QStringLiteral("Not for clinical use")));

    const QString reopenedLayout = QDir::temp().filePath(QStringLiteral("medforge-m8-reopen.ini"));
    QFile::remove(reopenedLayout);
    WorkspaceWindow reopened(reopenedLayout);
    QString openError;
    QVERIFY2(reopened.openProject(project, &openError), qPrintable(openError));
    QTRY_COMPARE(reopened.actions()->phase(), WorkspaceActions::Phase::Study);
    QCOMPARE(reopened.findChild<NumericEditor*>(QStringLiteral("windowWidth"))->value(), 1600.0);
    QCOMPARE(reopened.findChild<NumericEditor*>(QStringLiteral("isoThreshold"))->value(), 0.0);
    QCOMPARE(reopened.labelAt(3, 2, 1), 1);
    QCOMPARE(reopened.landmarkCount(), 2);
    QVERIFY(std::abs(reopened.measureValue(0) - 3.2) < 1e-6);
    QVERIFY(reopened.errorBanner()->message().contains(QStringLiteral("Not for clinical use")));

    if (triangles > 0) {
        reopened.buildSurface();
        QTRY_COMPARE_WITH_TIMEOUT(reopened.committedTriangleCount(), triangles, 20000);
        const QString again =
            QDir::temp().filePath(QStringLiteral("medforge-m8-roundtrip-again.stl"));
        QFile::remove(again);
        QString exportError;
        QVERIFY2(reopened.exportBinaryStl(again, &exportError), qPrintable(exportError));
        medforge::geometry::TriangleMesh mesh;
        std::string readError;
        QVERIFY(medforge::geometry::readBinaryStl(again.toStdString(), &mesh, &readError));
        QCOMPARE(static_cast<int>(mesh.indices.size() / 3), triangles);
    }
}

void ProjectWorkspaceTest::missingSeriesCanBeRetried() {
    const QString series = phantomFolder("medforge-m8-retry-ct");
    const QString layout = QDir::temp().filePath(QStringLiteral("medforge-m8-retry.ini"));
    const QString project = QDir::temp().filePath(QStringLiteral("medforge-m8-retry.mfproject"));
    QFile::remove(layout);
    QFile::remove(project);
    WorkspaceWindow window(layout);
    window.importFolder(series);
    QTRY_COMPARE(window.actions()->phase(), WorkspaceActions::Phase::Study);
    window.placeLandmarkAtVoxel(1, 1, 1);
    QString saveError;
    QVERIFY2(window.saveProject(project, &saveError), qPrintable(saveError));
    std::filesystem::remove_all(series.toStdString());

    const QString missingLayout =
        QDir::temp().filePath(QStringLiteral("medforge-m8-retry-missing.ini"));
    QFile::remove(missingLayout);
    WorkspaceWindow missing(missingLayout);
    QString openError;
    QVERIFY(!missing.openProject(project, &openError));
    QVERIFY(openError.contains(QStringLiteral("series")));
    QCOMPARE(missing.actions()->phase(), WorkspaceActions::Phase::Empty);

    std::string phantomError;
    QVERIFY(
        medforge::imaging::phantom::writeSyntheticAxialSeries(series.toStdString(), &phantomError));
    QVERIFY2(missing.openProject(project, &openError), qPrintable(openError));
    QTRY_COMPARE(missing.actions()->phase(), WorkspaceActions::Phase::Study);
    QCOMPARE(missing.landmarkCount(), 1);
}

void ProjectWorkspaceTest::checksumMismatchRefusesTheStudy() {
    const QString series = phantomFolder("medforge-m8-checksum-ct");
    const QString layout = QDir::temp().filePath(QStringLiteral("medforge-m8-checksum.ini"));
    const QString project = QDir::temp().filePath(QStringLiteral("medforge-m8-checksum.mfproject"));
    QFile::remove(layout);
    QFile::remove(project);
    WorkspaceWindow window(layout);
    window.importFolder(series);
    QTRY_COMPARE(window.actions()->phase(), WorkspaceActions::Phase::Study);
    window.placeLandmarkAtVoxel(1, 1, 1);
    QString saveError;
    QVERIFY2(window.saveProject(project, &saveError), qPrintable(saveError));

    QFile file(project);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QString text = QString::fromUtf8(file.readAll());
    file.close();
    QVERIFY(text.contains(QStringLiteral("checksum ")));
    text.replace(QStringLiteral("checksum "),
                 QStringLiteral("checksum 0000000000000001\nignored "));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write(text.toUtf8());
    file.close();

    const QString badLayout = QDir::temp().filePath(QStringLiteral("medforge-m8-checksum-bad.ini"));
    QFile::remove(badLayout);
    WorkspaceWindow bad(badLayout);
    QString openError;
    QVERIFY2(bad.openProject(project, &openError), qPrintable(openError));
    QTRY_VERIFY(bad.errorBanner()->message().contains(QStringLiteral("checksum")));
    QCOMPARE(bad.actions()->phase(), WorkspaceActions::Phase::Error);
    QCOMPARE(bad.landmarkCount(), 0);
}

void ProjectWorkspaceTest::closingDuringImportJoinsTheWorker() {
    const QString series = phantomFolder("medforge-m8-lifetime-ct");
    const QString layout = QDir::temp().filePath(QStringLiteral("medforge-m8-lifetime.ini"));
    QFile::remove(layout);
    {
        WorkspaceWindow window(layout);
        window.importFolder(series, 100);
        QTRY_COMPARE(window.actions()->phase(), WorkspaceActions::Phase::Importing);
    }
    QVERIFY(true);
}

void ProjectWorkspaceTest::projectActionsAreKeyboardAccessible() {
    const QString layout = QDir::temp().filePath(QStringLiteral("medforge-m8-keys.ini"));
    QFile::remove(layout);
    WorkspaceWindow window(layout);
    auto* save = window.findChild<QAction*>(QStringLiteral("saveProjectAction"));
    auto* open = window.findChild<QAction*>(QStringLiteral("openProjectAction"));
    QVERIFY(save != nullptr);
    QVERIFY(open != nullptr);
    QCOMPARE(save->shortcut(), QKeySequence::Save);
    QCOMPARE(open->shortcut(), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O));
    QVERIFY(!save->toolTip().isEmpty());
    QVERIFY(!open->toolTip().isEmpty());
    window.resize(1280, 800);
    window.show();
    window.activateWindow();
    auto* edit =
        window.findChild<NumericEditor*>(QStringLiteral("windowWidth"))->findChild<QLineEdit*>();
    QVERIFY(edit != nullptr);
    QCOMPARE(edit->focusPolicy(), Qt::StrongFocus);
    edit->setFocus(Qt::TabFocusReason);
    if (QApplication::focusWidget() == nullptr) {
        QSKIP("The offscreen platform did not activate keyboard focus.");
    }
    QCOMPARE(QApplication::focusWidget(), edit);
}

QTEST_MAIN(ProjectWorkspaceTest)
#include "ProjectWorkspaceTest.moc"
