#include "medforge/imaging/CtImport.h"

#include "medforge/imaging/DicomRead.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <map>
#include <thread>

namespace medforge::imaging {
namespace {

ImportResult makeResult(ImportStatus status, std::string message, int filesSeen = 0) {
    ImportResult result;
    result.status = status;
    result.message = std::move(message);
    result.filesSeen = filesSeen;
    return result;
}

std::vector<std::uint8_t> readFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return {};
    }
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(input),
                                     std::istreambuf_iterator<char>());
}

} // namespace

ImportResult importCtFolder(const ImportRequest& request,
                            const core::CancellationToken& token,
                            const std::function<void(int, int)>& progress) {
    if (token.isCancelled()) {
        return makeResult(ImportStatus::Cancelled, "Import cancelled");
    }

    std::vector<std::filesystem::path> files;
    try {
        if (!std::filesystem::exists(request.folder) ||
            !std::filesystem::is_directory(request.folder)) {
            return makeResult(ImportStatus::Unreadable, "Folder not found");
        }
        const auto options = std::filesystem::directory_options::skip_permission_denied;
        for (const std::filesystem::directory_entry& entry :
             std::filesystem::recursive_directory_iterator(request.folder, options)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path());
            }
        }
    } catch (const std::filesystem::filesystem_error&) {
        return makeResult(ImportStatus::Unreadable, "Folder not found");
    }
    std::sort(files.begin(), files.end());

    std::map<std::string, std::vector<DicomCtInstance>> series;
    std::string compressedSyntax;
    DicomReadStatus dicomFailure = DicomReadStatus::NotDicom;
    int filesSeen = 0;
    for (const std::filesystem::path& path : files) {
        if (token.isCancelled()) {
            return makeResult(ImportStatus::Cancelled, "Import cancelled", filesSeen);
        }
        if (request.delayPerFileMs > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(request.delayPerFileMs));
        }
        if (token.isCancelled()) {
            return makeResult(ImportStatus::Cancelled, "Import cancelled", filesSeen);
        }
        ++filesSeen;
        if (progress) {
            progress(filesSeen, static_cast<int>(files.size()));
        }
        const std::vector<std::uint8_t> bytes = readFile(path);
        if (bytes.empty()) {
            continue;
        }
        const DicomCtInstance instance = readUncompressedCt(bytes);
        if (instance.status == DicomReadStatus::Ok) {
            series[instance.seriesInstanceUid].push_back(instance);
        } else if (instance.status == DicomReadStatus::UnsupportedTransferSyntax) {
            compressedSyntax = instance.transferSyntaxUid;
        } else if (instance.status != DicomReadStatus::NotDicom) {
            dicomFailure = instance.status;
        }
    }

    if (series.empty()) {
        if (!compressedSyntax.empty()) {
            return makeResult(ImportStatus::Rejected,
                              "Compressed transfer syntax is recognized but not decoded",
                              filesSeen);
        }
        if (dicomFailure == DicomReadStatus::UnsupportedPixelData) {
            return makeResult(
                ImportStatus::Rejected,
                "A DICOM file was found, but its pixel data is not uncompressed 16-bit",
                filesSeen);
        }
        if (dicomFailure == DicomReadStatus::MissingAttribute) {
            return makeResult(
                ImportStatus::Rejected,
                "A DICOM file was found, but it is missing image geometry or pixel data",
                filesSeen);
        }
        if (dicomFailure != DicomReadStatus::NotDicom) {
            return makeResult(ImportStatus::Rejected,
                              "A DICOM file was found, but it could not be parsed", filesSeen);
        }
        return makeResult(ImportStatus::EmptyFolder, "No DICOM CT instances found", filesSeen);
    }

    const auto best = std::max_element(
        series.begin(), series.end(), [](const auto& left, const auto& right) {
            if (left.second.size() != right.second.size()) {
                return left.second.size() < right.second.size();
            }
            return left.first > right.first;
        });

    VolumeBuild build = buildCtVolume(best->second);
    ImportResult result;
    result.filesSeen = filesSeen;
    result.build = std::move(build);
    if (result.build.status != VolumeStatus::Ok) {
        result.status = ImportStatus::Rejected;
        result.message = result.build.message;
        return result;
    }
    result.status = ImportStatus::Ok;
    result.message = result.build.message;
    return result;
}

} // namespace medforge::imaging
