#include "medforge/core/Cancellation.h"
#include "medforge/geometry/MeshProcess.h"

#include <QtTest>
#include <cmath>
#include <filesystem>

using medforge::core::CancellationSource;
using medforge::geometry::MeshEditResult;
using medforge::geometry::MeshTopology;
using medforge::geometry::TriangleMesh;
using medforge::geometry::decimateMesh;
using medforge::geometry::fillSmallHoles;
using medforge::geometry::inspectMesh;
using medforge::geometry::readBinaryStl;
using medforge::geometry::smoothMesh;
using medforge::geometry::stlUnitLabel;
using medforge::geometry::writeBinaryStl;

namespace {

TriangleMesh cube(double size) {
    TriangleMesh mesh;
    mesh.positions = {{0, 0, 0}, {size, 0, 0}, {size, size, 0}, {0, size, 0},
                      {0, 0, size}, {size, 0, size}, {size, size, size}, {0, size, size}};
    mesh.indices = {4, 5, 6, 4, 6, 7, 0, 3, 2, 0, 2, 1, 3, 7, 6, 3, 6, 2,
                    0, 1, 5, 0, 5, 4, 1, 2, 6, 1, 6, 5, 0, 4, 7, 0, 7, 3};
    return mesh;
}

TriangleMesh sphere(double radius, int stacks, int slices) {
    TriangleMesh mesh;
    mesh.positions.push_back({0, 0, radius});
    for (int stack = 1; stack < stacks; ++stack) {
        const double v = 3.14159265358979323846 * stack / stacks;
        const double z = radius * std::cos(v);
        const double ring = radius * std::sin(v);
        for (int slice = 0; slice < slices; ++slice) {
            const double u = 6.28318530717958647692 * slice / slices;
            mesh.positions.push_back({ring * std::cos(u), ring * std::sin(u), z});
        }
    }
    mesh.positions.push_back({0, 0, -radius});
    const auto ringIndex = [slices](int stack, int slice) {
        return static_cast<std::uint32_t>(1 + (stack - 1) * slices + (slice % slices));
    };
    for (int slice = 0; slice < slices; ++slice) {
        mesh.indices.push_back(0);
        mesh.indices.push_back(ringIndex(1, slice + 1));
        mesh.indices.push_back(ringIndex(1, slice));
    }
    for (int stack = 1; stack < stacks - 1; ++stack) {
        for (int slice = 0; slice < slices; ++slice) {
            const std::uint32_t a = ringIndex(stack, slice);
            const std::uint32_t b = ringIndex(stack, slice + 1);
            const std::uint32_t c = ringIndex(stack + 1, slice);
            const std::uint32_t d = ringIndex(stack + 1, slice + 1);
            mesh.indices.insert(mesh.indices.end(), {a, b, d, a, d, c});
        }
    }
    const std::uint32_t bottom = static_cast<std::uint32_t>(mesh.positions.size() - 1);
    for (int slice = 0; slice < slices; ++slice) {
        mesh.indices.push_back(ringIndex(stacks - 1, slice));
        mesh.indices.push_back(ringIndex(stacks - 1, slice + 1));
        mesh.indices.push_back(bottom);
    }
    return mesh;
}

} // namespace

class MeshProcessTest : public QObject {
    Q_OBJECT

private slots:
    void closedCubeMatchesAnalyticalGeometry();
    void openAndNonManifoldEdgesAreReported();
    void smallHoleFillsAndLargeHoleIsRefused();
    void smoothingIsReversibleCopyAndDecimationShrinksTheSphere();
    void cancelledEditLeavesTheInputAlone();
    void binaryStlRoundTripsMillimeterGeometry();
};

void MeshProcessTest::closedCubeMatchesAnalyticalGeometry() {
    const TriangleMesh mesh = cube(10);
    const MeshTopology topology = inspectMesh(mesh);
    QCOMPARE(topology.triangleCount, 12);
    QCOMPARE(topology.componentCount, 1);
    QCOMPARE(topology.openEdgeCount, 0);
    QCOMPARE(topology.nonManifoldEdgeCount, 0);
    QVERIFY(topology.volumeValid);
    QVERIFY(std::abs(topology.volumeMm3 - 1000.0) < 1e-6);
    QVERIFY(std::abs(topology.surfaceAreaMm2 - 600.0) < 1e-6);
    QVERIFY(std::abs(topology.boundsMax.x - 10.0) < 1e-9);
    QCOMPARE(QString::fromUtf8(stlUnitLabel()), QStringLiteral("millimeters"));
}

void MeshProcessTest::openAndNonManifoldEdgesAreReported() {
    TriangleMesh open = cube(10);
    open.indices.resize(30);
    const MeshTopology opened = inspectMesh(open);
    QVERIFY(!opened.volumeValid);
    QVERIFY(opened.openEdgeCount > 0);
    QCOMPARE(opened.volumeMm3, 0.0);

    TriangleMesh parts;
    parts.positions = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {5, 0, 0}, {6, 0, 0}, {5, 1, 0}};
    parts.indices = {0, 1, 2, 3, 4, 5};
    QCOMPARE(inspectMesh(parts).componentCount, 2);

    TriangleMesh nonManifold;
    nonManifold.positions = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    nonManifold.indices = {0, 1, 2, 0, 1, 3, 0, 1, 2};
    QVERIFY(inspectMesh(nonManifold).nonManifoldEdgeCount >= 1);
}

void MeshProcessTest::smallHoleFillsAndLargeHoleIsRefused() {
    TriangleMesh damaged = cube(10);
    damaged.indices.resize(30);
    CancellationSource source;
    const MeshEditResult refused = fillSmallHoles(damaged, 3, source.token());
    QVERIFY(!refused.ok);
    QCOMPARE(static_cast<int>(refused.mesh.indices.size() / 3), 10);
    QVERIFY(QString::fromStdString(refused.message).contains(QStringLiteral("edge")));

    const MeshEditResult filled = fillSmallHoles(damaged, 8, source.token());
    QVERIFY2(filled.ok, filled.message.c_str());
    const MeshTopology topology = inspectMesh(filled.mesh);
    QVERIFY(topology.volumeValid);
    QCOMPARE(topology.triangleCount, 12);
    QVERIFY(std::abs(topology.volumeMm3 - 1000.0) < 1e-4);
    QCOMPARE(static_cast<int>(damaged.indices.size() / 3), 10);
}

void MeshProcessTest::smoothingIsReversibleCopyAndDecimationShrinksTheSphere() {
    const TriangleMesh original = cube(10);
    CancellationSource source;
    const MeshEditResult smoothed = smoothMesh(original, 1, 0.5, source.token());
    QVERIFY(smoothed.ok);
    QCOMPARE(static_cast<int>(smoothed.mesh.indices.size() / 3), 12);
    QVERIFY(inspectMesh(smoothed.mesh).volumeValid);
    QVERIFY(smoothed.mesh.positions.front().x > 0.1);
    QCOMPARE(original.positions.front().x, 0.0);

    const TriangleMesh round = sphere(10, 8, 12);
    const int before = static_cast<int>(round.indices.size() / 3);
    const MeshEditResult reduced = decimateMesh(round, 0.5, source.token());
    QVERIFY(reduced.ok);
    const int after = static_cast<int>(reduced.mesh.indices.size() / 3);
    QVERIFY(after < before);
    QVERIFY(after > 4);
    const MeshTopology topology = inspectMesh(reduced.mesh);
    QVERIFY(topology.boundsMax.x <= 10.0 + 1e-6);
    QVERIFY(topology.boundsMin.x >= -10.0 - 1e-6);
    QVERIFY(topology.boundsMax.x > 4.0);
    QCOMPARE(static_cast<int>(round.indices.size() / 3), before);
}

void MeshProcessTest::cancelledEditLeavesTheInputAlone() {
    CancellationSource source;
    source.cancel();
    const TriangleMesh original = cube(10);
    const MeshEditResult smoothed = smoothMesh(original, 20, 0.5, source.token());
    QVERIFY(smoothed.cancelled);
    QVERIFY(!smoothed.ok);
    QCOMPARE(smoothed.mesh.positions.front().x, 0.0);
}

void MeshProcessTest::binaryStlRoundTripsMillimeterGeometry() {
    const TriangleMesh original = cube(10);
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "medforge-m6-cube.stl";
    std::string error;
    QVERIFY2(writeBinaryStl(original, path, &error), error.c_str());
    TriangleMesh loaded;
    QVERIFY2(readBinaryStl(path, &loaded, &error), error.c_str());
    const MeshTopology topology = inspectMesh(loaded);
    QVERIFY(topology.volumeValid);
    QVERIFY(std::abs(topology.volumeMm3 - 1000.0) < 1e-3);
    QVERIFY(std::abs(topology.boundsMax.x - 10.0) < 1e-4);
    QVERIFY(std::abs(topology.surfaceAreaMm2 - 600.0) < 1e-2);
    std::filesystem::resize_file(path, 20);
    QVERIFY(!readBinaryStl(path, &loaded, &error));
    QVERIFY(!error.empty());
}

QTEST_APPLESS_MAIN(MeshProcessTest)
#include "MeshProcessTest.moc"
