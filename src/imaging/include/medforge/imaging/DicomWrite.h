#pragma once

#include "medforge/imaging/DicomRead.h"

#include <filesystem>
#include <string>

namespace medforge::imaging {

// Writes one explicit-VR little-endian part-10 CT file. The transfer syntax UID
// stored in the file meta header is instance.transferSyntaxUid when that is not empty.
bool writePart10Ct(const std::filesystem::path& path,
                   const DicomCtInstance& instance,
                   int instanceNumber,
                   std::string* error);

} // namespace medforge::imaging
