#include "medforge/core/SyntheticVolume.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void printUsage() {
    std::cerr << "Usage: medforge-fixtures --output <directory> [--size N] [--seed N]\n"
              << "Writes a deterministic synthetic volume. Not patient data.\n";
}

} // namespace

int main(int argc, char** argv) {
    std::string output;
    int size = 8;
    unsigned long seed = 1;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--output" && index + 1 < argc) {
            output = argv[++index];
        } else if (argument == "--size" && index + 1 < argc) {
            size = std::atoi(argv[++index]);
        } else if (argument == "--seed" && index + 1 < argc) {
            seed = std::strtoul(argv[++index], nullptr, 10);
        } else {
            printUsage();
            return 2;
        }
    }
    if (output.empty()) {
        printUsage();
        return 2;
    }

    medforge::core::SyntheticVolumeSpec spec;
    spec.sizeI = size;
    spec.sizeJ = size;
    spec.sizeK = size;
    spec.seed = static_cast<std::uint32_t>(seed);
    const auto volume = medforge::core::generateSyntheticVolume(spec);
    if (!volume.has_value()) {
        std::cerr << "Invalid synthetic volume specification.\n";
        return 1;
    }
    const auto written = medforge::core::writeSyntheticVolume(*volume, output);
    if (written.status != medforge::core::FixtureStatus::Ok) {
        std::cerr << written.message << "\n";
        return 1;
    }
    std::cout << "Wrote " << volume->samples.size() << " samples to " << output << "\n";
    return 0;
}
