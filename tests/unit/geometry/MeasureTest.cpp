#include "medforge/core/Coordinates.h"
#include "medforge/geometry/Measure.h"

#include <QFile>
#include <QtTest>
#include <cmath>
#include <filesystem>

using medforge::core::Vec3;
using medforge::core::voxelIndexToPatientLps;
using medforge::core::patientLpsToVoxelIndex;
using medforge::core::VoxelGridGeometry;
using medforge::geometry::MeasureDocument;
using medforge::geometry::MeasureKind;
using medforge::geometry::MeasureReportContext;
using medforge::geometry::TriangleMesh;
using medforge::geometry::evaluateMeasures;
using medforge::geometry::loadMeasureDocument;
using medforge::geometry::measureReportCsv;
using medforge::geometry::measureReportText;
using medforge::geometry::saveMeasureDocument;
using medforge::geometry::snapToSurface;

namespace {

TriangleMesh unitTriangle() {
    TriangleMesh mesh;
    mesh.positions = {{0, 0, 0}, {10, 0, 0}, {0, 10, 0}};
    mesh.indices = {0, 1, 2};
    return mesh;
}

TriangleMesh boxMesh(const Vec3& minCorner, const Vec3& maxCorner) {
    TriangleMesh mesh;
    const double x0 = minCorner.x;
    const double y0 = minCorner.y;
    const double z0 = minCorner.z;
    const double x1 = maxCorner.x;
    const double y1 = maxCorner.y;
    const double z1 = maxCorner.z;
    mesh.positions = {{x0, y0, z0}, {x1, y0, z0}, {x1, y1, z0}, {x0, y1, z0},
                      {x0, y0, z1}, {x1, y0, z1}, {x1, y1, z1}, {x0, y1, z1}};
    mesh.indices = {0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7, 0, 1, 5, 0, 5, 4,
                    3, 6, 2, 3, 7, 6, 0, 4, 7, 0, 7, 3, 1, 2, 6, 1, 6, 5};
    return mesh;
}

Vec3 rotated(const Vec3& point) {
    return Vec3{-point.y, point.x, point.z};
}

void expectNear(double actual, double expected) {
    QVERIFY2(std::abs(actual - expected) < 1e-6, qPrintable(QString::number(actual)));
}

} // namespace

class MeasureTest : public QObject {
    Q_OBJECT

private slots:
    void distanceAndRightAngleAreExact();
    void sectionAreaOfABoxIsTheCutFace();
    void valuesSurviveARigidTransform();
    void snapUsesTheToleranceAndFallsBack();
    void saveOmitsDerivedValuesAndReloadMatches();
    void reportCarriesUnitsProvenanceAndDisclaimer();
    void clickedVoxelRoundTripsThroughPatientMillimeters();
};

void MeasureTest::distanceAndRightAngleAreExact() {
    MeasureDocument document;
    const auto origin = document.addLandmark("O", {0, 0, 0}, false);
    const auto alongX = document.addLandmark("X", {3, 4, 0}, false);
    const auto axisX = document.addLandmark("A", {1, 0, 0}, false);
    const auto alongY = document.addLandmark("Y", {0, 1, 0}, false);
    document.addMeasure("OX", MeasureKind::Distance, {origin, alongX});
    document.addMeasure("angle", MeasureKind::Angle, {axisX, origin, alongY});
    const auto values = evaluateMeasures(document, nullptr);
    QCOMPARE(values.size(), static_cast<std::size_t>(2));
    QVERIFY(values[0].ok);
    QCOMPARE(values[0].unit, std::string("mm"));
    expectNear(values[0].value, 5);
    QVERIFY(values[1].ok);
    QCOMPARE(values[1].unit, std::string("deg"));
    expectNear(values[1].value, 90);
}

void MeasureTest::sectionAreaOfABoxIsTheCutFace() {
    const TriangleMesh box = boxMesh({0, 0, 0}, {10, 4, 6});
    MeasureDocument document;
    const auto a = document.addLandmark("A", {0, 0, 3}, false);
    const auto b = document.addLandmark("B", {10, 0, 3}, false);
    const auto c = document.addLandmark("C", {0, 4, 3}, false);
    document.addMeasure("cut", MeasureKind::SectionArea, {a, b, c});
    const auto values = evaluateMeasures(document, &box);
    QVERIFY(values[0].ok);
    QCOMPARE(values[0].unit, std::string("mm^2"));
    expectNear(values[0].value, 40);
    const auto missing = evaluateMeasures(document, nullptr);
    QVERIFY(!missing[0].ok);
}

void MeasureTest::valuesSurviveARigidTransform() {
    const TriangleMesh box = boxMesh({0, 0, 0}, {10, 4, 6});
    MeasureDocument document;
    const auto origin = document.addLandmark("O", {1, 2, 3}, false);
    const auto alongX = document.addLandmark("X", {4, 6, 3}, false);
    const auto alongY = document.addLandmark("Y", {1, 3, 3}, false);
    document.addMeasure("d", MeasureKind::Distance, {origin, alongX});
    document.addMeasure("a", MeasureKind::Angle, {alongX, origin, alongY});
    document.addMeasure("s", MeasureKind::SectionArea, {origin, alongX, alongY});
    const auto before = evaluateMeasures(document, &box);

    TriangleMesh turned = box;
    for (Vec3& position : turned.positions) {
        position = rotated(position);
    }
    for (auto& landmark : document.landmarks) {
        landmark.positionMm = rotated(landmark.positionMm);
    }
    const auto after = evaluateMeasures(document, &turned);
    QCOMPARE(after.size(), before.size());
    for (std::size_t index = 0; index < before.size(); ++index) {
        QCOMPARE(after[index].ok, before[index].ok);
        expectNear(after[index].value, before[index].value);
        QCOMPARE(after[index].unit, before[index].unit);
    }
}

void MeasureTest::snapUsesTheToleranceAndFallsBack() {
    const TriangleMesh mesh = unitTriangle();
    const auto near = snapToSurface(mesh, {1, 1, 0.4}, 1);
    QVERIFY(near.snapped);
    expectNear(near.positionMm.z, 0);
    expectNear(near.positionMm.x, 1);
    const auto far = snapToSurface(mesh, {1, 1, 5}, 1);
    QVERIFY(!far.snapped);
    expectNear(far.positionMm.z, 5);
}

void MeasureTest::saveOmitsDerivedValuesAndReloadMatches() {
    MeasureDocument document;
    const auto origin = document.addLandmark("O", {0, 0, 0}, false);
    const auto alongX = document.addLandmark("X", {3, 4, 0}, true);
    document.addMeasure("OX", MeasureKind::Distance, {origin, alongX});
    const auto path = std::filesystem::temp_directory_path() / "medforge-m5-measures.txt";
    std::string error;
    QVERIFY2(saveMeasureDocument(document, "1.2.3", path.string(), &error), error.c_str());
    QFile file(QString::fromStdString(path.string()));
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QString text = QString::fromUtf8(file.readAll());
    QVERIFY(!text.contains(QStringLiteral("value=")));
    MeasureDocument loaded;
    std::string series;
    QVERIFY2(loadMeasureDocument(path.string(), &loaded, &series, &error), error.c_str());
    QCOMPARE(series, std::string("1.2.3"));
    QCOMPARE(loaded.landmarks.size(), static_cast<std::size_t>(2));
    QVERIFY(loaded.landmarks[1].snapped);
    const auto values = evaluateMeasures(loaded, nullptr);
    expectNear(values[0].value, 5);
}

void MeasureTest::reportCarriesUnitsProvenanceAndDisclaimer() {
    MeasureDocument document;
    const auto origin = document.addLandmark("O", {0, 0, 0}, false);
    const auto alongX = document.addLandmark("X", {3, 4, 0}, false);
    const auto alongY = document.addLandmark("Y", {0, 1, 0}, false);
    const auto plane = document.addLandmark("P", {0, 0, 1}, false);
    document.addMeasure("OX", MeasureKind::Distance, {origin, alongX});
    document.addMeasure("angle", MeasureKind::Angle, {alongX, origin, alongY});
    document.addMeasure("cut", MeasureKind::SectionArea, {origin, alongX, plane});
    const MeasureReportContext context{"1.2.826.0.1.3680043.10.543.100", {0.8, 0.5, 2}};
    const TriangleMesh box = boxMesh({0, 0, 0}, {10, 10, 2});
    const std::string csv = measureReportCsv(document, &box, context);
    const std::string text = measureReportText(document, &box, context);
    QVERIFY(csv.find("mm") != std::string::npos);
    QVERIFY(csv.find("deg") != std::string::npos);
    QVERIFY(csv.find("mm^2") != std::string::npos);
    QVERIFY(text.find("1.2.826.0.1.3680043.10.543.100") != std::string::npos);
    QVERIFY(text.find("not for clinical use") != std::string::npos);
    QVERIFY(text.find("Not a validated measurement") != std::string::npos);
    QVERIFY(text.find("CT resolution") != std::string::npos);
    QVERIFY(text.find("segmentation") != std::string::npos);
    QVERIFY(text.find("landmark placement") != std::string::npos);
    QVERIFY(text.find("0.8") != std::string::npos);
}

void MeasureTest::clickedVoxelRoundTripsThroughPatientMillimeters() {
    VoxelGridGeometry geometry;
    geometry.originMm = {10, -20, 0};
    geometry.spacingMm = {0.8, 0.5, 2};
    const auto lps = voxelIndexToPatientLps({3, 2, 1}, geometry);
    QVERIFY(lps.ok());
    MeasureDocument document;
    document.addLandmark("marker", lps.value, false);
    const auto back = patientLpsToVoxelIndex(document.landmarks[0].positionMm, geometry);
    QVERIFY(back.ok());
    expectNear(back.value.x, 3);
    expectNear(back.value.y, 2);
    expectNear(back.value.z, 1);
}

QTEST_APPLESS_MAIN(MeasureTest)
#include "MeasureTest.moc"
