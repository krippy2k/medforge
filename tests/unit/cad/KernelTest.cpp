#include "medforge/cad/Kernel.h"

#include <QDir>
#include <QFile>
#include <QtTest>

#include <cmath>
#include <filesystem>

namespace {

bool near(double value, double expected, double tolerance) {
    return std::abs(value - expected) <= tolerance;
}

std::filesystem::path tempFile(const char* name) {
    return std::filesystem::path(QDir::tempPath().toStdString()) / name;
}

} // namespace

class CadKernelTest : public QObject {
    Q_OBJECT

private slots:
    void init() {
        if (!medforge::cad::available()) {
            QSKIP("Open CASCADE is not linked");
        }
    }

    void boxAndTransformKeepVolume();
    void clearanceReportsDistanceAndOverlap();
    void booleanFailureIsRejected();
    void stepRoundTripChecksBoundsVolumeAndWatermark();
    void igesRoundTripAndRejectedFile();
    void guideIsRepeatable();
};

void CadKernelTest::boxAndTransformKeepVolume() {
    std::string error;
    const medforge::cad::Shape box =
        medforge::cad::makeBox({0, 0, 0}, {10, 10, 10}, &error);
    QVERIFY2(box.isValidSolid(), error.c_str());
    QVERIFY(near(box.volumeMm3(), 1000, 1e-6));
    QVERIFY(near(box.boundsMin().x, 0, 1e-6));
    QVERIFY(near(box.boundsMax().z, 10, 1e-6));
    QVERIFY(!box.mesh().indices.empty());

    medforge::cad::Pose pose;
    pose.translationMm = {5, 0, 0};
    const medforge::cad::Shape moved = medforge::cad::transformShape(box, pose, &error);
    QVERIFY2(moved.isValidSolid(), error.c_str());
    QVERIFY(near(moved.volumeMm3(), 1000, 1e-6));
    QVERIFY(near(moved.boundsMin().x, 5, 1e-4));
    QVERIFY(near(moved.boundsMax().x, 15, 1e-4));

    const medforge::cad::Shape plate =
        medforge::cad::makeBox({-5, -5, 0}, {10, 10, 2}, &error);
    pose = {};
    pose.rotationDeg = {90, 0, 0};
    const medforge::cad::Shape turned = medforge::cad::transformShape(plate, pose, &error);
    QVERIFY2(turned.isValidSolid(), error.c_str());
    QVERIFY(near(turned.volumeMm3(), 200, 1e-4));
    QVERIFY(turned.boundsMax().z > 4);
    QVERIFY(turned.boundsMin().z < -4);
}

void CadKernelTest::clearanceReportsDistanceAndOverlap() {
    std::string error;
    const medforge::cad::Shape left = medforge::cad::makeBox({0, 0, 0}, {10, 10, 10}, &error);
    const medforge::cad::Shape apart = medforge::cad::makeBox({12, 0, 0}, {10, 10, 10}, &error);
    const medforge::cad::Clearance gap = medforge::cad::measureClearance(left, apart, 0.1);
    QVERIFY2(gap.ok, gap.message.c_str());
    QVERIFY(gap.method.find("BRepExtrema") != std::string::npos);
    QVERIFY(near(gap.distanceMm, 2, 0.05));
    QVERIFY(!gap.overlap);
    QVERIFY(gap.commonVolumeMm3 < 1e-4);
    QCOMPARE(gap.toleranceMm, 0.1);

    const medforge::cad::Shape into = medforge::cad::makeBox({5, 0, 0}, {10, 10, 10}, &error);
    const medforge::cad::Clearance hit = medforge::cad::measureClearance(left, into, 0.1);
    QVERIFY(hit.ok);
    QVERIFY(hit.distanceMm < 0.05);
    QVERIFY(hit.overlap);
    QVERIFY(near(hit.commonVolumeMm3, 500, 0.05));

    const medforge::cad::Clearance bad = medforge::cad::measureClearance(left, into, 0);
    QVERIFY(!bad.ok);
    QVERIFY(bad.message.find("olerance") != std::string::npos);
}

void CadKernelTest::booleanFailureIsRejected() {
    std::string error;
    const medforge::cad::Shape box = medforge::cad::makeBox({0, 0, 0}, {10, 4, 6}, &error);
    const medforge::cad::Shape same = medforge::cad::makeBox({0, 0, 0}, {10, 4, 6}, &error);
    const medforge::cad::Shape cut = medforge::cad::booleanCut(box, same, &error);
    QVERIFY(cut.isNull() || !cut.isValidSolid());
    const std::string message = error + cut.message();
    QVERIFY(message.find("Boolean") != std::string::npos ||
            message.find("watertight") != std::string::npos);

    const medforge::cad::Shape empty = medforge::cad::booleanCut(box, {}, &error);
    QVERIFY(empty.isNull());
    QVERIFY(error.find("Boolean") != std::string::npos);

    const medforge::cad::Shape badBox = medforge::cad::makeBox({0, 0, 0}, {0, 1, 1}, &error);
    QVERIFY(badBox.isNull());
    QVERIFY(!error.empty());
}

void CadKernelTest::stepRoundTripChecksBoundsVolumeAndWatermark() {
    std::string error;
    const medforge::cad::Shape box = medforge::cad::makeBox({1, -2, 3}, {8, 5, 2}, &error);
    const auto path = tempFile("medforge-m7-box.step");
    std::filesystem::remove(path);
    QVERIFY2(medforge::cad::writeStep(box, path.string(), &error), error.c_str());
    QFile file(QString::fromStdString(path.string()));
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray text = file.readAll();
    QVERIFY(text.contains("Not for manufacture"));
    QVERIFY(text.contains("clinical use"));

    const medforge::cad::Shape loaded = medforge::cad::readStep(path.string(), &error);
    QVERIFY2(loaded.isValidSolid(), error.c_str());
    QVERIFY(near(loaded.volumeMm3(), box.volumeMm3(), 0.05));
    QVERIFY(near(loaded.boundsMin().x, box.boundsMin().x, 0.01));
    QVERIFY(near(loaded.boundsMax().y, box.boundsMax().y, 0.01));
    QVERIFY(near(loaded.boundsMax().z, box.boundsMax().z, 0.01));
}

void CadKernelTest::igesRoundTripAndRejectedFile() {
    std::string error;
    const medforge::cad::Shape box = medforge::cad::makeBox({0, 0, 0}, {4, 3, 2}, &error);
    const auto iges = tempFile("medforge-m7-box.igs");
    std::filesystem::remove(iges);
    QVERIFY2(medforge::cad::writeIges(box, iges.string(), &error), error.c_str());
    const medforge::cad::Shape loaded = medforge::cad::readIges(iges.string(), &error);
    QVERIFY2(loaded.isValidSolid(), error.c_str());
    QVERIFY(near(loaded.volumeMm3(), 24, 0.05));
    QVERIFY(near(loaded.boundsMax().x, 4, 0.05));

    const auto junk = tempFile("medforge-m7-junk.step");
    QFile file(QString::fromStdString(junk.string()));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write("this is not a step file");
    file.close();
    const medforge::cad::Shape rejected = medforge::cad::readStep(junk.string(), &error);
    QVERIFY(rejected.isNull());
    QVERIFY(error.find("STEP") != std::string::npos);
}

void CadKernelTest::guideIsRepeatable() {
    medforge::cad::GuideSpec spec;
    spec.contactOriginMm = {10, -20, 6};
    spec.patchLengthMm = 10;
    spec.patchWidthMm = 8;
    spec.offsetMm = 4;
    spec.holeDiameterMm = 2;
    std::string error;
    const medforge::cad::Shape first = medforge::cad::buildGuide(spec, &error);
    QVERIFY2(first.isValidSolid(), error.c_str());
    const double expected = (10.0 * 8.0 * 4.0) - (3.141592653589793 * 1.0 * 1.0 * 4.0);
    QVERIFY2(near(first.volumeMm3(), expected, 0.05),
             qPrintable(QString::number(first.volumeMm3())));
    const medforge::cad::Shape second = medforge::cad::buildGuide(spec, &error);
    QVERIFY(near(second.volumeMm3(), first.volumeMm3(), 1e-6));
    QVERIFY(near(second.boundsMin().x, first.boundsMin().x, 1e-6));
    QVERIFY(near(second.boundsMax().z, first.boundsMax().z, 1e-6));

    spec.holeDiameterMm = 9;
    const medforge::cad::Shape refused = medforge::cad::buildGuide(spec, &error);
    QVERIFY(refused.isNull());
    QVERIFY(error.find("hole") != std::string::npos);

    spec.holeDiameterMm = 2;
    spec.offsetMm = 0;
    const medforge::cad::Shape noOffset = medforge::cad::buildGuide(spec, &error);
    QVERIFY(noOffset.isNull());
    QVERIFY(error.find("offset") != std::string::npos);
}

QTEST_APPLESS_MAIN(CadKernelTest)
#include "KernelTest.moc"
