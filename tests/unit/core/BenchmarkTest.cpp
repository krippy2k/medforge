#include "medforge/core/Cancellation.h"
#include "medforge/geometry/MeshProcess.h"
#include "medforge/imaging/CtImport.h"
#include "medforge/imaging/LabelMap.h"
#include "medforge/imaging/SyntheticPhantom.h"
#include "medforge/rendering/Isosurface.h"

#include <QtTest>
#include <chrono>
#include <filesystem>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
// clang-format off
#include <windows.h>
#include <psapi.h>
// clang-format on
#ifdef _MSC_VER
#pragma comment(lib, "psapi.lib")
#endif
#endif

namespace {

qint64 elapsedMs(std::chrono::steady_clock::time_point started) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() -
                                                                 started)
        .count();
}

} // namespace

class BenchmarkTest : public QObject {
    Q_OBJECT

private slots:
    void phantomImportSegmentSurfaceAndExport();
};

void BenchmarkTest::phantomImportSegmentSurfaceAndExport() {
    const auto folder = std::filesystem::temp_directory_path() / "medforge-m8-benchmark";
    std::filesystem::remove_all(folder);
    std::filesystem::create_directories(folder);
    std::string error;
    QVERIFY(medforge::imaging::phantom::writeSyntheticAxialSeries(folder, &error));

    medforge::core::CancellationSource cancellation;
    auto started = std::chrono::steady_clock::now();
    const medforge::imaging::ImportResult imported = medforge::imaging::importCtFolder(
        medforge::imaging::ImportRequest{folder, 0}, cancellation.token(), {});
    const qint64 importMs = elapsedMs(started);
    QCOMPARE(imported.status, medforge::imaging::ImportStatus::Ok);
    QVERIFY(importMs < 15000);

    const medforge::imaging::CtVolume& volume = imported.build.volume;
    medforge::imaging::LabelMap labels = medforge::imaging::emptyLabelMap(volume);
    started = std::chrono::steady_clock::now();
    medforge::imaging::applyLabelEdit(
        labels, medforge::imaging::thresholdEdit(volume, labels, 1, 200.f, 3000.f));
    const qint64 segmentMs = elapsedMs(started);
    QCOMPARE(labels.at(3, 2, 1), static_cast<std::uint8_t>(1));
    QVERIFY(segmentMs < 15000);

    medforge::geometry::ScalarGrid grid;
    grid.columns = volume.columns;
    grid.rows = volume.rows;
    grid.slices = volume.slices;
    grid.values = volume.hu;
    grid.geometry = volume.geometry.geometry;
    started = std::chrono::steady_clock::now();
    const medforge::rendering::SurfaceResult surface =
        medforge::rendering::extractIsosurface(grid, 0, cancellation.token(), 0);
    const qint64 surfaceMs = elapsedMs(started);
    QVERIFY(surface.status == medforge::rendering::SurfaceStatus::Ok ||
            surface.status == medforge::rendering::SurfaceStatus::Unavailable);
    if (surface.status == medforge::rendering::SurfaceStatus::Ok) {
        QVERIFY(surface.statistics.triangleCount > 0);
        QVERIFY(surfaceMs < 30000);
        const auto stl = folder / "surface.stl";
        started = std::chrono::steady_clock::now();
        QVERIFY(medforge::geometry::writeBinaryStl(
            surface.mesh,
            stl,
            &error,
            "MedForge STL millimeters. Research prototype. Not for manufacture."));
        const qint64 exportMs = elapsedMs(started);
        QVERIFY(exportMs < 15000);
        medforge::geometry::TriangleMesh readBack;
        QVERIFY(medforge::geometry::readBinaryStl(stl, &readBack, &error));
        QCOMPARE(readBack.indices.size(), surface.mesh.indices.size());
        qInfo(
            "benchmark import=%lld ms segment=%lld ms surface=%lld ms export=%lld ms triangles=%d",
            static_cast<long long>(importMs),
            static_cast<long long>(segmentMs),
            static_cast<long long>(surfaceMs),
            static_cast<long long>(exportMs),
            surface.statistics.triangleCount);
    } else {
        qInfo("benchmark import=%lld ms segment=%lld ms surface=unavailable",
              static_cast<long long>(importMs),
              static_cast<long long>(segmentMs));
    }

#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS counters{};
    QVERIFY(GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)));
    qInfo("benchmark workingSet=%llu bytes",
          static_cast<unsigned long long>(counters.WorkingSetSize));
    QVERIFY(counters.WorkingSetSize < static_cast<SIZE_T>(2ull << 30));
#endif
}

QTEST_APPLESS_MAIN(BenchmarkTest)
#include "BenchmarkTest.moc"
