#include "medforge/core/SyntheticVolume.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <sstream>

namespace medforge::core {
namespace {

constexpr int kMaxSize = 64;

bool isPositive(const Vec3& spacing) {
    return spacing.x > 0 && spacing.y > 0 && spacing.z > 0;
}

bool isFinite(const Vec3& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool specIsValid(const SyntheticVolumeSpec& spec) {
    if (spec.sizeI < 1 || spec.sizeJ < 1 || spec.sizeK < 1 || spec.sizeI > kMaxSize ||
        spec.sizeJ > kMaxSize || spec.sizeK > kMaxSize) {
        return false;
    }
    return isFinite(spec.originMm) && isFinite(spec.spacingMm) && isPositive(spec.spacingMm);
}

std::string number(double value) {
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream.setf(std::ios::fmtflags(0), std::ios::floatfield);
    stream.precision(17);
    stream << value;
    return stream.str();
}

void writeInt16Le(std::ostream& output, std::int16_t value) {
    const auto bits = static_cast<std::uint16_t>(value);
    const std::array<char, 2> bytes = {static_cast<char>(bits & 0xffU),
                                       static_cast<char>((bits >> 8U) & 0xffU)};
    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

} // namespace

std::optional<SyntheticVolume> generateSyntheticVolume(const SyntheticVolumeSpec& spec) {
    if (!specIsValid(spec)) {
        return std::nullopt;
    }
    SyntheticVolume volume;
    volume.spec = spec;
    const auto count = static_cast<std::size_t>(spec.sizeI) * static_cast<std::size_t>(spec.sizeJ) *
                       static_cast<std::size_t>(spec.sizeK);
    volume.samples.resize(count);

    const double centerI = (static_cast<double>(spec.sizeI) - 1.0) / 2.0;
    const double centerJ = (static_cast<double>(spec.sizeJ) - 1.0) / 2.0;
    const double centerK = (static_cast<double>(spec.sizeK) - 1.0) / 2.0;
    const int minSize = std::min({spec.sizeI, spec.sizeJ, spec.sizeK});
    const double radius = 0.28 * static_cast<double>(minSize);
    const double radiusSquared = radius * radius;
    const auto inside = static_cast<std::int16_t>(400 + static_cast<int>(spec.seed % 200U));
    constexpr std::int16_t outside = -1000;

    for (int k = 0; k < spec.sizeK; ++k) {
        for (int j = 0; j < spec.sizeJ; ++j) {
            for (int i = 0; i < spec.sizeI; ++i) {
                const double di = static_cast<double>(i) - centerI;
                const double dj = static_cast<double>(j) - centerJ;
                const double dk = static_cast<double>(k) - centerK;
                const bool inSphere = (di * di) + (dj * dj) + (dk * dk) <= radiusSquared;
                int sample = inSphere ? static_cast<int>(inside) : static_cast<int>(outside);
                if (((i + j + k) & 1) != 0) {
                    sample += 3;
                }
                const auto index =
                    (static_cast<std::size_t>(k) * static_cast<std::size_t>(spec.sizeJ) +
                     static_cast<std::size_t>(j)) *
                        static_cast<std::size_t>(spec.sizeI) +
                    static_cast<std::size_t>(i);
                volume.samples[index] = static_cast<std::int16_t>(sample);
            }
        }
    }
    return volume;
}

FixtureWriteResult writeSyntheticVolume(const SyntheticVolume& volume,
                                        const std::filesystem::path& directory) {
    if (volume.samples.empty()) {
        return {FixtureStatus::InvalidSpec, "Volume has no samples."};
    }
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error) {
        return {FixtureStatus::IoError, error.message()};
    }

    const auto rawPath = directory / "voxels.raw";
    std::ofstream raw(rawPath, std::ios::binary | std::ios::trunc);
    if (!raw) {
        return {FixtureStatus::IoError, "Could not write voxels.raw."};
    }
    for (const std::int16_t sample : volume.samples) {
        writeInt16Le(raw, sample);
    }
    raw.close();

    const auto& spec = volume.spec;
    std::ostringstream json;
    json.imbue(std::locale::classic());
    json << "{\n"
         << "  \"generator\": \"medforge-synthetic\",\n"
         << "  \"formatVersion\": 1,\n"
         << "  \"scalarType\": \"int16\",\n"
         << "  \"endianness\": \"little\",\n"
         << "  \"indexOrder\": \"i-fastest\",\n"
         << "  \"size\": [" << spec.sizeI << ", " << spec.sizeJ << ", " << spec.sizeK << "],\n"
         << "  \"spacingMm\": [" << number(spec.spacingMm.x) << ", " << number(spec.spacingMm.y)
         << ", " << number(spec.spacingMm.z) << "],\n"
         << "  \"originMm\": [" << number(spec.originMm.x) << ", " << number(spec.originMm.y)
         << ", " << number(spec.originMm.z) << "],\n"
         << "  \"directionRowMajor\": [";
    for (std::size_t index = 0; index < spec.direction.m.size(); ++index) {
        if (index != 0) {
            json << ", ";
        }
        json << number(spec.direction.m[index]);
    }
    json << "],\n"
         << "  \"seed\": " << spec.seed << ",\n"
         << "  \"researchOnly\": true,\n"
         << "  \"note\": \"Synthetic fixture. Not patient data. Not for clinical use.\"\n"
         << "}\n";

    std::ofstream sidecar(directory / "volume.json", std::ios::binary | std::ios::trunc);
    if (!sidecar) {
        return {FixtureStatus::IoError, "Could not write volume.json."};
    }
    sidecar << json.str();
    return {FixtureStatus::Ok, {}};
}

} // namespace medforge::core
