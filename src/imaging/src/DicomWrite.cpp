#include "medforge/imaging/DicomWrite.h"

#include <charconv>
#include <fstream>

namespace medforge::imaging {
namespace {

void appendU16(std::vector<std::uint8_t>& out, std::uint16_t value) {
    out.push_back(static_cast<std::uint8_t>(value & 0xFFU));
    out.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
}

void appendU32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    out.push_back(static_cast<std::uint8_t>(value & 0xFFU));
    out.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
    out.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xFFU));
    out.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xFFU));
}

void appendTag(std::vector<std::uint8_t>& out, std::uint16_t group, std::uint16_t element) {
    appendU16(out, group);
    appendU16(out, element);
}

void appendText(std::vector<std::uint8_t>& out,
                std::uint16_t group,
                std::uint16_t element,
                const char* vr,
                std::string value) {
    if (value.size() % 2U != 0U) {
        value.push_back(vr[0] == 'U' ? '\0' : ' ');
    }
    appendTag(out, group, element);
    out.push_back(static_cast<std::uint8_t>(vr[0]));
    out.push_back(static_cast<std::uint8_t>(vr[1]));
    appendU16(out, static_cast<std::uint16_t>(value.size()));
    out.insert(out.end(), value.begin(), value.end());
}

void appendUs(std::vector<std::uint8_t>& out,
              std::uint16_t group,
              std::uint16_t element,
              std::uint16_t value) {
    appendTag(out, group, element);
    out.push_back('U');
    out.push_back('S');
    appendU16(out, 2);
    appendU16(out, value);
}

std::string formatDecimal(double value) {
    char buffer[64];
    const auto result =
        std::to_chars(buffer, buffer + sizeof(buffer), value, std::chars_format::general, 8);
    if (result.ec != std::errc{}) {
        return "0";
    }
    return std::string(buffer, result.ptr);
}

std::string join(const std::vector<double>& values) {
    std::string text;
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index != 0) {
            text.push_back('\\');
        }
        text += formatDecimal(values[index]);
    }
    return text;
}

void fail(std::string* error, const char* message) {
    if (error != nullptr) {
        *error = message;
    }
}

} // namespace

bool writePart10Ct(const std::filesystem::path& path,
                   const DicomCtInstance& instance,
                   int instanceNumber,
                   std::string* error) {
    const int pixelCount = instance.rows * instance.columns;
    if (instance.seriesInstanceUid.empty() || instance.rows <= 0 || instance.columns <= 0 ||
        pixelCount <= 0 || static_cast<int>(instance.storedValues.size()) != pixelCount) {
        fail(error, "CT instance is incomplete");
        return false;
    }

    const std::string sopClass = "1.2.840.10008.5.1.4.1.1.2";
    const std::string sopInstance =
        instance.seriesInstanceUid + "." + std::to_string(instanceNumber);
    const std::string transferSyntax = instance.transferSyntaxUid.empty()
                                           ? std::string("1.2.840.10008.1.2.1")
                                           : instance.transferSyntaxUid;

    std::vector<std::uint8_t> meta;
    appendText(meta, 0x0002, 0x0002, "UI", sopClass);
    appendText(meta, 0x0002, 0x0003, "UI", sopInstance);
    appendText(meta, 0x0002, 0x0010, "UI", transferSyntax);
    appendText(meta, 0x0002, 0x0012, "UI", "1.2.826.0.1.3680043.10.543");

    std::vector<std::uint8_t> file(128, 0);
    file.insert(file.end(), {'D', 'I', 'C', 'M'});
    appendTag(file, 0x0002, 0x0000);
    file.push_back('U');
    file.push_back('L');
    appendU16(file, 4);
    appendU32(file, static_cast<std::uint32_t>(meta.size()));
    file.insert(file.end(), meta.begin(), meta.end());

    appendText(file, 0x0008, 0x0016, "UI", sopClass);
    appendText(file, 0x0008, 0x0018, "UI", sopInstance);
    appendText(file, 0x0008, 0x0060, "CS", "CT");
    appendText(file, 0x0020, 0x000E, "UI", instance.seriesInstanceUid);
    appendText(file, 0x0020, 0x0013, "IS", std::to_string(instanceNumber));
    appendText(file, 0x0020, 0x0032, "DS",
               join({instance.geometry.imagePositionPatient.x,
                     instance.geometry.imagePositionPatient.y,
                     instance.geometry.imagePositionPatient.z}));
    appendText(file,
               0x0020,
               0x0037,
               "DS",
               join({instance.geometry.imageOrientationPatient[0],
                     instance.geometry.imageOrientationPatient[1],
                     instance.geometry.imageOrientationPatient[2],
                     instance.geometry.imageOrientationPatient[3],
                     instance.geometry.imageOrientationPatient[4],
                     instance.geometry.imageOrientationPatient[5]}));
    appendUs(file, 0x0028, 0x0002, 1);
    appendText(file, 0x0028, 0x0004, "CS", "MONOCHROME2");
    appendUs(file, 0x0028, 0x0010, static_cast<std::uint16_t>(instance.rows));
    appendUs(file, 0x0028, 0x0011, static_cast<std::uint16_t>(instance.columns));
    appendText(file, 0x0028, 0x0030, "DS",
               join({instance.geometry.rowSpacingMm, instance.geometry.columnSpacingMm}));
    appendUs(file, 0x0028, 0x0100, 16);
    appendUs(file, 0x0028, 0x0101, 16);
    appendUs(file, 0x0028, 0x0102, 15);
    appendUs(file, 0x0028, 0x0103, 1);
    appendText(file, 0x0028, 0x1052, "DS", formatDecimal(instance.rescale.intercept));
    appendText(file, 0x0028, 0x1053, "DS", formatDecimal(instance.rescale.slope));

    appendTag(file, 0x7FE0, 0x0010);
    file.push_back('O');
    file.push_back('W');
    appendU16(file, 0);
    const auto pixelBytes =
        static_cast<std::uint32_t>(instance.storedValues.size() * sizeof(std::int16_t));
    appendU32(file, pixelBytes);
    for (const std::int16_t sample : instance.storedValues) {
        appendU16(file, static_cast<std::uint16_t>(sample));
    }

    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        fail(error, "Could not write DICOM file");
        return false;
    }
    output.write(reinterpret_cast<const char*>(file.data()),
                 static_cast<std::streamsize>(file.size()));
    if (!output) {
        fail(error, "Could not write DICOM file");
        return false;
    }
    return true;
}

} // namespace medforge::imaging
