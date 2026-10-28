#include "medforge/imaging/DicomRead.h"

#include <QtTest>
#include <cstdint>
#include <cstring>
#include <vector>

using medforge::imaging::DicomReadStatus;
using medforge::imaging::readUncompressedCt;

namespace {

void appendBytes(std::vector<std::uint8_t>& out, const void* data, std::size_t size) {
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    out.insert(out.end(), bytes, bytes + size);
}

void appendU16(std::vector<std::uint8_t>& out, std::uint16_t value) {
    appendBytes(out, &value, sizeof(value));
}

void appendU32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    appendBytes(out, &value, sizeof(value));
}

void appendTag(std::vector<std::uint8_t>& out, std::uint16_t group, std::uint16_t element) {
    appendU16(out, group);
    appendU16(out, element);
}

void appendText(std::vector<std::uint8_t>& out, std::uint16_t group, std::uint16_t element,
                const char* vr, std::string value) {
    if (value.size() % 2 != 0) {
        value.push_back(value.empty() || vr[0] == 'U' ? '\0' : ' ');
    }
    appendTag(out, group, element);
    out.push_back(static_cast<std::uint8_t>(vr[0]));
    out.push_back(static_cast<std::uint8_t>(vr[1]));
    appendU16(out, static_cast<std::uint16_t>(value.size()));
    appendBytes(out, value.data(), value.size());
}

void appendUndefinedSequence(std::vector<std::uint8_t>& out) {
    appendTag(out, 0x0008, 0x1140);
    out.push_back('S');
    out.push_back('Q');
    appendU16(out, 0);
    appendU32(out, 0xFFFFFFFFU);
    appendTag(out, 0xFFFE, 0xE000);
    appendU32(out, 0xFFFFFFFFU);
    appendTag(out, 0xFFFE, 0xE00D);
    appendU32(out, 0);
    appendTag(out, 0xFFFE, 0xE0DD);
    appendU32(out, 0);
}

void appendU16Element(std::vector<std::uint8_t>& out, std::uint16_t group, std::uint16_t element,
                      std::uint16_t value) {
    appendTag(out, group, element);
    out.push_back('U');
    out.push_back('S');
    appendU16(out, 2);
    appendU16(out, value);
}

void appendImplicitText(std::vector<std::uint8_t>& out, std::uint16_t group, std::uint16_t element,
                        std::string value) {
    if (value.size() % 2 != 0) {
        value.push_back(' ');
    }
    appendTag(out, group, element);
    appendU32(out, static_cast<std::uint32_t>(value.size()));
    appendBytes(out, value.data(), value.size());
}

void appendImplicitU16(std::vector<std::uint8_t>& out, std::uint16_t group, std::uint16_t element,
                       std::uint16_t value) {
    appendTag(out, group, element);
    appendU32(out, 2);
    appendU16(out, value);
}

std::vector<std::uint8_t> syntheticExplicitCt(const char* transferSyntax,
                                              const std::vector<std::int16_t>& pixels) {
    std::vector<std::uint8_t> meta;
    appendText(meta, 0x0002, 0x0002, "UI", "1.2.840.10008.5.1.4.1.1.2");
    appendText(meta, 0x0002, 0x0003, "UI", "1.2.826.0.1.3680043.10.543.1");
    appendText(meta, 0x0002, 0x0010, "UI", transferSyntax);
    appendText(meta, 0x0002, 0x0012, "UI", "1.2.826.0.1.3680043.10.543");

    std::vector<std::uint8_t> file(128, 0);
    file.insert(file.end(), {'D', 'I', 'C', 'M'});
    appendTag(file, 0x0002, 0x0000);
    file.push_back('U');
    file.push_back('L');
    appendU16(file, 4);
    appendU32(file, static_cast<std::uint32_t>(meta.size()));
    file.insert(file.end(), meta.begin(), meta.end());

    appendText(file, 0x0008, 0x0060, "CS", "CT");
    appendText(file, 0x0020, 0x000E, "UI", "1.2.826.0.1.3680043.10.543.2");
    appendText(file, 0x0020, 0x0032, "DS", "-10\\20\\-30");
    appendText(file, 0x0020, 0x0037, "DS", "1\\0\\0\\0\\1\\0");
    appendU16Element(file, 0x0028, 0x0010, 2);
    appendU16Element(file, 0x0028, 0x0011, 2);
    appendText(file, 0x0028, 0x0030, "DS", "0.5\\0.8");
    appendU16Element(file, 0x0028, 0x0100, 16);
    appendU16Element(file, 0x0028, 0x0103, 1);
    appendText(file, 0x0028, 0x1052, "DS", "-1024");
    appendText(file, 0x0028, 0x1053, "DS", "2");
    appendTag(file, 0x7FE0, 0x0010);
    file.push_back('O');
    file.push_back('W');
    appendU16(file, 0);
    appendU32(file, static_cast<std::uint32_t>(pixels.size() * sizeof(std::int16_t)));
    appendBytes(file, pixels.data(), pixels.size() * sizeof(std::int16_t));
    return file;
}

} // namespace

class DicomReadTest : public QObject {
    Q_OBJECT

private slots:
    void readsExplicitLittleEndianCt();
    void readsImplicitLittleEndianCt();
    void readsASequenceAndPaddedDecimalStrings();
    void readsAnImplicitSequence();
    void readsNativePixelsAfterAnEmptyOffsetTable();
    void readsADatasetWithoutAFilePreamble();
    void rejectsCompressedTransferSyntax();
    void rejectsATruncatedFile();
    void rejectsAFileThatIsNotDicom();
};

void DicomReadTest::readsExplicitLittleEndianCt() {
    const std::vector<std::int16_t> pixels{0, 24, -12, 100};
    const auto bytes = syntheticExplicitCt("1.2.840.10008.1.2.1", pixels);
    const auto instance = readUncompressedCt(bytes);
    QCOMPARE(instance.status, DicomReadStatus::Ok);
    QCOMPARE(instance.seriesInstanceUid, std::string("1.2.826.0.1.3680043.10.543.2"));
    QCOMPARE(instance.rows, 2);
    QCOMPARE(instance.columns, 2);
    QCOMPARE(instance.storedValues, pixels);
    QVERIFY(std::abs(instance.geometry.imagePositionPatient.x + 10) < 1e-6);
    QVERIFY(std::abs(instance.geometry.imagePositionPatient.y - 20) < 1e-6);
    QVERIFY(std::abs(instance.geometry.imagePositionPatient.z + 30) < 1e-6);
    QVERIFY(std::abs(instance.geometry.rowSpacingMm - 0.5) < 1e-6);
    QVERIFY(std::abs(instance.geometry.columnSpacingMm - 0.8) < 1e-6);
    QCOMPARE(instance.geometry.imageOrientationPatient[0], 1.0);
    QCOMPARE(instance.geometry.imageOrientationPatient[4], 1.0);
    QVERIFY(std::abs(instance.rescale.slope - 2) < 1e-6);
    QVERIFY(std::abs(instance.rescale.intercept + 1024) < 1e-6);
}

void DicomReadTest::readsImplicitLittleEndianCt() {
    std::vector<std::uint8_t> meta;
    appendText(meta, 0x0002, 0x0010, "UI", "1.2.840.10008.1.2");

    std::vector<std::uint8_t> file(128, 0);
    file.insert(file.end(), {'D', 'I', 'C', 'M'});
    appendTag(file, 0x0002, 0x0000);
    file.push_back('U');
    file.push_back('L');
    appendU16(file, 4);
    appendU32(file, static_cast<std::uint32_t>(meta.size()));
    file.insert(file.end(), meta.begin(), meta.end());
    appendText(file, 0x0002, 0x0012, "UI", "1.2.826.0.1.3680043.10.543");

    appendImplicitText(file, 0x0020, 0x000E, "1.2.826.0.1.3680043.10.543.2");
    appendImplicitText(file, 0x0020, 0x0032, "-10\\20\\-30");
    appendImplicitText(file, 0x0020, 0x0037, "1\\0\\0\\0\\1\\0");
    appendImplicitU16(file, 0x0028, 0x0010, 2);
    appendImplicitU16(file, 0x0028, 0x0011, 2);
    appendImplicitText(file, 0x0028, 0x0030, "0.5\\0.8");
    appendImplicitU16(file, 0x0028, 0x0100, 16);
    appendImplicitU16(file, 0x0028, 0x0103, 1);
    const std::vector<std::int16_t> pixels{0, 24, -12, 100};
    appendTag(file, 0x7FE0, 0x0010);
    appendU32(file, static_cast<std::uint32_t>(pixels.size() * sizeof(std::int16_t)));
    appendBytes(file, pixels.data(), pixels.size() * sizeof(std::int16_t));

    const auto instance = readUncompressedCt(file);
    QCOMPARE(instance.status, DicomReadStatus::Ok);
    QCOMPARE(instance.storedValues, pixels);
    QCOMPARE(instance.seriesInstanceUid, std::string("1.2.826.0.1.3680043.10.543.2"));
}

void DicomReadTest::readsASequenceAndPaddedDecimalStrings() {
    const std::vector<std::int16_t> pixels{0, 24, -12, 100};
    std::vector<std::uint8_t> meta;
    appendText(meta, 0x0002, 0x0010, "UI", "1.2.840.10008.1.2.1");
    std::vector<std::uint8_t> file(128, 0);
    file.insert(file.end(), {'D', 'I', 'C', 'M'});
    appendTag(file, 0x0002, 0x0000);
    file.push_back('U');
    file.push_back('L');
    appendU16(file, 4);
    appendU32(file, static_cast<std::uint32_t>(meta.size()));
    file.insert(file.end(), meta.begin(), meta.end());
    appendUndefinedSequence(file);
    appendText(file, 0x0020, 0x000E, "UI", "1.2.826.0.1.3680043.10.543.2");
    appendText(file, 0x0020, 0x0032, "DS", " -10\\20\\-30 ");
    appendText(file, 0x0020, 0x0037, "DS", "1\\0\\0\\0\\1\\0");
    appendU16Element(file, 0x0028, 0x0010, 2);
    appendU16Element(file, 0x0028, 0x0011, 2);
    appendText(file, 0x0028, 0x0030, "DS", " 0.5\\0.8");
    appendU16Element(file, 0x0028, 0x0100, 16);
    appendTag(file, 0x7FE0, 0x0010);
    file.push_back('O');
    file.push_back('W');
    appendU16(file, 0);
    appendU32(file, static_cast<std::uint32_t>(pixels.size() * sizeof(std::int16_t)));
    appendBytes(file, pixels.data(), pixels.size() * sizeof(std::int16_t));

    const auto instance = readUncompressedCt(file);
    QCOMPARE(instance.status, DicomReadStatus::Ok);
    QCOMPARE(instance.storedValues, pixels);
    QVERIFY(std::abs(instance.geometry.rowSpacingMm - 0.5) < 1e-6);
    QVERIFY(std::abs(instance.geometry.imagePositionPatient.x + 10) < 1e-6);
}

void DicomReadTest::readsAnImplicitSequence() {
    std::vector<std::uint8_t> meta;
    appendText(meta, 0x0002, 0x0010, "UI", "1.2.840.10008.1.2");
    std::vector<std::uint8_t> file(128, 0);
    file.insert(file.end(), {'D', 'I', 'C', 'M'});
    appendTag(file, 0x0002, 0x0000);
    file.push_back('U');
    file.push_back('L');
    appendU16(file, 4);
    appendU32(file, static_cast<std::uint32_t>(meta.size()));
    file.insert(file.end(), meta.begin(), meta.end());

    appendTag(file, 0x0008, 0x1140);
    appendU32(file, 0xFFFFFFFFU);
    appendTag(file, 0xFFFE, 0xE000);
    appendU32(file, 0);
    appendTag(file, 0xFFFE, 0xE0DD);
    appendU32(file, 0);
    appendImplicitText(file, 0x0020, 0x000E, "1.2.826.0.1.3680043.10.543.2");
    appendImplicitText(file, 0x0020, 0x0032, "-10\\20\\-30");
    appendImplicitText(file, 0x0020, 0x0037, "1\\0\\0\\0\\1\\0");
    appendImplicitU16(file, 0x0028, 0x0010, 2);
    appendImplicitU16(file, 0x0028, 0x0011, 2);
    appendImplicitText(file, 0x0028, 0x0030, "0.5\\0.8");
    appendImplicitU16(file, 0x0028, 0x0100, 16);
    const std::vector<std::int16_t> pixels{0, 24, -12, 100};
    appendTag(file, 0x7FE0, 0x0010);
    appendU32(file, static_cast<std::uint32_t>(pixels.size() * sizeof(std::int16_t)));
    appendBytes(file, pixels.data(), pixels.size() * sizeof(std::int16_t));

    const auto instance = readUncompressedCt(file);
    QCOMPARE(instance.status, DicomReadStatus::Ok);
    QCOMPARE(instance.storedValues, pixels);
}

void DicomReadTest::readsNativePixelsAfterAnEmptyOffsetTable() {
    const std::vector<std::int16_t> pixels{0, 24, -12, 100};
    std::vector<std::uint8_t> meta;
    appendText(meta, 0x0002, 0x0010, "UI", "1.2.840.10008.1.2.1");
    std::vector<std::uint8_t> file(128, 0);
    file.insert(file.end(), {'D', 'I', 'C', 'M'});
    appendTag(file, 0x0002, 0x0000);
    file.push_back('U');
    file.push_back('L');
    appendU16(file, 4);
    appendU32(file, static_cast<std::uint32_t>(meta.size()));
    file.insert(file.end(), meta.begin(), meta.end());
    appendText(file, 0x0020, 0x000E, "UI", "1.2.826.0.1.3680043.10.543.2");
    appendText(file, 0x0020, 0x0032, "DS", "0\\0\\0");
    appendText(file, 0x0020, 0x0037, "DS", "1\\0\\0\\0\\1\\0");
    appendU16Element(file, 0x0028, 0x0010, 2);
    appendU16Element(file, 0x0028, 0x0011, 2);
    appendText(file, 0x0028, 0x0030, "DS", "1\\1");
    appendU16Element(file, 0x0028, 0x0100, 16);
    appendTag(file, 0x7FE0, 0x0010);
    file.push_back('O');
    file.push_back('W');
    appendU16(file, 0);
    appendU32(file, 0xFFFFFFFFU);
    appendTag(file, 0xFFFE, 0xE000);
    appendU32(file, 0);
    appendTag(file, 0xFFFE, 0xE000);
    appendU32(file, static_cast<std::uint32_t>(pixels.size() * sizeof(std::int16_t)));
    appendBytes(file, pixels.data(), pixels.size() * sizeof(std::int16_t));
    appendTag(file, 0xFFFE, 0xE0DD);
    appendU32(file, 0);

    const auto instance = readUncompressedCt(file);
    QCOMPARE(instance.status, DicomReadStatus::Ok);
    QCOMPARE(instance.storedValues, pixels);
}

void DicomReadTest::readsADatasetWithoutAFilePreamble() {
    const std::vector<std::int16_t> pixels{1, 2, 3, 4};
    std::vector<std::uint8_t> file;
    appendText(file, 0x0008, 0x0060, "CS", "CT");
    appendText(file, 0x0020, 0x000E, "UI", "1.2.826.0.1.3680043.10.543.2");
    appendText(file, 0x0020, 0x0032, "DS", "0\\0\\0");
    appendText(file, 0x0020, 0x0037, "DS", "1\\0\\0\\0\\1\\0");
    appendU16Element(file, 0x0028, 0x0010, 2);
    appendU16Element(file, 0x0028, 0x0011, 2);
    appendText(file, 0x0028, 0x0030, "DS", "1\\1");
    appendU16Element(file, 0x0028, 0x0100, 16);
    appendTag(file, 0x7FE0, 0x0010);
    file.push_back('O');
    file.push_back('W');
    appendU16(file, 0);
    appendU32(file, static_cast<std::uint32_t>(pixels.size() * sizeof(std::int16_t)));
    appendBytes(file, pixels.data(), pixels.size() * sizeof(std::int16_t));

    const auto instance = readUncompressedCt(file);
    QCOMPARE(instance.status, DicomReadStatus::Ok);
    QCOMPARE(instance.storedValues, pixels);
}

void DicomReadTest::rejectsCompressedTransferSyntax() {
    const auto bytes = syntheticExplicitCt("1.2.840.10008.1.2.4.70", {0, 0, 0, 0});
    const auto instance = readUncompressedCt(bytes);
    QCOMPARE(instance.status, DicomReadStatus::UnsupportedTransferSyntax);
}

void DicomReadTest::rejectsATruncatedFile() {
    auto bytes = syntheticExplicitCt("1.2.840.10008.1.2.1", {0, 0, 0, 0});
    bytes.resize(140);
    const auto instance = readUncompressedCt(bytes);
    QCOMPARE(instance.status, DicomReadStatus::Truncated);
}

void DicomReadTest::rejectsAFileThatIsNotDicom() {
    const std::vector<std::uint8_t> bytes(200, 0);
    QCOMPARE(readUncompressedCt(bytes).status, DicomReadStatus::NotDicom);
}

QTEST_APPLESS_MAIN(DicomReadTest)
#include "DicomReadTest.moc"
