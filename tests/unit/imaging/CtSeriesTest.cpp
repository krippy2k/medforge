#include "medforge/imaging/CtSeries.h"

#include "medforge/core/Coordinates.h"

#include <QtTest>
#include <cmath>
#include <limits>

using medforge::core::Vec3;
using medforge::core::voxelIndexToPatientLps;
using medforge::imaging::Rescale;
using medforge::imaging::SeriesGeometryStatus;
using medforge::imaging::SliceGeometry;
using medforge::imaging::TransferSyntaxClass;
using medforge::imaging::WindowLevel;
using medforge::imaging::assessSeriesGeometry;
using medforge::imaging::classifyTransferSyntax;
using medforge::imaging::storedValueToHu;
using medforge::imaging::windowBoundsHu;

namespace {

constexpr double kTolerance = 1e-6;

SliceGeometry axialSlice(double zMm) {
    SliceGeometry slice;
    slice.imageOrientationPatient = {1, 0, 0, 0, 1, 0};
    slice.imagePositionPatient = Vec3{10, -20, zMm};
    slice.rowSpacingMm = 0.5;
    slice.columnSpacingMm = 0.8;
    return slice;
}

void expectNear(double actual, double expected) {
    QVERIFY(std::abs(actual - expected) <= kTolerance);
}

} // namespace

class CtSeriesTest : public QObject {
    Q_OBJECT

private slots:
    void axialStackMapsVoxelCornersToLps();
    void obliqueStackUsesCrossProductNormal();
    void rejectsDuplicateAndInconsistentSlices();
    void warnsWhenASliceIsMissing();
    void convertsStoredValuesToHounsfield();
    void windowLevelPresetsBoundTheDisplayRange();
    void classifiesUncompressedAndNamedCompressedSyntaxes();
};

void CtSeriesTest::axialStackMapsVoxelCornersToLps() {
    const auto series = assessSeriesGeometry({axialSlice(-10), axialSlice(-8), axialSlice(-6)});
    QCOMPARE(series.status, SeriesGeometryStatus::Ok);
    QVERIFY(!series.missingSlice);
    expectNear(series.geometry.spacingMm.x, 0.8);
    expectNear(series.geometry.spacingMm.y, 0.5);
    expectNear(series.geometry.spacingMm.z, 2);
    const auto corner = voxelIndexToPatientLps(Vec3{1, 2, 1}, series.geometry);
    QVERIFY(corner.ok());
    expectNear(corner.value.x, 10.8);
    expectNear(corner.value.y, -19);
    expectNear(corner.value.z, -8);
}

void CtSeriesTest::obliqueStackUsesCrossProductNormal() {
    SliceGeometry first;
    first.imageOrientationPatient = {1, 0, 0, 0, 0, -1};
    first.imagePositionPatient = Vec3{0, 5, 0};
    first.rowSpacingMm = 1;
    first.columnSpacingMm = 1;
    SliceGeometry second = first;
    second.imagePositionPatient = Vec3{0, 8, 0};
    const auto series = assessSeriesGeometry({second, first});
    QCOMPARE(series.status, SeriesGeometryStatus::Ok);
    const auto point = voxelIndexToPatientLps(Vec3{0, 1, 1}, series.geometry);
    QVERIFY(point.ok());
    expectNear(point.value.x, 0);
    expectNear(point.value.y, 8);
    expectNear(point.value.z, -1);
}

void CtSeriesTest::rejectsDuplicateAndInconsistentSlices() {
    auto duplicate = assessSeriesGeometry({axialSlice(0), axialSlice(0)});
    QCOMPARE(duplicate.status, SeriesGeometryStatus::DuplicateSlice);

    auto tilted = axialSlice(2);
    tilted.imageOrientationPatient = {0, 1, 0, 0, 0, -1};
    auto inconsistent = assessSeriesGeometry({axialSlice(0), tilted});
    QCOMPARE(inconsistent.status, SeriesGeometryStatus::InconsistentOrientation);

    auto empty = assessSeriesGeometry({});
    QCOMPARE(empty.status, SeriesGeometryStatus::Empty);

    auto uneven = assessSeriesGeometry({axialSlice(0), axialSlice(2), axialSlice(5)});
    QCOMPARE(uneven.status, SeriesGeometryStatus::InconsistentSpacing);

    auto single = assessSeriesGeometry({axialSlice(0)});
    QCOMPARE(single.status, SeriesGeometryStatus::InsufficientSlices);
}

void CtSeriesTest::warnsWhenASliceIsMissing() {
    const auto series = assessSeriesGeometry({axialSlice(0), axialSlice(2), axialSlice(6)});
    QCOMPARE(series.status, SeriesGeometryStatus::Ok);
    QVERIFY(series.missingSlice);
    QCOMPARE(series.sliceCount, 4);
}

void CtSeriesTest::convertsStoredValuesToHounsfield() {
    const Rescale rescale{2, -1024};
    expectNear(storedValueToHu(0, rescale), -1024);
    expectNear(storedValueToHu(24, rescale), -976);
    QVERIFY(std::isnan(storedValueToHu(std::numeric_limits<double>::quiet_NaN(), rescale)));
}

void CtSeriesTest::windowLevelPresetsBoundTheDisplayRange() {
    const auto softTissue = windowBoundsHu(WindowLevel::softTissue());
    expectNear(softTissue.lowerHu, -160);
    expectNear(softTissue.upperHu, 240);
    const auto lung = windowBoundsHu(WindowLevel::lung());
    expectNear(lung.lowerHu, -1350);
    expectNear(lung.upperHu, 150);
}

void CtSeriesTest::classifiesUncompressedAndNamedCompressedSyntaxes() {
    QCOMPARE(classifyTransferSyntax("1.2.840.10008.1.2.1"), TransferSyntaxClass::Uncompressed);
    QCOMPARE(classifyTransferSyntax("1.2.840.10008.1.2"), TransferSyntaxClass::Uncompressed);
    QCOMPARE(classifyTransferSyntax("1.2.840.10008.1.2.4.70"), TransferSyntaxClass::Compressed);
    QCOMPARE(classifyTransferSyntax("1.2.840.10008.1.2.4.90"), TransferSyntaxClass::Compressed);
    QCOMPARE(classifyTransferSyntax("1.2.840.10008.1.2.5"), TransferSyntaxClass::Compressed);
    QCOMPARE(classifyTransferSyntax("1.2.999.1"), TransferSyntaxClass::Unrecognized);
}

QTEST_APPLESS_MAIN(CtSeriesTest)
#include "CtSeriesTest.moc"
