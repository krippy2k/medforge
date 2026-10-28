#include "medforge/core/Cancellation.h"
#include "medforge/imaging/CtImport.h"
#include "medforge/imaging/DicomWrite.h"
#include "medforge/imaging/SyntheticPhantom.h"

#include <QtTest>
#include <filesystem>
#include <fstream>
#include <vector>

using medforge::imaging::DicomCtInstance;
using medforge::imaging::DicomReadStatus;
using medforge::imaging::ImportRequest;
using medforge::imaging::ImportStatus;
using medforge::imaging::importCtFolder;
using medforge::imaging::phantom::kAirHu;
using medforge::imaging::phantom::kMarkerHu;
using medforge::imaging::phantom::kMarkerI;
using medforge::imaging::phantom::kMarkerJ;
using medforge::imaging::phantom::kMarkerK;
using medforge::imaging::writePart10Ct;

namespace {

DicomCtInstance sliceAt(const std::string& uid, double zMm) {
    DicomCtInstance instance;
    instance.status = DicomReadStatus::Ok;
    instance.seriesInstanceUid = uid;
    instance.geometry.imageOrientationPatient = {1, 0, 0, 0, 1, 0};
    instance.geometry.imagePositionPatient = medforge::core::Vec3{0, 0, zMm};
    instance.geometry.rowSpacingMm = 1;
    instance.geometry.columnSpacingMm = 1;
    instance.rescale = medforge::imaging::Rescale{1, 0};
    instance.rows = 2;
    instance.columns = 2;
    instance.storedValues = {0, 0, 0, 0};
    return instance;
}

} // namespace

class CtImportTest : public QObject {
    Q_OBJECT

private slots:
    void importsTheSyntheticPhantom();
    void selectsTheLargestSeries();
    void cancelsBetweenFiles();
    void reportsACompressedSyntaxWithoutDecodingIt();
    void reportsADicomFileThatCannotBeRead();
    void rejectsADuplicateSeriesOnDisk();
    void writesTheDocumentedFixture();
};

void CtImportTest::importsTheSyntheticPhantom() {
    const std::filesystem::path folder =
        std::filesystem::temp_directory_path() / "medforge-m2-phantom";
    std::string error;
    QVERIFY2(medforge::imaging::phantom::writeSyntheticAxialSeries(folder, &error), error.c_str());
    medforge::core::CancellationSource source;
    int lastDone = 0;
    const auto imported = importCtFolder(
        ImportRequest{folder, 0}, source.token(), [&](int done, int total) {
            QVERIFY(done <= total);
            lastDone = done;
        });
    QCOMPARE(imported.status, ImportStatus::Ok);
    QCOMPARE(lastDone, imported.filesSeen);
    QVERIFY(imported.filesSeen >= 4);
    QVERIFY(std::abs(imported.build.volume.huAt(1, 0, 0) - static_cast<float>(kAirHu)) < 0.01F);
    QVERIFY(std::abs(imported.build.volume.huAt(0, 0, 0) - 80.0F) < 0.01F);
    QVERIFY(std::abs(imported.build.volume.huAt(kMarkerI, kMarkerJ, kMarkerK) -
                     static_cast<float>(kMarkerHu)) < 0.01F);
}

void CtImportTest::selectsTheLargestSeries() {
    const std::filesystem::path folder =
        std::filesystem::temp_directory_path() / "medforge-m2-largest";
    std::filesystem::remove_all(folder);
    std::filesystem::create_directories(folder);
    std::string error;
    QVERIFY(writePart10Ct(folder / "small-0.dcm", sliceAt("1.2.9.1", 0), 1, &error));
    QVERIFY(writePart10Ct(folder / "small-1.dcm", sliceAt("1.2.9.1", 1), 2, &error));
    for (int index = 0; index < 3; ++index) {
        QVERIFY(writePart10Ct(folder / ("large-" + std::to_string(index) + ".dcm"),
                              sliceAt("1.2.9.2", index),
                              index + 1,
                              &error));
    }
    medforge::core::CancellationSource source;
    const auto imported = importCtFolder(ImportRequest{folder, 0}, source.token(), {});
    QCOMPARE(imported.status, ImportStatus::Ok);
    QCOMPARE(imported.build.volume.seriesInstanceUid, std::string("1.2.9.2"));
    QCOMPARE(imported.build.volume.slices, 3);
}

void CtImportTest::cancelsBetweenFiles() {
    const std::filesystem::path folder =
        std::filesystem::temp_directory_path() / "medforge-m2-cancel";
    std::string error;
    QVERIFY(medforge::imaging::phantom::writeSyntheticAxialSeries(folder, &error));
    medforge::core::CancellationSource source;
    source.cancel();
    const auto imported = importCtFolder(ImportRequest{folder, 0}, source.token(), {});
    QCOMPARE(imported.status, ImportStatus::Cancelled);
    QCOMPARE(imported.message, std::string("Import cancelled"));
    QCOMPARE(imported.filesSeen, 0);
}

void CtImportTest::reportsACompressedSyntaxWithoutDecodingIt() {
    const std::filesystem::path folder =
        std::filesystem::temp_directory_path() / "medforge-m2-compressed";
    std::filesystem::remove_all(folder);
    std::filesystem::create_directories(folder);
    auto instance = sliceAt("1.2.9.3", 0);
    instance.transferSyntaxUid = "1.2.840.10008.1.2.4.70";
    std::string error;
    QVERIFY(writePart10Ct(folder / "jpeg.dcm", instance, 1, &error));
    medforge::core::CancellationSource source;
    const auto imported = importCtFolder(ImportRequest{folder, 0}, source.token(), {});
    QCOMPARE(imported.status, ImportStatus::Rejected);
    QCOMPARE(imported.message,
             std::string("Compressed transfer syntax is recognized but not decoded"));
}

void CtImportTest::reportsADicomFileThatCannotBeRead() {
    const std::filesystem::path folder =
        std::filesystem::temp_directory_path() / "medforge-m2-unreadable-dicom";
    std::filesystem::remove_all(folder);
    std::filesystem::create_directories(folder);
    std::vector<std::uint8_t> bytes(128, 0);
    bytes.insert(bytes.end(), {'D', 'I', 'C', 'M'});
    bytes.push_back(0);
    std::ofstream output(folder / "broken.dcm", std::ios::binary);
    output.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
    output.close();

    medforge::core::CancellationSource source;
    const auto imported = importCtFolder(ImportRequest{folder, 0}, source.token(), {});
    QCOMPARE(imported.status, ImportStatus::Rejected);
    QCOMPARE(imported.message, std::string("A DICOM file was found, but it could not be parsed"));
}

void CtImportTest::rejectsADuplicateSeriesOnDisk() {
    const std::filesystem::path folder =
        std::filesystem::temp_directory_path() / "medforge-m2-duplicate";
    std::filesystem::remove_all(folder);
    std::filesystem::create_directories(folder);
    std::string error;
    QVERIFY(writePart10Ct(folder / "a.dcm", sliceAt("1.2.9.4", 0), 1, &error));
    QVERIFY(writePart10Ct(folder / "b.dcm", sliceAt("1.2.9.4", 0), 2, &error));
    medforge::core::CancellationSource source;
    const auto imported = importCtFolder(ImportRequest{folder, 0}, source.token(), {});
    QCOMPARE(imported.status, ImportStatus::Rejected);
    QCOMPARE(imported.message, std::string("Duplicate slice positions"));
}

void CtImportTest::writesTheDocumentedFixture() {
    const std::filesystem::path folder =
        std::filesystem::path(MEDFORGE_SOURCE_DIR) / "tests" / "fixtures" / "synthetic-ct";
    std::string error;
    QVERIFY2(medforge::imaging::phantom::writeSyntheticAxialSeries(folder, &error), error.c_str());
    medforge::core::CancellationSource source;
    const auto imported = importCtFolder(ImportRequest{folder, 0}, source.token(), {});
    QCOMPARE(imported.status, ImportStatus::Ok);
    QCOMPARE(imported.build.volume.seriesInstanceUid,
             std::string(medforge::imaging::phantom::kSeriesUid));
}

QTEST_APPLESS_MAIN(CtImportTest)
#include "CtImportTest.moc"
