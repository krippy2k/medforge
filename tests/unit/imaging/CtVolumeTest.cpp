#include "medforge/imaging/CtVolume.h"

#include <QtTest>
#include <cmath>
#include <limits>

using medforge::imaging::DicomCtInstance;
using medforge::imaging::DicomReadStatus;
using medforge::imaging::MprCrosshair;
using medforge::imaging::MprPlane;
using medforge::imaging::SeriesGeometryStatus;
using medforge::imaging::VolumeStatus;
using medforge::imaging::WindowLevel;
using medforge::imaging::buildCtVolume;
using medforge::imaging::crosshairLps;
using medforge::imaging::crosshairPixel;
using medforge::imaging::huToDisplayGray;
using medforge::imaging::moveCrosshairOnPlane;
using medforge::imaging::orientationLabels;
using medforge::imaging::scrollCrosshair;

namespace {

DicomCtInstance axialSlice(const std::string& uid, double zMm, int rows, int columns) {
    DicomCtInstance instance;
    instance.status = DicomReadStatus::Ok;
    instance.seriesInstanceUid = uid;
    instance.geometry.imageOrientationPatient = {1, 0, 0, 0, 1, 0};
    instance.geometry.imagePositionPatient = medforge::core::Vec3{10, -20, zMm};
    instance.geometry.rowSpacingMm = 0.5;
    instance.geometry.columnSpacingMm = 0.8;
    instance.rescale = medforge::imaging::Rescale{2, -1024};
    instance.rows = rows;
    instance.columns = columns;
    instance.storedValues.assign(static_cast<std::size_t>(rows * columns), 12);
    return instance;
}

} // namespace

class CtVolumeTest : public QObject {
    Q_OBJECT

private slots:
    void mapsStoredValuesToKnownHounsfieldUnits();
    void roundTripsTheCrosshairThroughPatientSpace();
    void keepsTheCrosshairOnTheSamePointAcrossPlanes();
    void warnsWhenASliceIsMissing();
    void rejectsDuplicateSlices();
    void windowLevelMapsTheSoftTissueRange();
};

void CtVolumeTest::mapsStoredValuesToKnownHounsfieldUnits() {
    auto first = axialSlice("1.2.3", 0, 6, 8);
    auto second = axialSlice("1.2.3", 2, 6, 8);
    second.storedValues[static_cast<std::size_t>(2 * 8 + 3)] = 712;
    const auto built = buildCtVolume({first, second});
    QCOMPARE(built.status, VolumeStatus::Ok);
    QVERIFY(std::abs(built.volume.huAt(0, 0, 0) - (-1000.0F)) < 0.01F);
    QVERIFY(std::abs(built.volume.huAt(3, 2, 1) - 400.0F) < 0.01F);
}

void CtVolumeTest::roundTripsTheCrosshairThroughPatientSpace() {
    const auto built = buildCtVolume({axialSlice("1.2.3", 0, 6, 8), axialSlice("1.2.3", 2, 6, 8)});
    MprCrosshair crosshair{3, 2, 1};
    const auto lps = crosshairLps(crosshair, built.volume);
    QVERIFY(lps.ok());
    QVERIFY(std::abs(lps.value.x - 12.4) < 1e-6);
    QVERIFY(std::abs(lps.value.y - (-19.0)) < 1e-6);
    QVERIFY(std::abs(lps.value.z - 2.0) < 1e-6);
    const auto back =
        medforge::core::patientLpsToVoxelIndex(lps.value, built.volume.geometry.geometry);
    QVERIFY(back.ok());
    QVERIFY(std::abs(back.value.x - 3) < 1e-6);
    QVERIFY(std::abs(back.value.y - 2) < 1e-6);
    QVERIFY(std::abs(back.value.z - 1) < 1e-6);
}

void CtVolumeTest::keepsTheCrosshairOnTheSamePointAcrossPlanes() {
    const auto built =
        buildCtVolume({axialSlice("1.2.3", 0, 6, 8), axialSlice("1.2.3", 2, 6, 8),
                       axialSlice("1.2.3", 4, 6, 8), axialSlice("1.2.3", 6, 6, 8)});
    MprCrosshair crosshair{4, 3, 2};
    scrollCrosshair(crosshair, MprPlane::Axial, 1, built.volume);
    QCOMPARE(crosshair.k, 3);
    QCOMPARE(crosshairPixel(crosshair, MprPlane::Axial).x, 4);
    QCOMPARE(crosshairPixel(crosshair, MprPlane::Sagittal).y, 3);
    QCOMPARE(crosshairPixel(crosshair, MprPlane::Coronal).y, 3);
    moveCrosshairOnPlane(crosshair, MprPlane::Sagittal, 1, 2, built.volume);
    QCOMPARE(crosshair.j, 1);
    QCOMPARE(crosshair.k, 2);
    QCOMPARE(crosshair.i, 4);
    QCOMPARE(crosshairPixel(crosshair, MprPlane::Axial).y, 1);
    QCOMPARE(crosshairPixel(crosshair, MprPlane::Coronal).y, 2);
    const auto labels = orientationLabels(built.volume, MprPlane::Axial);
    QCOMPARE(labels.left, 'R');
    QCOMPARE(labels.right, 'L');
    QCOMPARE(labels.top, 'A');
    QCOMPARE(labels.bottom, 'P');
}

void CtVolumeTest::warnsWhenASliceIsMissing() {
    const auto built = buildCtVolume(
        {axialSlice("1.2.3", 0, 4, 4), axialSlice("1.2.3", 2, 4, 4), axialSlice("1.2.3", 6, 4, 4)});
    QCOMPARE(built.status, VolumeStatus::Ok);
    QVERIFY(built.volume.missingSlice);
    QCOMPARE(built.volume.slices, 4);
    QVERIFY(std::isnan(built.volume.huAt(0, 0, 2)));
    QCOMPARE(built.message, std::string("Missing slice in the series"));
}

void CtVolumeTest::rejectsDuplicateSlices() {
    const auto built = buildCtVolume({axialSlice("1.2.3", 0, 4, 4), axialSlice("1.2.3", 0, 4, 4)});
    QCOMPARE(built.status, VolumeStatus::Rejected);
    QCOMPARE(built.geometryStatus, SeriesGeometryStatus::DuplicateSlice);
    QCOMPARE(built.message, std::string("Duplicate slice positions"));
}

void CtVolumeTest::windowLevelMapsTheSoftTissueRange() {
    const WindowLevel soft = WindowLevel::softTissue();
    QCOMPARE(huToDisplayGray(-160, soft), 0);
    QCOMPARE(huToDisplayGray(40, soft), 128);
    QCOMPARE(huToDisplayGray(240, soft), 255);
    QCOMPARE(huToDisplayGray(std::numeric_limits<float>::quiet_NaN(), soft), 0);
}

QTEST_APPLESS_MAIN(CtVolumeTest)
#include "CtVolumeTest.moc"
