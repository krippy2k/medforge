#pragma once

#include "medforge/imaging/CtSeries.h"

#include <cstdint>
#include <string>
#include <vector>

namespace medforge::imaging {

enum class DicomReadStatus {
    Ok,
    NotDicom,
    Truncated,
    UnsupportedTransferSyntax,
    MissingAttribute,
    UnsupportedPixelData
};

struct DicomCtInstance {
    DicomReadStatus status = DicomReadStatus::NotDicom;
    std::string transferSyntaxUid;
    std::string seriesInstanceUid;
    SliceGeometry geometry{};
    Rescale rescale{};
    int rows = 0;
    int columns = 0;
    std::vector<std::int16_t> storedValues;
};

// Reads one uncompressed CT instance. Part-10 explicit and implicit VR little endian
// are accepted, including undefined-length sequences and a dataset with no file
// preamble. Big-endian and compressed syntaxes are recognized and rejected until a
// decoder for them exists.
DicomCtInstance readUncompressedCt(const std::vector<std::uint8_t>& bytes);

} // namespace medforge::imaging
