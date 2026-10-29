#include "medforge/core/Project.h"

#include <bit>
#include <charconv>
#include <fstream>
#include <limits>
#include <sstream>
#include <unordered_map>
#include <utility>

namespace medforge::core {
namespace {

constexpr std::uint64_t kFnvOffset = 14695981039346656037ull;
constexpr std::uint64_t kFnvPrime = 1099511628211ull;

void mixByte(std::uint64_t& hash, unsigned char byte) {
    hash ^= byte;
    hash *= kFnvPrime;
}

void mix32(std::uint64_t& hash, std::uint32_t value) {
    mixByte(hash, static_cast<unsigned char>(value & 0xffu));
    mixByte(hash, static_cast<unsigned char>((value >> 8) & 0xffu));
    mixByte(hash, static_cast<unsigned char>((value >> 16) & 0xffu));
    mixByte(hash, static_cast<unsigned char>((value >> 24) & 0xffu));
}

std::string formatDouble(double value) {
    std::ostringstream out;
    out.precision(std::numeric_limits<double>::max_digits10);
    out << value;
    return out.str();
}

std::string formatHex(std::uint64_t value) {
    char text[17];
    const auto result = std::to_chars(text, text + 16, value, 16);
    const std::size_t written = static_cast<std::size_t>(result.ptr - text);
    return std::string(16 - written, '0') + std::string(text, written);
}

std::string_view trim(std::string_view text) {
    while (!text.empty() && (text.front() == ' ' || text.front() == '\t' || text.front() == '\r')) {
        text.remove_prefix(1);
    }
    while (!text.empty() && (text.back() == ' ' || text.back() == '\t' || text.back() == '\r')) {
        text.remove_suffix(1);
    }
    return text;
}

bool parseInt(std::string_view text, int* value) {
    const auto token = trim(text);
    int parsed = 0;
    const auto result = std::from_chars(token.data(), token.data() + token.size(), parsed);
    if (result.ec != std::errc{} || result.ptr != token.data() + token.size()) {
        return false;
    }
    *value = parsed;
    return true;
}

bool parseDouble(std::string_view text, double* value) {
    const auto token = trim(text);
    double parsed = 0;
    const auto result = std::from_chars(token.data(), token.data() + token.size(), parsed);
    if (result.ec != std::errc{} || result.ptr != token.data() + token.size()) {
        return false;
    }
    *value = parsed;
    return true;
}

bool parseBool(std::string_view text, bool* value) {
    int parsed = 0;
    if (!parseInt(text, &parsed) || (parsed != 0 && parsed != 1)) {
        return false;
    }
    *value = parsed == 1;
    return true;
}

bool required(const std::unordered_map<std::string, std::string>& fields,
              const char* key,
              std::string* value,
              std::string* error) {
    const auto found = fields.find(key);
    if (found == fields.end() || found->second.empty()) {
        if (error != nullptr) {
            *error = std::string("Project is missing ") + key + ".";
        }
        return false;
    }
    *value = found->second;
    return true;
}

ProjectLoad corrupt(std::string message) {
    ProjectLoad load;
    load.status = ProjectStatus::Corrupt;
    load.message = std::move(message);
    return load;
}

} // namespace

std::uint64_t contentFingerprint(std::string_view seriesInstanceUid,
                                 int columns,
                                 int rows,
                                 int slices,
                                 const float* hu,
                                 std::size_t count) {
    if (columns <= 0 || rows <= 0 || slices <= 0 || seriesInstanceUid.empty()) {
        return 0;
    }
    const std::size_t expected = static_cast<std::size_t>(columns) *
                                 static_cast<std::size_t>(rows) * static_cast<std::size_t>(slices);
    if (hu == nullptr || count != expected) {
        return 0;
    }
    std::uint64_t hash = kFnvOffset;
    for (char byte : seriesInstanceUid) {
        mixByte(hash, static_cast<unsigned char>(byte));
    }
    mix32(hash, static_cast<std::uint32_t>(columns));
    mix32(hash, static_cast<std::uint32_t>(rows));
    mix32(hash, static_cast<std::uint32_t>(slices));
    for (std::size_t index = 0; index < count; ++index) {
        mix32(hash, std::bit_cast<std::uint32_t>(hu[index]));
    }
    return hash == 0 ? 1 : hash;
}

bool fingerprintMatches(const ProjectDocument& document,
                        std::string_view seriesInstanceUid,
                        int columns,
                        int rows,
                        int slices,
                        const float* hu,
                        std::size_t count) {
    const std::uint64_t actual =
        contentFingerprint(seriesInstanceUid, columns, rows, slices, hu, count);
    return actual != 0 && actual == document.checksum &&
           seriesInstanceUid == document.seriesInstanceUid && columns == document.columns &&
           rows == document.rows && slices == document.slices;
}

bool saveProject(const std::filesystem::path& path,
                 const ProjectDocument& document,
                 std::string* error) {
    if (document.version != kProjectVersion) {
        if (error != nullptr) {
            *error = "Only project version 1 can be written.";
        }
        return false;
    }
    if (document.seriesInstanceUid.empty() || document.seriesFolder.empty() ||
        document.checksum == 0 || document.researchNotice.empty()) {
        if (error != nullptr) {
            *error = "Project is missing the series, checksum, or research notice.";
        }
        return false;
    }
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    const std::filesystem::path temp(path.string() + ".tmp");
    const std::filesystem::path backup(path.string() + ".bak");
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!out) {
            if (error != nullptr) {
                *error = "Could not write the project.";
            }
            return false;
        }
        out << "MFPROJECT " << kProjectVersion << "\n";
        out << "seriesUid " << document.seriesInstanceUid << "\n";
        out << "seriesFolder " << document.seriesFolder << "\n";
        out << "columns " << document.columns << "\n";
        out << "rows " << document.rows << "\n";
        out << "slices " << document.slices << "\n";
        out << "checksum " << formatHex(document.checksum) << "\n";
        out << "windowWidth " << formatDouble(document.windowWidth) << "\n";
        out << "windowCenter " << formatDouble(document.windowCenter) << "\n";
        out << "crosshairI " << formatDouble(document.crosshairI) << "\n";
        out << "crosshairJ " << formatDouble(document.crosshairJ) << "\n";
        out << "crosshairK " << formatDouble(document.crosshairK) << "\n";
        out << "isoThreshold " << formatDouble(document.isoThreshold) << "\n";
        out << "hasImplant " << (document.hasImplant ? 1 : 0) << "\n";
        out << "implantL " << formatDouble(document.implantL) << "\n";
        out << "implantP " << formatDouble(document.implantP) << "\n";
        out << "implantS " << formatDouble(document.implantS) << "\n";
        out << "implantRx " << formatDouble(document.implantRx) << "\n";
        out << "implantRy " << formatDouble(document.implantRy) << "\n";
        out << "implantRz " << formatDouble(document.implantRz) << "\n";
        out << "clearanceTolerance " << formatDouble(document.clearanceTolerance) << "\n";
        out << "guideOffset " << formatDouble(document.guideOffset) << "\n";
        out << "guideHole " << formatDouble(document.guideHole) << "\n";
        out << "guideApplied " << (document.guideApplied ? 1 : 0) << "\n";
        out << "notice " << document.researchNotice << "\n";
        out.flush();
        if (!out) {
            std::filesystem::remove(temp);
            if (error != nullptr) {
                *error = "Could not write the project.";
            }
            return false;
        }
    }
    std::filesystem::remove(backup, ec);
    if (std::filesystem::exists(path)) {
        std::filesystem::rename(path, backup, ec);
        if (ec) {
            std::filesystem::remove(temp);
            if (error != nullptr) {
                *error = "Could not replace the project.";
            }
            return false;
        }
    }
    std::filesystem::rename(temp, path, ec);
    if (ec) {
        std::error_code restore;
        std::filesystem::rename(backup, path, restore);
        std::filesystem::remove(temp);
        if (error != nullptr) {
            *error = "Could not replace the project.";
        }
        return false;
    }
    std::filesystem::remove(backup, ec);
    return true;
}

ProjectLoad loadProject(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        ProjectLoad load;
        load.status = ProjectStatus::IoError;
        load.message = "Could not read the project.";
        return load;
    }
    std::string line;
    if (!std::getline(in, line)) {
        return corrupt("Project file is empty.");
    }
    const auto header = trim(line);
    constexpr std::string_view prefix = "MFPROJECT ";
    if (!header.starts_with(prefix)) {
        return corrupt("Project header is missing.");
    }
    int version = 0;
    if (!parseInt(header.substr(prefix.size()), &version)) {
        return corrupt("Project version is not a number.");
    }
    if (version > kProjectVersion) {
        ProjectLoad load;
        load.status = ProjectStatus::UnsupportedVersion;
        load.message =
            "This project is version " + std::to_string(version) + ". MedForge reads version 1.";
        return load;
    }
    if (version != kProjectVersion) {
        return corrupt("Project version is not supported.");
    }

    std::unordered_map<std::string, std::string> fields;
    while (std::getline(in, line)) {
        const auto text = trim(line);
        if (text.empty()) {
            continue;
        }
        const auto gap = text.find(' ');
        if (gap == std::string_view::npos) {
            return corrupt("Project line has no value.");
        }
        fields.emplace(std::string(text.substr(0, gap)), std::string(trim(text.substr(gap + 1))));
    }

    ProjectDocument document;
    document.version = version;
    std::string value;
    std::string error;
    if (!required(fields, "seriesUid", &document.seriesInstanceUid, &error) ||
        !required(fields, "seriesFolder", &document.seriesFolder, &error) ||
        !required(fields, "checksum", &value, &error) ||
        !required(fields, "notice", &document.researchNotice, &error)) {
        return corrupt(error);
    }
    const auto checksumText = trim(value);
    std::uint64_t checksum = 0;
    const auto parsed = std::from_chars(
        checksumText.data(), checksumText.data() + checksumText.size(), checksum, 16);
    if (parsed.ec != std::errc{} || parsed.ptr != checksumText.data() + checksumText.size() ||
        checksum == 0) {
        return corrupt("Project checksum is invalid.");
    }
    document.checksum = checksum;
    if (!required(fields, "columns", &value, &error) || !parseInt(value, &document.columns) ||
        !required(fields, "rows", &value, &error) || !parseInt(value, &document.rows) ||
        !required(fields, "slices", &value, &error) || !parseInt(value, &document.slices) ||
        !required(fields, "windowWidth", &value, &error) ||
        !parseDouble(value, &document.windowWidth) ||
        !required(fields, "windowCenter", &value, &error) ||
        !parseDouble(value, &document.windowCenter) ||
        !required(fields, "crosshairI", &value, &error) ||
        !parseDouble(value, &document.crosshairI) ||
        !required(fields, "crosshairJ", &value, &error) ||
        !parseDouble(value, &document.crosshairJ) ||
        !required(fields, "crosshairK", &value, &error) ||
        !parseDouble(value, &document.crosshairK) ||
        !required(fields, "isoThreshold", &value, &error) ||
        !parseDouble(value, &document.isoThreshold)) {
        return corrupt(error.empty() ? "Project field is invalid." : error);
    }
    if (auto found = fields.find("hasImplant"); found != fields.end()) {
        if (!parseBool(found->second, &document.hasImplant)) {
            return corrupt("Project implant flag is invalid.");
        }
    }
    const auto readDouble = [&](const char* key, double* target) {
        const auto found = fields.find(key);
        if (found == fields.end()) {
            return true;
        }
        return parseDouble(found->second, target);
    };
    if (!readDouble("implantL", &document.implantL) ||
        !readDouble("implantP", &document.implantP) ||
        !readDouble("implantS", &document.implantS) ||
        !readDouble("implantRx", &document.implantRx) ||
        !readDouble("implantRy", &document.implantRy) ||
        !readDouble("implantRz", &document.implantRz) ||
        !readDouble("clearanceTolerance", &document.clearanceTolerance) ||
        !readDouble("guideOffset", &document.guideOffset) ||
        !readDouble("guideHole", &document.guideHole)) {
        return corrupt("Project number is invalid.");
    }
    if (auto found = fields.find("guideApplied"); found != fields.end()) {
        if (!parseBool(found->second, &document.guideApplied)) {
            return corrupt("Project guide flag is invalid.");
        }
    }
    if (document.researchNotice.find("not for clinical use") == std::string::npos) {
        return corrupt("Project is missing the research notice.");
    }
    ProjectLoad load;
    load.status = ProjectStatus::Ok;
    load.document = std::move(document);
    return load;
}

std::string portableSeriesFolder(const std::filesystem::path& projectFile,
                                 const std::filesystem::path& seriesFolder) {
    std::error_code ec;
    const std::filesystem::path relative =
        std::filesystem::relative(seriesFolder, projectFile.parent_path(), ec);
    const std::filesystem::path chosen =
        (!ec && !relative.empty() && relative.is_relative()) ? relative : seriesFolder;
    return chosen.generic_string();
}

std::filesystem::path resolveSeriesFolder(const std::filesystem::path& projectFile,
                                          const std::string& stored) {
    const std::filesystem::path path(stored);
    if (path.is_absolute()) {
        return path.lexically_normal();
    }
    return (projectFile.parent_path() / path).lexically_normal();
}

std::filesystem::path projectSidecar(const std::filesystem::path& projectFile,
                                     std::string_view extension) {
    std::filesystem::path path = projectFile;
    path.replace_extension(std::string(extension));
    return path;
}

} // namespace medforge::core
