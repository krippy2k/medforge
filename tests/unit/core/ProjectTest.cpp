#include "medforge/core/Project.h"

#include "medforge/core/Version.h"

#include <QtTest>
#include <filesystem>
#include <fstream>

namespace {

medforge::core::ProjectDocument sampleDocument() {
    medforge::core::ProjectDocument document;
    document.seriesInstanceUid = "1.2.826.0.1.3680043.10.543.100";
    document.seriesFolder = "series";
    document.columns = 2;
    document.rows = 1;
    document.slices = 1;
    const float hu[] = {-1000.f, 400.f};
    document.checksum = medforge::core::contentFingerprint(
        document.seriesInstanceUid, document.columns, document.rows, document.slices, hu, 2);
    document.windowWidth = 1500;
    document.windowCenter = 300;
    document.crosshairI = 3;
    document.crosshairJ = 2;
    document.crosshairK = 1;
    document.isoThreshold = 180;
    document.hasImplant = true;
    document.implantL = 19.6;
    document.implantP = -20;
    document.implantS = 0;
    document.implantRx = 0;
    document.implantRy = 10;
    document.implantRz = 0;
    document.clearanceTolerance = 0.1;
    document.guideOffset = 4;
    document.guideHole = 2;
    document.guideApplied = false;
    document.researchNotice = medforge::core::kResearchNotice;
    return document;
}

} // namespace

class ProjectTest : public QObject {
    Q_OBJECT

private slots:
    void roundTripKeepsStudyPoseAndNotice();
    void replaceLeavesNoTemporaryFile();
    void newerVersionIsRejected();
    void corruptFileIsRejected();
    void fingerprintDetectsAChangedVoxel();
    void seriesPathIsRelativeToTheProject();
};

void ProjectTest::roundTripKeepsStudyPoseAndNotice() {
    const auto folder = std::filesystem::temp_directory_path() / "medforge-m8-project";
    std::filesystem::remove_all(folder);
    std::filesystem::create_directories(folder);
    const auto path = folder / "case.mfproject";
    const medforge::core::ProjectDocument document = sampleDocument();
    std::string error;
    QVERIFY2(medforge::core::saveProject(path, document, &error), error.c_str());

    const medforge::core::ProjectLoad loaded = medforge::core::loadProject(path);
    QCOMPARE(loaded.status, medforge::core::ProjectStatus::Ok);
    QCOMPARE(loaded.document.seriesInstanceUid, document.seriesInstanceUid);
    QCOMPARE(loaded.document.columns, 2);
    QCOMPARE(loaded.document.windowWidth, 1500.0);
    QCOMPARE(loaded.document.crosshairK, 1.0);
    QCOMPARE(loaded.document.implantL, 19.6);
    QCOMPARE(loaded.document.implantRy, 10.0);
    QVERIFY(loaded.document.researchNotice.find("not for clinical use") != std::string::npos);

    std::ifstream in(path);
    std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    QVERIFY(text.find("not for clinical use") != std::string::npos);
    QVERIFY(text.find("MFPROJECT 1") != std::string::npos);
}

void ProjectTest::replaceLeavesNoTemporaryFile() {
    const auto folder = std::filesystem::temp_directory_path() / "medforge-m8-replace";
    std::filesystem::remove_all(folder);
    std::filesystem::create_directories(folder);
    const auto path = folder / "case.mfproject";
    medforge::core::ProjectDocument document = sampleDocument();
    std::string error;
    QVERIFY(medforge::core::saveProject(path, document, &error));
    document.windowWidth = 800;
    QVERIFY2(medforge::core::saveProject(path, document, &error), error.c_str());
    const medforge::core::ProjectLoad loaded = medforge::core::loadProject(path);
    QCOMPARE(loaded.status, medforge::core::ProjectStatus::Ok);
    QCOMPARE(loaded.document.windowWidth, 800.0);
    QVERIFY(!std::filesystem::exists(std::filesystem::path(path.string() + ".tmp")));
    QVERIFY(!std::filesystem::exists(std::filesystem::path(path.string() + ".bak")));
}

void ProjectTest::newerVersionIsRejected() {
    const auto folder = std::filesystem::temp_directory_path() / "medforge-m8-version";
    std::filesystem::remove_all(folder);
    std::filesystem::create_directories(folder);
    const auto path = folder / "future.mfproject";
    {
        std::ofstream out(path);
        out << "MFPROJECT 99\nseriesUid 1.2.3\n";
    }
    const medforge::core::ProjectLoad loaded = medforge::core::loadProject(path);
    QCOMPARE(loaded.status, medforge::core::ProjectStatus::UnsupportedVersion);
    QVERIFY(loaded.message.find("version") != std::string::npos);
    QVERIFY(loaded.message.find("1") != std::string::npos);
}

void ProjectTest::corruptFileIsRejected() {
    const auto folder = std::filesystem::temp_directory_path() / "medforge-m8-corrupt";
    std::filesystem::remove_all(folder);
    std::filesystem::create_directories(folder);
    const auto path = folder / "broken.mfproject";
    {
        std::ofstream out(path);
        out << "not a project\n";
    }
    const medforge::core::ProjectLoad loaded = medforge::core::loadProject(path);
    QCOMPARE(loaded.status, medforge::core::ProjectStatus::Corrupt);
    const medforge::core::ProjectLoad missing =
        medforge::core::loadProject(folder / "missing.mfproject");
    QCOMPARE(missing.status, medforge::core::ProjectStatus::IoError);
}

void ProjectTest::fingerprintDetectsAChangedVoxel() {
    const float original[] = {-1000.f, 400.f};
    const float changed[] = {-1000.f, 401.f};
    const auto first = medforge::core::contentFingerprint("1.2.3", 2, 1, 1, original, 2);
    const auto second = medforge::core::contentFingerprint("1.2.3", 2, 1, 1, changed, 2);
    QVERIFY(first != 0);
    QVERIFY(first != second);
    medforge::core::ProjectDocument document = sampleDocument();
    document.seriesInstanceUid = "1.2.3";
    document.columns = 2;
    document.rows = 1;
    document.slices = 1;
    document.checksum = first;
    QVERIFY(medforge::core::fingerprintMatches(document, "1.2.3", 2, 1, 1, original, 2));
    QVERIFY(!medforge::core::fingerprintMatches(document, "1.2.3", 2, 1, 1, changed, 2));
}

void ProjectTest::seriesPathIsRelativeToTheProject() {
    const auto root = std::filesystem::temp_directory_path() / "medforge-m8-paths";
    std::filesystem::remove_all(root);
    const auto project = root / "plans" / "case.mfproject";
    const auto series = root / "ct";
    std::filesystem::create_directories(project.parent_path());
    std::filesystem::create_directories(series);
    const std::string stored = medforge::core::portableSeriesFolder(project, series);
    QVERIFY(stored.find("..") != std::string::npos);
    const std::filesystem::path resolved = medforge::core::resolveSeriesFolder(project, stored);
    QCOMPARE(std::filesystem::weakly_canonical(resolved),
             std::filesystem::weakly_canonical(series));
    QCOMPARE(std::filesystem::path(medforge::core::projectSidecar(project, ".mfmask").filename()),
             std::filesystem::path("case.mfmask"));
}

QTEST_APPLESS_MAIN(ProjectTest)
#include "ProjectTest.moc"
