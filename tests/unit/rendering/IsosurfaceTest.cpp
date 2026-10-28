#include "medforge/core/Cancellation.h"
#include "medforge/rendering/Isosurface.h"
#include "medforge/rendering/TransferFunction.h"

#include <QtTest>
#include <cmath>

using medforge::geometry::ScalarGrid;
using medforge::rendering::SurfaceStatus;
using medforge::rendering::boneTransfer;
using medforge::rendering::extractIsosurface;
using medforge::rendering::lungTransfer;
using medforge::rendering::opacityAt;

namespace {

ScalarGrid sphereGrid(const medforge::core::Vec3& spacing, double radiusMm) {
    ScalarGrid grid;
    const int size = 24;
    grid.columns = size;
    grid.rows = size;
    grid.slices = static_cast<int>(std::ceil((radiusMm * 2.0 + 4.0) / spacing.z));
    if (grid.slices < 8) {
        grid.slices = 8;
    }
    grid.geometry.spacingMm = spacing;
    grid.geometry.originMm = {-(size - 1) * spacing.x * 0.5, -(size - 1) * spacing.y * 0.5,
                              -(grid.slices - 1) * spacing.z * 0.5};
    grid.values.assign(static_cast<std::size_t>(grid.columns * grid.rows * grid.slices), -1000.0f);
    const double radiusSquared = radiusMm * radiusMm;
    for (int k = 0; k < grid.slices; ++k) {
        for (int j = 0; j < grid.rows; ++j) {
            for (int i = 0; i < grid.columns; ++i) {
                const double x = grid.geometry.originMm.x + (i * spacing.x);
                const double y = grid.geometry.originMm.y + (j * spacing.y);
                const double z = grid.geometry.originMm.z + (k * spacing.z);
                const std::size_t index =
                    (static_cast<std::size_t>(k) * static_cast<std::size_t>(grid.rows) +
                     static_cast<std::size_t>(j)) *
                        static_cast<std::size_t>(grid.columns) +
                    static_cast<std::size_t>(i);
                if ((x * x) + (y * y) + (z * z) <= radiusSquared) {
                    grid.values[index] = 400.0f;
                }
            }
        }
    }
    return grid;
}

} // namespace

class IsosurfaceTest : public QObject {
    Q_OBJECT

private slots:
    void boneIsOpaqueWhereLungIsNot();
    void cancelledTokenExtractsNothing();
    void sphereVolumeAndRadiusMatchThePhantom();
    void anisotropicSpacingKeepsAPhysicalSphere();
    void repeatedExtractionIsStable();
};

void IsosurfaceTest::boneIsOpaqueWhereLungIsNot() {
    QVERIFY(opacityAt(boneTransfer(), -1000) == 0.0);
    QVERIFY(opacityAt(boneTransfer(), 800) > 0.8);
    QVERIFY(opacityAt(lungTransfer(), -600) > opacityAt(boneTransfer(), -600));
}

void IsosurfaceTest::cancelledTokenExtractsNothing() {
    medforge::core::CancellationSource source;
    source.cancel();
    const auto result = extractIsosurface(sphereGrid({1, 1, 1}, 6), 0, source.token());
    QCOMPARE(result.status, SurfaceStatus::Cancelled);
    QCOMPARE(result.statistics.triangleCount, 0);
}

void IsosurfaceTest::sphereVolumeAndRadiusMatchThePhantom() {
#ifndef MEDFORGE_HAS_VTK
    QSKIP("VTK is required to extract an isosurface");
#else
    medforge::core::CancellationSource source;
    const auto result = extractIsosurface(sphereGrid({1, 1, 1}, 6), 0, source.token());
    QCOMPARE(result.status, SurfaceStatus::Ok);
    const double expected = 4.0 / 3.0 * 3.141592653589793 * 216.0;
    QVERIFY2(std::abs(result.statistics.volumeMm3 - expected) < expected * 0.15,
             qPrintable(QString::number(result.statistics.volumeMm3)));
    QVERIFY(std::abs(result.statistics.meanRadiusMm - 6.0) < 0.8);
    QVERIFY(std::abs(result.statistics.centroid.x) < 1.0);
    QVERIFY(std::abs(result.statistics.centroid.y) < 1.0);
    QVERIFY(std::abs(result.statistics.centroid.z) < 1.0);
    int outward = 0;
    int considered = 0;
    for (std::size_t index = 0; index < result.mesh.positions.size(); ++index) {
        const auto& position = result.mesh.positions[index];
        const double radius = std::sqrt((position.x * position.x) + (position.y * position.y) +
                                        (position.z * position.z));
        if (radius < 2.0) {
            continue;
        }
        ++considered;
        const auto& normal = result.mesh.normals[index];
        if ((normal.x * position.x) + (normal.y * position.y) + (normal.z * position.z) > 0) {
            ++outward;
        }
    }
    QVERIFY(considered > 20);
    QVERIFY(outward > considered * 0.9);
#endif
}

void IsosurfaceTest::anisotropicSpacingKeepsAPhysicalSphere() {
#ifndef MEDFORGE_HAS_VTK
    QSKIP("VTK is required to extract an isosurface");
#else
    medforge::core::CancellationSource source;
    const auto result = extractIsosurface(sphereGrid({1, 1, 2}, 6), 0, source.token());
    QCOMPARE(result.status, SurfaceStatus::Ok);
    double maxZ = 0;
    double poleNormal = -1;
    double poleRadius = 1e9;
    for (std::size_t index = 0; index < result.mesh.positions.size(); ++index) {
        const auto& position = result.mesh.positions[index];
        maxZ = std::max(maxZ, std::abs(position.z));
        if (position.z < 4.0) {
            continue;
        }
        const double radial = std::sqrt((position.x * position.x) + (position.y * position.y));
        if (radial < poleRadius) {
            poleRadius = radial;
            poleNormal = result.mesh.normals[index].z;
        }
    }
    QVERIFY2(maxZ > 5.0 && maxZ < 7.0, qPrintable(QString::number(maxZ)));
    QVERIFY2(poleNormal > 0.4, qPrintable(QString::number(poleNormal)));
    QVERIFY(std::abs(result.statistics.meanRadiusMm - 6.0) < 1.0);
#endif
}

void IsosurfaceTest::repeatedExtractionIsStable() {
#ifndef MEDFORGE_HAS_VTK
    QSKIP("VTK is required to extract an isosurface");
#else
    const ScalarGrid grid = sphereGrid({1, 1, 1}, 6);
    medforge::core::CancellationSource first;
    medforge::core::CancellationSource second;
    const auto once = extractIsosurface(grid, 0, first.token());
    const auto twice = extractIsosurface(grid, 0, second.token());
    QCOMPARE(once.status, SurfaceStatus::Ok);
    QCOMPARE(twice.statistics.triangleCount, once.statistics.triangleCount);
    QVERIFY(twice.elapsedMs >= 0);
    QVERIFY(twice.volumeBytes > 0);
#endif
}

QTEST_APPLESS_MAIN(IsosurfaceTest)
#include "IsosurfaceTest.moc"
