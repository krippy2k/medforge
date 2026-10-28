#pragma once

#include "medforge/core/Cancellation.h"
#include "medforge/imaging/CtVolume.h"

#include <filesystem>
#include <functional>
#include <string>

namespace medforge::imaging {

enum class ImportStatus { Ok, EmptyFolder, Cancelled, Rejected, Unreadable };

struct ImportRequest {
    std::filesystem::path folder;
    int delayPerFileMs = 0;
};

struct ImportResult {
    ImportStatus status = ImportStatus::EmptyFolder;
    VolumeBuild build;
    int filesSeen = 0;
    std::string message;
};

// Reads uncompressed CT files under folder, including subfolders.
// The largest series is built. Compressed syntaxes are reported and not decoded.
// progress may be empty. delayPerFileMs is for tests that need a cancellable import.
ImportResult importCtFolder(const ImportRequest& request,
                            const core::CancellationToken& token,
                            const std::function<void(int done, int total)>& progress);

} // namespace medforge::imaging
