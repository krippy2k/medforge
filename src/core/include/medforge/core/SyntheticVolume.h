#pragma once

#include "medforge/core/Coordinates.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace medforge::core {

struct SyntheticVolumeSpec {
    int sizeI = 8;
    int sizeJ = 8;
    int sizeK = 8;
    Vec3 spacingMm{1, 1, 1};
    Vec3 originMm{0, 0, 0};
    Mat3 direction = Mat3::identity();
    std::uint32_t seed = 1;
};

struct SyntheticVolume {
    SyntheticVolumeSpec spec;
    std::vector<std::int16_t> samples;
};

enum class FixtureStatus { Ok, InvalidSpec, IoError };

struct FixtureWriteResult {
    FixtureStatus status = FixtureStatus::Ok;
    std::string message;
};

std::optional<SyntheticVolume> generateSyntheticVolume(const SyntheticVolumeSpec& spec);
FixtureWriteResult writeSyntheticVolume(const SyntheticVolume& volume,
                                        const std::filesystem::path& directory);

} // namespace medforge::core
