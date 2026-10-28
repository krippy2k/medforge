#include "medforge/geometry/Surface.h"

#include <QtTest>
#include <cmath>

using medforge::geometry::MeshStatistics;
using medforge::geometry::TriangleMesh;
using medforge::geometry::clipHalfSpace;
using medforge::geometry::measureMesh;

class SurfaceTest : public QObject {
    Q_OBJECT

private slots:
    void tetrahedronVolumeIsOneSixth();
    void clipKeepsTheNegativeHalfSpace();
};

void SurfaceTest::tetrahedronVolumeIsOneSixth() {
    TriangleMesh mesh;
    mesh.positions = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    mesh.normals = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    mesh.indices = {0, 1, 2};
    const MeshStatistics stats = measureMesh(mesh);
    QCOMPARE(stats.triangleCount, 1);
    QCOMPARE(stats.vertexCount, 3);
    QVERIFY(std::abs(stats.volumeMm3 - (1.0 / 6.0)) < 1e-9);
    QVERIFY(stats.boundsMax.x == 1.0);
    QVERIFY(stats.boundsMin.z == 0.0);
}

void SurfaceTest::clipKeepsTheNegativeHalfSpace() {
    TriangleMesh mesh;
    mesh.positions = {{-2, 0, 0}, {-1, 1, 0}, {-1, 0, 1}, {2, 0, 0}, {1, 1, 0}, {1, 0, 1}};
    mesh.normals = {{0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1}};
    mesh.indices = {0, 1, 2, 3, 4, 5};
    const TriangleMesh clipped = clipHalfSpace(mesh, {0, 0, 0}, {1, 0, 0});
    const MeshStatistics stats = measureMesh(clipped);
    QCOMPARE(stats.triangleCount, 1);
    QVERIFY(stats.boundsMax.x <= 1e-6);
    QVERIFY(stats.boundsMin.x < 0);
}

QTEST_APPLESS_MAIN(SurfaceTest)
#include "SurfaceTest.moc"
