#include "medforge/imaging/SyntheticPhantom.h"

#include "medforge/imaging/DicomWrite.h"

#include <cmath>

namespace medforge::imaging::phantom {
namespace {

std::int16_t storedForHu(double hu) {
    return static_cast<std::int16_t>(std::llround((hu - kIntercept) / kSlope));
}

} // namespace

bool writeSyntheticAxialSeries(const std::filesystem::path& directory, std::string* error) {
    std::error_code failure;
    std::filesystem::create_directories(directory, failure);
    if (failure) {
        if (error != nullptr) {
            *error = "Could not create the phantom folder";
        }
        return false;
    }

    for (int slice = 0; slice < kSlices; ++slice) {
        DicomCtInstance instance;
        instance.status = DicomReadStatus::Ok;
        instance.transferSyntaxUid = "1.2.840.10008.1.2.1";
        instance.seriesInstanceUid = kSeriesUid;
        instance.geometry.imageOrientationPatient = {1, 0, 0, 0, 1, 0};
        instance.geometry.imagePositionPatient =
            core::Vec3{kOriginX, kOriginY, kOriginZ + slice * kSliceSpacingMm};
        instance.geometry.rowSpacingMm = kRowSpacingMm;
        instance.geometry.columnSpacingMm = kColumnSpacingMm;
        instance.rescale = Rescale{kSlope, kIntercept};
        instance.rows = kRows;
        instance.columns = kColumns;
        instance.storedValues.assign(static_cast<std::size_t>(kRows * kColumns),
                                     storedForHu(kAirHu));
        for (int row = 0; row < kRows; ++row) {
            instance.storedValues[static_cast<std::size_t>(row * kColumns + slice)] =
                storedForHu(80);
        }
        if (slice == kMarkerK) {
            instance.storedValues[static_cast<std::size_t>(kMarkerJ * kColumns + kMarkerI)] =
                storedForHu(kMarkerHu);
        }
        const std::filesystem::path file =
            directory / ("slice-0" + std::to_string(slice) + ".dcm");
        if (!writePart10Ct(file, instance, slice + 1, error)) {
            return false;
        }
    }
    return true;
}

} // namespace medforge::imaging::phantom
