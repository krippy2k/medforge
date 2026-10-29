#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace medforge::core {

inline constexpr int kProjectVersion = 1;

// Native-endian HU values are hashed as little-endian bits, with the series UID
// and dimensions. The CT folder remains the image data. A project file stores
// the fingerprint so a changed series is refused.
std::uint64_t contentFingerprint(std::string_view seriesInstanceUid,
                                 int columns,
                                 int rows,
                                 int slices,
                                 const float* hu,
                                 std::size_t count);

struct ProjectDocument {
    int version = kProjectVersion;
    std::string seriesInstanceUid;
    std::string seriesFolder;
    int columns = 0;
    int rows = 0;
    int slices = 0;
    std::uint64_t checksum = 0;
    double windowWidth = 0;
    double windowCenter = 0;
    double crosshairI = 0;
    double crosshairJ = 0;
    double crosshairK = 0;
    double isoThreshold = 200;
    bool hasImplant = false;
    double implantL = 0;
    double implantP = 0;
    double implantS = 0;
    double implantRx = 0;
    double implantRy = 0;
    double implantRz = 0;
    double clearanceTolerance = 0.1;
    double guideOffset = 4;
    double guideHole = 2;
    bool guideApplied = false;
    std::string researchNotice;
};

enum class ProjectStatus { Ok, UnsupportedVersion, Corrupt, IoError };

struct ProjectLoad {
    ProjectStatus status = ProjectStatus::IoError;
    std::string message;
    ProjectDocument document;
};

bool fingerprintMatches(const ProjectDocument& document,
                        std::string_view seriesInstanceUid,
                        int columns,
                        int rows,
                        int slices,
                        const float* hu,
                        std::size_t count);

// Writes a temporary file, then replaces any existing project. A failed replace
// restores the previous file.
bool saveProject(const std::filesystem::path& path,
                 const ProjectDocument& document,
                 std::string* error);

ProjectLoad loadProject(const std::filesystem::path& path);

std::string portableSeriesFolder(const std::filesystem::path& projectFile,
                                 const std::filesystem::path& seriesFolder);
std::filesystem::path resolveSeriesFolder(const std::filesystem::path& projectFile,
                                          const std::string& stored);
std::filesystem::path projectSidecar(const std::filesystem::path& projectFile,
                                     std::string_view extension);

} // namespace medforge::core
