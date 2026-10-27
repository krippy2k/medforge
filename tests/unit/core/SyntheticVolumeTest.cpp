#include "medforge/core/SyntheticVolume.h"

#include <QDir>
#include <QFile>
#include <QtTest>
#include <filesystem>

using medforge::core::FixtureStatus;
using medforge::core::generateSyntheticVolume;
using medforge::core::SyntheticVolumeSpec;
using medforge::core::writeSyntheticVolume;

class SyntheticVolumeTest : public QObject {
    Q_OBJECT

private slots:
    void rejectsInvalidShape();
    void isDeterministicAndSeeded();
    void writesLittleEndianFixtureWithoutPatientFields();
    void matchesCommittedSphereFixture();
};

void SyntheticVolumeTest::rejectsInvalidShape() {
    SyntheticVolumeSpec spec;
    spec.sizeI = 0;
    QVERIFY(!generateSyntheticVolume(spec).has_value());
    spec.sizeI = 8;
    spec.spacingMm = {0, 1, 1};
    QVERIFY(!generateSyntheticVolume(spec).has_value());
}

void SyntheticVolumeTest::isDeterministicAndSeeded() {
    SyntheticVolumeSpec spec;
    const auto first = generateSyntheticVolume(spec);
    const auto second = generateSyntheticVolume(spec);
    QVERIFY(first.has_value());
    QCOMPARE(first->samples, second->samples);
    QCOMPARE(first->samples.size(), std::size_t{512});
    spec.seed = 2;
    const auto other = generateSyntheticVolume(spec);
    QVERIFY(other.has_value());
    QVERIFY(first->samples != other->samples);
}

void SyntheticVolumeTest::writesLittleEndianFixtureWithoutPatientFields() {
    const auto volume = generateSyntheticVolume(SyntheticVolumeSpec{});
    QVERIFY(volume.has_value());
    const auto directory = std::filesystem::temp_directory_path() / "medforge-m0-synthetic";
    std::filesystem::remove_all(directory);
    const auto written = writeSyntheticVolume(*volume, directory);
    QCOMPARE(written.status, FixtureStatus::Ok);

    QFile raw(QString::fromStdString((directory / "voxels.raw").string()));
    QVERIFY(raw.open(QIODevice::ReadOnly));
    QCOMPARE(raw.size(), static_cast<qint64>(volume->samples.size() * 2));
    const QByteArray bytes = raw.readAll();
    QCOMPARE(
        static_cast<unsigned char>(bytes.at(0)),
        static_cast<unsigned char>(static_cast<std::uint16_t>(volume->samples.front()) & 0xffU));

    QFile json(QString::fromStdString((directory / "volume.json").string()));
    QVERIFY(json.open(QIODevice::ReadOnly));
    const QString text = QString::fromUtf8(json.readAll());
    QVERIFY(text.contains(QStringLiteral("\"researchOnly\": true")));
    QVERIFY(text.contains(QStringLiteral("Not patient data")));
    QVERIFY(!text.contains(QStringLiteral("PatientName")));
    QVERIFY(!text.contains(QStringLiteral("PatientID")));
}

void SyntheticVolumeTest::matchesCommittedSphereFixture() {
    const auto volume = generateSyntheticVolume(SyntheticVolumeSpec{});
    QVERIFY(volume.has_value());
    const QString path = QStringLiteral(MEDFORGE_SOURCE_DIR) +
                         QStringLiteral("/tests/fixtures/synthetic/sphere8/voxels.raw");
    QFile raw(path);
    QVERIFY2(raw.open(QIODevice::ReadOnly), qPrintable(path));
    QByteArray expected;
    expected.resize(static_cast<qsizetype>(volume->samples.size() * 2));
    for (int index = 0; index < static_cast<int>(volume->samples.size()); ++index) {
        const auto bits =
            static_cast<std::uint16_t>(volume->samples[static_cast<std::size_t>(index)]);
        expected[index * 2] = static_cast<char>(bits & 0xffU);
        expected[(index * 2) + 1] = static_cast<char>((bits >> 8U) & 0xffU);
    }
    QCOMPARE(raw.readAll(), expected);
}

QTEST_APPLESS_MAIN(SyntheticVolumeTest)
#include "SyntheticVolumeTest.moc"
