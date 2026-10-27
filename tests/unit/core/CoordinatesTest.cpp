#include "medforge/core/Coordinates.h"

#include <QtTest>
#include <cmath>
#include <limits>

using medforge::core::CoordinateStatus;
using medforge::core::Mat3;
using medforge::core::patientLpsToVoxelIndex;
using medforge::core::Vec3;
using medforge::core::VoxelGridGeometry;
using medforge::core::voxelIndexToPatientLps;

#define EXPECT_NEAR(actual, ex, ey, ez)                                                            \
    do {                                                                                           \
        constexpr double kToleranceMm = 1e-9;                                                      \
        QVERIFY(std::abs((actual).x - (ex)) <= kToleranceMm);                                      \
        QVERIFY(std::abs((actual).y - (ey)) <= kToleranceMm);                                      \
        QVERIFY(std::abs((actual).z - (ez)) <= kToleranceMm);                                      \
    } while (false)

class CoordinatesTest : public QObject {
    Q_OBJECT

private slots:
    void identityMapsIndexToMillimeters();
    void appliesOriginAndAnisotropicSpacing();
    void directionCosinesMapVoxelAxesIntoLps();
    void roundTripsPatientPointToVoxelIndex();
    void rejectsNonPositiveSpacing();
    void rejectsNonFiniteInput();
    void rejectsSingularDirection();
};

void CoordinatesTest::identityMapsIndexToMillimeters() {
    const VoxelGridGeometry geometry;
    const auto result = voxelIndexToPatientLps(Vec3{1, 2, 3}, geometry);
    QCOMPARE(result.status, CoordinateStatus::Ok);
    EXPECT_NEAR(result.value, 1, 2, 3);
}

void CoordinatesTest::appliesOriginAndAnisotropicSpacing() {
    VoxelGridGeometry geometry;
    geometry.originMm = Vec3{-10, 20, 5};
    geometry.spacingMm = Vec3{0.5, 0.5, 1.25};
    const auto result = voxelIndexToPatientLps(Vec3{4, 2, 8}, geometry);
    QCOMPARE(result.status, CoordinateStatus::Ok);
    EXPECT_NEAR(result.value, -8, 21, 15);
}

void CoordinatesTest::directionCosinesMapVoxelAxesIntoLps() {
    VoxelGridGeometry geometry;
    geometry.originMm = Vec3{5, 0, -1};
    geometry.spacingMm = Vec3{2, 1, 1};
    geometry.direction(0, 0) = 0;
    geometry.direction(1, 0) = 1;
    geometry.direction(0, 1) = 1;
    geometry.direction(1, 1) = 0;
    const auto result = voxelIndexToPatientLps(Vec3{2, 3, 4}, geometry);
    QCOMPARE(result.status, CoordinateStatus::Ok);
    EXPECT_NEAR(result.value, 8, 4, 3);
}

void CoordinatesTest::roundTripsPatientPointToVoxelIndex() {
    VoxelGridGeometry geometry;
    geometry.originMm = Vec3{5, 0, -1};
    geometry.spacingMm = Vec3{2, 1, 1};
    geometry.direction(0, 0) = 0;
    geometry.direction(1, 0) = 1;
    geometry.direction(0, 1) = 1;
    geometry.direction(1, 1) = 0;
    const Vec3 patient = voxelIndexToPatientLps(Vec3{2, 3, 4}, geometry).value;
    const auto inverse = patientLpsToVoxelIndex(patient, geometry);
    QCOMPARE(inverse.status, CoordinateStatus::Ok);
    EXPECT_NEAR(inverse.value, 2, 3, 4);
}

void CoordinatesTest::rejectsNonPositiveSpacing() {
    VoxelGridGeometry geometry;
    geometry.spacingMm = Vec3{0, 1, 1};
    QCOMPARE(voxelIndexToPatientLps(Vec3{0, 0, 0}, geometry).status,
             CoordinateStatus::NonPositiveSpacing);

    geometry.spacingMm = Vec3{1, -0.5, 1};
    QCOMPARE(patientLpsToVoxelIndex(Vec3{0, 0, 0}, geometry).status,
             CoordinateStatus::NonPositiveSpacing);
}

void CoordinatesTest::rejectsNonFiniteInput() {
    VoxelGridGeometry geometry;
    const double nan = std::numeric_limits<double>::quiet_NaN();
    QCOMPARE(voxelIndexToPatientLps(Vec3{nan, 0, 0}, geometry).status,
             CoordinateStatus::NonFiniteInput);

    geometry.spacingMm = Vec3{1, std::numeric_limits<double>::infinity(), 1};
    QCOMPARE(voxelIndexToPatientLps(Vec3{0, 0, 0}, geometry).status,
             CoordinateStatus::NonFiniteInput);
}

void CoordinatesTest::rejectsSingularDirection() {
    VoxelGridGeometry geometry;
    geometry.direction = Mat3{};
    QCOMPARE(voxelIndexToPatientLps(Vec3{1, 0, 0}, geometry).status,
             CoordinateStatus::SingularDirection);
    QCOMPARE(patientLpsToVoxelIndex(Vec3{1, 0, 0}, geometry).status,
             CoordinateStatus::SingularDirection);
}

QTEST_APPLESS_MAIN(CoordinatesTest)
#include "CoordinatesTest.moc"
