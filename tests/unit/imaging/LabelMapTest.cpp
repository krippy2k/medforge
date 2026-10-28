#include "medforge/imaging/LabelMap.h"

#include <QtTest>
#include <filesystem>

using medforge::core::CancellationSource;
using medforge::core::CommandHistory;
using medforge::imaging::BrushStamp;
using medforge::imaging::Connectivity;
using medforge::imaging::CtVolume;
using medforge::imaging::IndexBounds;
using medforge::imaging::LabelEdit;
using medforge::imaging::LabelMap;
using medforge::imaging::LabelMapCommand;
using medforge::imaging::MprPlane;
using medforge::imaging::brushEdit;
using medforge::imaging::countLabel;
using medforge::imaging::emptyLabelMap;
using medforge::imaging::loadLabelMap;
using medforge::imaging::regionGrowEdit;
using medforge::imaging::saveLabelMap;
using medforge::imaging::thresholdEdit;

namespace {

CtVolume grid(int columns, int rows, int slices, double sx, double sy, double sz) {
    CtVolume volume;
    volume.seriesInstanceUid = "1.2.3.4.5";
    volume.columns = columns;
    volume.rows = rows;
    volume.slices = slices;
    volume.geometry.geometry.spacingMm = {sx, sy, sz};
    volume.geometry.geometry.originMm = {10, -20, 0};
    volume.hu.assign(static_cast<std::size_t>(columns * rows * slices), -1000.0F);
    return volume;
}

void setHu(CtVolume& volume, int i, int j, int k, float hu) {
    const std::size_t index = (static_cast<std::size_t>(k) * static_cast<std::size_t>(volume.rows) +
                               static_cast<std::size_t>(j)) *
                                  static_cast<std::size_t>(volume.columns) +
                              static_cast<std::size_t>(i);
    volume.hu[index] = hu;
}

} // namespace

class LabelMapTest : public QObject {
    Q_OBJECT

private slots:
    void thresholdKeepsOnlyTheRequestedRange();
    void regionGrowStaysInsideOneComponent();
    void diagonalVoxelsNeed26Connectivity();
    void growBoundsStopLeakage();
    void otherLabelsBlockTheGrow();
    void cancelledGrowWritesNothing();
    void brushRespectsSliceEdgesAndAnisotropicSpacing();
    void undoAndRedoRestoreTheExactMask();
    void saveAndReloadKeepEveryVoxel();
};

void LabelMapTest::thresholdKeepsOnlyTheRequestedRange() {
    CtVolume volume = grid(4, 1, 1, 1, 1, 1);
    setHu(volume, 0, 0, 0, 80);
    setHu(volume, 1, 0, 0, 400);
    setHu(volume, 2, 0, 0, 1200);
    LabelMap map = emptyLabelMap(volume);
    const LabelEdit edit = thresholdEdit(volume, map, 1, 300, 500);
    applyLabelEdit(map, edit);
    QCOMPARE(map.at(0, 0, 0), static_cast<std::uint8_t>(0));
    QCOMPARE(map.at(1, 0, 0), static_cast<std::uint8_t>(1));
    QCOMPARE(map.at(2, 0, 0), static_cast<std::uint8_t>(0));
}

void LabelMapTest::regionGrowStaysInsideOneComponent() {
    CtVolume volume = grid(5, 1, 1, 1, 1, 1);
    setHu(volume, 0, 0, 0, 400);
    setHu(volume, 1, 0, 0, 400);
    setHu(volume, 3, 0, 0, 400);
    LabelMap map = emptyLabelMap(volume);
    CancellationSource source;
    const auto grown = regionGrowEdit(volume, map, 0, 0, 0, 1, 300, 500, Connectivity::Faces6, {},
                                      source.token());
    QVERIFY(grown.message.empty());
    applyLabelEdit(map, grown.edit);
    QCOMPARE(countLabel(map, 1), static_cast<std::size_t>(2));
    QCOMPARE(map.at(3, 0, 0), static_cast<std::uint8_t>(0));
}

void LabelMapTest::diagonalVoxelsNeed26Connectivity() {
    CtVolume volume = grid(2, 2, 1, 1, 1, 1);
    setHu(volume, 0, 0, 0, 400);
    setHu(volume, 1, 1, 0, 400);
    LabelMap map = emptyLabelMap(volume);
    CancellationSource source;
    const auto faces = regionGrowEdit(volume, map, 0, 0, 0, 1, 300, 500, Connectivity::Faces6, {},
                                      source.token());
    applyLabelEdit(map, faces.edit);
    QCOMPARE(countLabel(map, 1), static_cast<std::size_t>(1));
    LabelMap wide = emptyLabelMap(volume);
    const auto full = regionGrowEdit(volume, wide, 0, 0, 0, 1, 300, 500, Connectivity::Neighborhood26,
                                     {}, source.token());
    applyLabelEdit(wide, full.edit);
    QCOMPARE(countLabel(wide, 1), static_cast<std::size_t>(2));
}

void LabelMapTest::growBoundsStopLeakage() {
    CtVolume volume = grid(4, 1, 1, 1, 1, 1);
    for (int i = 0; i < 4; ++i) {
        setHu(volume, i, 0, 0, 400);
    }
    LabelMap map = emptyLabelMap(volume);
    IndexBounds bounds;
    bounds.limited = true;
    bounds.iMin = 0;
    bounds.iMax = 1;
    bounds.jMin = 0;
    bounds.jMax = 0;
    bounds.kMin = 0;
    bounds.kMax = 0;
    CancellationSource source;
    const auto grown = regionGrowEdit(volume, map, 0, 0, 0, 1, 300, 500, Connectivity::Faces6, bounds,
                                      source.token());
    applyLabelEdit(map, grown.edit);
    QCOMPARE(map.at(1, 0, 0), static_cast<std::uint8_t>(1));
    QCOMPARE(map.at(2, 0, 0), static_cast<std::uint8_t>(0));
}

void LabelMapTest::otherLabelsBlockTheGrow() {
    CtVolume volume = grid(3, 1, 1, 1, 1, 1);
    setHu(volume, 0, 0, 0, 400);
    setHu(volume, 1, 0, 0, 400);
    setHu(volume, 2, 0, 0, 400);
    LabelMap map = emptyLabelMap(volume);
    applyLabelEdit(map, thresholdEdit(volume, map, 2, 300, 500));
    map.voxels[0] = 0;
    map.voxels[2] = 0;
    CancellationSource source;
    const auto grown = regionGrowEdit(volume, map, 0, 0, 0, 1, 300, 500, Connectivity::Faces6, {},
                                      source.token());
    applyLabelEdit(map, grown.edit);
    QCOMPARE(map.at(0, 0, 0), static_cast<std::uint8_t>(1));
    QCOMPARE(map.at(1, 0, 0), static_cast<std::uint8_t>(2));
    QCOMPARE(map.at(2, 0, 0), static_cast<std::uint8_t>(0));
}

void LabelMapTest::cancelledGrowWritesNothing() {
    CtVolume volume = grid(3, 1, 1, 1, 1, 1);
    setHu(volume, 0, 0, 0, 400);
    setHu(volume, 1, 0, 0, 400);
    LabelMap map = emptyLabelMap(volume);
    CancellationSource source;
    source.cancel();
    const auto grown = regionGrowEdit(volume, map, 0, 0, 0, 1, 300, 500, Connectivity::Faces6, {},
                                      source.token(), 50);
    QVERIFY(grown.cancelled);
    QVERIFY(grown.edit.changes.empty());
    applyLabelEdit(map, grown.edit);
    QCOMPARE(countLabel(map, 1), static_cast<std::size_t>(0));
}

void LabelMapTest::brushRespectsSliceEdgesAndAnisotropicSpacing() {
    CtVolume volume = grid(4, 4, 2, 0.8, 0.5, 2);
    LabelMap map = emptyLabelMap(volume);
    BrushStamp stamp;
    stamp.plane = MprPlane::Axial;
    stamp.slice = 0;
    stamp.x = 1;
    stamp.y = 1;
    stamp.radiusMm = 0.6;
    stamp.label = 1;
    applyLabelEdit(map, brushEdit(map, stamp));
    QCOMPARE(map.at(1, 1, 0), static_cast<std::uint8_t>(1));
    QCOMPARE(map.at(1, 2, 0), static_cast<std::uint8_t>(1));
    QCOMPARE(map.at(2, 1, 0), static_cast<std::uint8_t>(0));
    QCOMPARE(map.at(1, 1, 1), static_cast<std::uint8_t>(0));

    BrushStamp edge;
    edge.plane = MprPlane::Axial;
    edge.slice = 1;
    edge.x = 0;
    edge.y = 0;
    edge.radiusMm = 5;
    edge.label = 1;
    applyLabelEdit(map, brushEdit(map, edge));
    QCOMPARE(map.at(0, 0, 1), static_cast<std::uint8_t>(1));
    QCOMPARE(map.at(0, 0, 0), static_cast<std::uint8_t>(0));
    QCOMPARE(map.at(3, 3, 1), static_cast<std::uint8_t>(1));
}

void LabelMapTest::undoAndRedoRestoreTheExactMask() {
    CtVolume volume = grid(2, 1, 1, 1, 1, 1);
    setHu(volume, 0, 0, 0, 400);
    LabelMap map = emptyLabelMap(volume);
    CommandHistory history;
    QVERIFY(history.execute(
        std::make_unique<LabelMapCommand>(&map, thresholdEdit(volume, map, 1, 300, 500), "threshold")));
    QCOMPARE(map.at(0, 0, 0), static_cast<std::uint8_t>(1));
    QVERIFY(history.undo());
    QCOMPARE(map.at(0, 0, 0), static_cast<std::uint8_t>(0));
    QVERIFY(history.redo());
    QCOMPARE(map.at(0, 0, 0), static_cast<std::uint8_t>(1));
}

void LabelMapTest::saveAndReloadKeepEveryVoxel() {
    CtVolume volume = grid(2, 2, 2, 0.8, 0.5, 2);
    setHu(volume, 1, 0, 1, 400);
    LabelMap map = emptyLabelMap(volume);
    applyLabelEdit(map, thresholdEdit(volume, map, 1, 300, 500));
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "medforge-m4-mask.bin";
    std::string error;
    QVERIFY2(saveLabelMap(map, path, &error), error.c_str());
    LabelMap loaded;
    QVERIFY2(loadLabelMap(path, &loaded, &error), error.c_str());
    QCOMPARE(loaded.seriesInstanceUid, map.seriesInstanceUid);
    QCOMPARE(loaded.columns, map.columns);
    QCOMPARE(loaded.rows, map.rows);
    QCOMPARE(loaded.slices, map.slices);
    QCOMPARE(loaded.voxels, map.voxels);
    QVERIFY(std::abs(loaded.geometry.spacingMm.y - 0.5) < 1e-9);
    QVERIFY(std::abs(loaded.geometry.originMm.y - (-20.0)) < 1e-9);
}

QTEST_APPLESS_MAIN(LabelMapTest)
#include "LabelMapTest.moc"
