#include "medforge/imaging/DicomWrite.h"
#include "medforge/imaging/SyntheticPhantom.h"
#include "medforge/ui/FoundationWidgets.h"
#include "medforge/ui/MprViewport.h"
#include "medforge/ui/SegmentedControl.h"
#include "medforge/ui/WorkspaceActions.h"
#include "medforge/ui/WorkspaceWindow.h"

#include <QDir>
#include <QLabel>
#include <QtTest>
#include <filesystem>

using medforge::imaging::DicomCtInstance;
using medforge::imaging::DicomReadStatus;
using medforge::imaging::phantom::kMarkerI;
using medforge::imaging::phantom::kMarkerJ;
using medforge::imaging::writePart10Ct;

namespace {

DicomCtInstance sliceAt(const std::string& uid, double zMm) {
    DicomCtInstance instance;
    instance.status = DicomReadStatus::Ok;
    instance.seriesInstanceUid = uid;
    instance.geometry.imageOrientationPatient = {1, 0, 0, 0, 1, 0};
    instance.geometry.imagePositionPatient = medforge::core::Vec3{0, 0, zMm};
    instance.geometry.rowSpacingMm = 1;
    instance.geometry.columnSpacingMm = 1;
    instance.rescale = medforge::imaging::Rescale{1, 0};
    instance.rows = 2;
    instance.columns = 2;
    instance.storedValues = {0, 0, 0, 0};
    return instance;
}

std::filesystem::path tempFolder(const char* name) {
    const std::filesystem::path folder = std::filesystem::temp_directory_path() / name;
    std::filesystem::remove_all(folder);
    std::filesystem::create_directories(folder);
    return folder;
}

} // namespace

class MprWorkspaceTest : public QObject {
    Q_OBJECT

private slots:
    void emptyWorkspaceShowsThreePlanes();
    void importShowsLinkedViewsAndHounsfieldReadout();
    void reformattedViewsUseMillimeterSpacing();
    void keyboardScrollMovesTheSharedCrosshair();
    void lungPresetChangesTheDisplayedGray();
    void duplicateSeriesShowsAnError();
    void missingSliceKeepsTheViewsAndWarns();
    void cancelReturnsToTheEmptyWorkspace();
};

void MprWorkspaceTest::emptyWorkspaceShowsThreePlanes() {
    WorkspaceWindow window(QDir::temp().filePath(QStringLiteral("medforge-m2-empty.ini")));
    QCOMPARE(window.axialViewport()->planeTitle(), QStringLiteral("Axial"));
    QCOMPARE(window.sagittalViewport()->planeTitle(), QStringLiteral("Sagittal"));
    QCOMPARE(window.coronalViewport()->planeTitle(), QStringLiteral("Coronal"));
    QVERIFY(!window.axialViewport()->hasImage());
    QCOMPARE(window.actions()->statusText(), QStringLiteral("No dataset loaded"));
    QVERIFY(window.importCtAction() != nullptr);
}

void MprWorkspaceTest::importShowsLinkedViewsAndHounsfieldReadout() {
    const std::filesystem::path folder = tempFolder("medforge-m2-ui-phantom");
    std::string error;
    QVERIFY2(medforge::imaging::phantom::writeSyntheticAxialSeries(folder, &error), error.c_str());
    WorkspaceWindow window(QDir::temp().filePath(QStringLiteral("medforge-m2-ui.ini")));
    window.importFolder(QString::fromStdString(folder.string()));
    QTRY_COMPARE(window.actions()->phase(), WorkspaceActions::Phase::Study);
    QVERIFY(window.axialViewport()->hasImage());
    QCOMPARE(window.axialViewport()->imagePixelSize(), QSize(8, 6));
    QCOMPARE(window.sagittalViewport()->imagePixelSize(), QSize(6, 4));
    QCOMPARE(window.coronalViewport()->imagePixelSize(), QSize(8, 4));
    QCOMPARE(window.axialViewport()->leftLabel(), QStringLiteral("R"));
    QCOMPARE(window.axialViewport()->rightLabel(), QStringLiteral("L"));
    QCOMPARE(window.axialViewport()->topLabel(), QStringLiteral("A"));
    QCOMPARE(window.axialViewport()->bottomLabel(), QStringLiteral("P"));
    QCOMPARE(window.axialViewport()->crosshair(), QPoint(4, 3));
    QCOMPARE(window.sagittalViewport()->crosshair(), QPoint(3, 2));
    QCOMPARE(window.coronalViewport()->crosshair(), QPoint(4, 2));
    QVERIFY(window.cursorReadout()->text().contains(QStringLiteral("i=4")));
    QVERIFY(window.cursorReadout()->text().contains(QStringLiteral("HU ")));
}

void MprWorkspaceTest::reformattedViewsUseMillimeterSpacing() {
    const std::filesystem::path folder = tempFolder("medforge-m2-ui-aspect");
    std::string error;
    QVERIFY2(medforge::imaging::phantom::writeSyntheticAxialSeries(folder, &error), error.c_str());
    WorkspaceWindow window(QDir::temp().filePath(QStringLiteral("medforge-m2-aspect.ini")));
    window.show();
    window.importFolder(QString::fromStdString(folder.string()));
    QTRY_COMPARE(window.actions()->phase(), WorkspaceActions::Phase::Study);

    const QRect sagittal = window.sagittalViewport()->imageBounds();
    QVERIFY(sagittal.width() > 10);
    const double sagittalMmWide =
        medforge::imaging::phantom::kRows * medforge::imaging::phantom::kRowSpacingMm;
    const double sagittalMmTall =
        medforge::imaging::phantom::kSlices * medforge::imaging::phantom::kSliceSpacingMm;
    const double ratio =
        static_cast<double>(sagittal.width()) / static_cast<double>(sagittal.height());
    QVERIFY(std::abs(ratio - sagittalMmWide / sagittalMmTall) < 0.05);
}

void MprWorkspaceTest::keyboardScrollMovesTheSharedCrosshair() {
    const std::filesystem::path folder = tempFolder("medforge-m2-ui-keys");
    std::string error;
    QVERIFY(medforge::imaging::phantom::writeSyntheticAxialSeries(folder, &error));
    WorkspaceWindow window(QDir::temp().filePath(QStringLiteral("medforge-m2-keys.ini")));
    window.show();
    window.importFolder(QString::fromStdString(folder.string()));
    QTRY_COMPARE(window.actions()->phase(), WorkspaceActions::Phase::Study);
    QTest::keyClick(window.axialViewport(), Qt::Key_Down);
    QVERIFY(window.cursorReadout()->text().contains(QStringLiteral("k=3")));
    QCOMPARE(window.sagittalViewport()->crosshair().y(), 3);
    QCOMPARE(window.coronalViewport()->crosshair().y(), 3);
    QTest::keyClick(window.axialViewport(), Qt::Key_Right);
    QVERIFY(window.cursorReadout()->text().contains(QStringLiteral("i=5")));
    QCOMPARE(window.coronalViewport()->crosshair().x(), 5);
}

void MprWorkspaceTest::lungPresetChangesTheDisplayedGray() {
    const std::filesystem::path folder = tempFolder("medforge-m2-ui-wl");
    std::string error;
    QVERIFY(medforge::imaging::phantom::writeSyntheticAxialSeries(folder, &error));
    WorkspaceWindow window(QDir::temp().filePath(QStringLiteral("medforge-m2-wl.ini")));
    window.importFolder(QString::fromStdString(folder.string()));
    QTRY_COMPARE(window.actions()->phase(), WorkspaceActions::Phase::Study);
    QTest::keyClick(window.axialViewport(), Qt::Key_Up);
    QCOMPARE(window.axialViewport()->grayAt(0, 0), 0);
    QCOMPARE(window.axialViewport()->grayAt(kMarkerI, kMarkerJ), 255);
    auto* presets = window.findChild<SegmentedControl*>(QStringLiteral("windowLevelPresets"));
    QVERIFY(presets != nullptr);
    QVERIFY(presets->setCurrentIndex(1));
    QVERIFY(window.axialViewport()->grayAt(0, 0) > 0);
    QVERIFY(window.axialViewport()->grayAt(0, 0) < 255);
}

void MprWorkspaceTest::duplicateSeriesShowsAnError() {
    const std::filesystem::path folder = tempFolder("medforge-m2-ui-dup");
    std::string error;
    QVERIFY(writePart10Ct(folder / "a.dcm", sliceAt("1.2.9.8", 0), 1, &error));
    QVERIFY(writePart10Ct(folder / "b.dcm", sliceAt("1.2.9.8", 0), 2, &error));
    WorkspaceWindow window(QDir::temp().filePath(QStringLiteral("medforge-m2-dup.ini")));
    window.importFolder(QString::fromStdString(folder.string()));
    QTRY_COMPARE(window.actions()->phase(), WorkspaceActions::Phase::Error);
    QCOMPARE(window.actions()->statusText(), QStringLiteral("Import failed"));
    QVERIFY(window.errorBanner()->message().contains(QStringLiteral("Duplicate slice")));
    QVERIFY(!window.axialViewport()->hasImage());
}

void MprWorkspaceTest::missingSliceKeepsTheViewsAndWarns() {
    const std::filesystem::path folder = tempFolder("medforge-m2-ui-gap");
    std::string error;
    QVERIFY(writePart10Ct(folder / "a.dcm", sliceAt("1.2.9.7", 0), 1, &error));
    QVERIFY(writePart10Ct(folder / "b.dcm", sliceAt("1.2.9.7", 2), 2, &error));
    QVERIFY(writePart10Ct(folder / "c.dcm", sliceAt("1.2.9.7", 6), 3, &error));
    WorkspaceWindow window(QDir::temp().filePath(QStringLiteral("medforge-m2-gap.ini")));
    window.importFolder(QString::fromStdString(folder.string()));
    QTRY_COMPARE(window.actions()->phase(), WorkspaceActions::Phase::Study);
    QVERIFY(window.axialViewport()->hasImage());
    QCOMPARE(window.errorBanner()->severityText(), QStringLiteral("Warning"));
    QVERIFY(window.errorBanner()->message().contains(QStringLiteral("Missing slice")));
}

void MprWorkspaceTest::cancelReturnsToTheEmptyWorkspace() {
    const std::filesystem::path folder = tempFolder("medforge-m2-ui-cancel");
    std::string error;
    QVERIFY(medforge::imaging::phantom::writeSyntheticAxialSeries(folder, &error));
    WorkspaceWindow window(QDir::temp().filePath(QStringLiteral("medforge-m2-cancel.ini")));
    window.importFolder(QString::fromStdString(folder.string()), 200);
    QCOMPARE(window.actions()->phase(), WorkspaceActions::Phase::Importing);
    window.cancelImport();
    QTRY_VERIFY(window.errorBanner()->message().contains(QStringLiteral("cancelled")));
    QCOMPARE(window.actions()->phase(), WorkspaceActions::Phase::Empty);
    QVERIFY(!window.axialViewport()->hasImage());
}

QTEST_MAIN(MprWorkspaceTest)
#include "MprWorkspaceTest.moc"
