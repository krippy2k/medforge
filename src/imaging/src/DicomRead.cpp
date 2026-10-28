#include "medforge/imaging/DicomRead.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace medforge::imaging {
namespace {

constexpr std::size_t kPreambleBytes = 128;
constexpr std::uint32_t kUndefinedLength = 0xFFFFFFFFU;
constexpr std::uint32_t kItemTag = 0xFFFEE000U;
constexpr std::uint32_t kItemDelimiter = 0xFFFEE00DU;
constexpr std::uint32_t kSequenceDelimiter = 0xFFFEE0DDU;

class Cursor {
public:
    Cursor(const std::uint8_t* data, std::size_t size) : data_(data), size_(size) {}

    bool truncated() const { return truncated_; }

    void markTruncated() { truncated_ = true; }

    std::optional<std::uint16_t> peekU16() const {
        if (offset_ + 2 > size_) {
            return std::nullopt;
        }
        return static_cast<std::uint16_t>(data_[offset_] | (data_[offset_ + 1] << 8));
    }

    std::optional<std::uint32_t> peekTag() const {
        if (offset_ + 4 > size_) {
            return std::nullopt;
        }
        const auto group = static_cast<std::uint16_t>(data_[offset_] | (data_[offset_ + 1] << 8));
        const auto element =
            static_cast<std::uint16_t>(data_[offset_ + 2] | (data_[offset_ + 3] << 8));
        return (static_cast<std::uint32_t>(group) << 16) | element;
    }

    bool take(std::size_t count, const std::uint8_t*& out) {
        if (offset_ + count > size_) {
            truncated_ = true;
            return false;
        }
        out = data_ + offset_;
        offset_ += count;
        return true;
    }

    std::optional<std::uint16_t> u16() {
        const std::uint8_t* bytes = nullptr;
        if (!take(2, bytes)) {
            return std::nullopt;
        }
        return static_cast<std::uint16_t>(bytes[0] | (bytes[1] << 8));
    }

    std::optional<std::uint32_t> u32() {
        const std::uint8_t* bytes = nullptr;
        if (!take(4, bytes)) {
            return std::nullopt;
        }
        return static_cast<std::uint32_t>(bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) |
                                          (bytes[3] << 24));
    }

private:
    const std::uint8_t* data_ = nullptr;
    std::size_t size_ = 0;
    std::size_t offset_ = 0;
    bool truncated_ = false;
};

bool isLongValueRepresentation(std::string_view vr) {
    return vr == "OB" || vr == "OD" || vr == "OF" || vr == "OL" || vr == "OV" || vr == "OW" ||
           vr == "SQ" || vr == "SV" || vr == "UC" || vr == "UN" || vr == "UR" || vr == "UT" ||
           vr == "UV";
}

bool isKnownVr(std::string_view vr) {
    static constexpr std::string_view kVrs[] = {
        "AE", "AS", "AT", "CS", "DA", "DS", "DT", "FL", "FD", "IS", "LO", "LT", "OB", "OD", "OF",
        "OL", "OV", "OW", "PN", "SH", "SL", "SQ", "SS", "ST", "SV", "TM", "UC", "UI", "UL", "UN",
        "UR", "US", "UT", "UV"};
    return std::find(std::begin(kVrs), std::end(kVrs), vr) != std::end(kVrs);
}

std::string trimmed(std::string_view value) {
    while (!value.empty() && (value.front() == ' ' || value.front() == '\0')) {
        value.remove_prefix(1);
    }
    while (!value.empty() && (value.back() == ' ' || value.back() == '\0')) {
        value.remove_suffix(1);
    }
    return std::string(value);
}

std::string_view trimToken(std::string_view token) {
    while (!token.empty() && (token.front() == ' ' || token.front() == '\0')) {
        token.remove_prefix(1);
    }
    while (!token.empty() && (token.back() == ' ' || token.back() == '\0')) {
        token.remove_suffix(1);
    }
    return token;
}

std::vector<double> parseDoubles(std::string_view text) {
    std::vector<double> values;
    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t end = text.find('\\', start);
        const std::string_view token = trimToken(text.substr(
            start, end == std::string_view::npos ? std::string_view::npos : end - start));
        if (token.empty()) {
            return {};
        }
        double value = 0;
        const auto parsed = std::from_chars(token.data(), token.data() + token.size(), value,
                                            std::chars_format::general);
        if (parsed.ec != std::errc{} || parsed.ptr != token.data() + token.size()) {
            return {};
        }
        values.push_back(value);
        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1;
    }
    return values;
}

struct Element {
    std::uint16_t group = 0;
    std::uint16_t element = 0;
    std::string vr;
    std::vector<std::uint8_t> value;
};

struct ReadResult {
    bool ok = false;
    bool unsupportedPixels = false;
    Element element;
};

ReadResult readExplicit(Cursor& cursor);
ReadResult readImplicit(Cursor& cursor);

bool skipUndefinedItem(Cursor& cursor, bool explicitVr);

bool skipUndefinedSequence(Cursor& cursor, bool explicitVr) {
    while (true) {
        const auto tag = cursor.peekTag();
        if (!tag) {
            cursor.markTruncated();
            return false;
        }
        if (*tag == kSequenceDelimiter) {
            cursor.u16();
            cursor.u16();
            return cursor.u32().has_value();
        }
        if (*tag != kItemTag) {
            return false;
        }
        cursor.u16();
        cursor.u16();
        const auto length = cursor.u32();
        if (!length) {
            return false;
        }
        if (*length == kUndefinedLength) {
            if (!skipUndefinedItem(cursor, explicitVr)) {
                return false;
            }
        } else {
            const std::uint8_t* ignored = nullptr;
            if (!cursor.take(*length, ignored)) {
                return false;
            }
        }
    }
}

bool skipUndefinedItem(Cursor& cursor, bool explicitVr) {
    while (true) {
        const auto tag = cursor.peekTag();
        if (!tag) {
            cursor.markTruncated();
            return false;
        }
        if (*tag == kItemDelimiter) {
            cursor.u16();
            cursor.u16();
            return cursor.u32().has_value();
        }
        if (*tag == kSequenceDelimiter || *tag == kItemTag) {
            return false;
        }
        const ReadResult nested = explicitVr ? readExplicit(cursor) : readImplicit(cursor);
        if (!nested.ok) {
            return false;
        }
    }
}

bool readFragments(Cursor& cursor, std::vector<std::vector<std::uint8_t>>& items) {
    while (true) {
        const auto tag = cursor.peekTag();
        if (!tag) {
            cursor.markTruncated();
            return false;
        }
        if (*tag == kSequenceDelimiter) {
            cursor.u16();
            cursor.u16();
            return cursor.u32().has_value();
        }
        if (*tag != kItemTag) {
            return false;
        }
        cursor.u16();
        cursor.u16();
        const auto length = cursor.u32();
        if (!length || *length == kUndefinedLength) {
            return false;
        }
        const std::uint8_t* bytes = nullptr;
        if (!cursor.take(*length, bytes)) {
            return false;
        }
        items.emplace_back(bytes, bytes + *length);
    }
}

ReadResult readUndefinedPixels(Cursor& cursor, ReadResult result) {
    std::vector<std::vector<std::uint8_t>> items;
    if (!readFragments(cursor, items)) {
        result.unsupportedPixels = !cursor.truncated();
        return result;
    }
    if (items.empty()) {
        result.unsupportedPixels = true;
        return result;
    }
    if (items.size() == 1) {
        result.element.value = std::move(items[0]);
    } else if (items[0].empty() || items[0].size() % 4 == 0) {
        for (std::size_t index = 1; index < items.size(); ++index) {
            result.element.value.insert(result.element.value.end(), items[index].begin(),
                                        items[index].end());
        }
    } else {
        result.unsupportedPixels = true;
        return result;
    }
    result.element.vr = "OW";
    result.ok = true;
    return result;
}

ReadResult readExplicit(Cursor& cursor) {
    ReadResult result;
    const auto group = cursor.u16();
    const auto element = cursor.u16();
    const std::uint8_t* vrBytes = nullptr;
    if (!group || !element || !cursor.take(2, vrBytes)) {
        return result;
    }
    result.element.group = *group;
    result.element.element = *element;
    result.element.vr.assign(reinterpret_cast<const char*>(vrBytes), 2);
    std::uint32_t length = 0;
    if (isLongValueRepresentation(result.element.vr)) {
        const std::uint8_t* reserved = nullptr;
        if (!cursor.take(2, reserved)) {
            return result;
        }
        const auto wide = cursor.u32();
        if (!wide) {
            return result;
        }
        length = *wide;
    } else {
        const auto shortLength = cursor.u16();
        if (!shortLength) {
            return result;
        }
        length = *shortLength;
    }
    const bool pixels = result.element.group == 0x7FE0 && result.element.element == 0x0010;
    if (length == kUndefinedLength) {
        if (pixels) {
            return readUndefinedPixels(cursor, std::move(result));
        }
        if (!skipUndefinedSequence(cursor, true)) {
            return result;
        }
        result.ok = true;
        return result;
    }
    const std::uint8_t* value = nullptr;
    if (!cursor.take(length, value)) {
        return result;
    }
    if (length > 0) {
        result.element.value.assign(value, value + length);
    }
    result.ok = true;
    return result;
}

ReadResult readImplicit(Cursor& cursor) {
    ReadResult result;
    const auto group = cursor.u16();
    const auto element = cursor.u16();
    const auto length = cursor.u32();
    if (!group || !element || !length) {
        return result;
    }
    result.element.group = *group;
    result.element.element = *element;
    const bool pixels = *group == 0x7FE0 && *element == 0x0010;
    if (*length == kUndefinedLength) {
        if (pixels) {
            result.element.vr = "OW";
            return readUndefinedPixels(cursor, std::move(result));
        }
        if (!skipUndefinedSequence(cursor, false)) {
            return result;
        }
        result.element.vr = "SQ";
        result.ok = true;
        return result;
    }
    const std::uint8_t* value = nullptr;
    if (!cursor.take(*length, value)) {
        return result;
    }
    result.element.vr = pixels ? "OW" : "UN";
    if (*length > 0) {
        result.element.value.assign(value, value + *length);
    }
    result.ok = true;
    return result;
}

DicomCtInstance fail(DicomReadStatus status, std::string transferSyntax = {}) {
    DicomCtInstance instance;
    instance.status = status;
    instance.transferSyntaxUid = std::move(transferSyntax);
    return instance;
}

std::string_view elementText(const Element& element) {
    if (element.value.empty()) {
        return {};
    }
    return {reinterpret_cast<const char*>(element.value.data()), element.value.size()};
}

DicomCtInstance parseDataset(Cursor& cursor, bool explicitLittleEndian,
                             const std::string& transferSyntax) {
    DicomCtInstance instance;
    instance.transferSyntaxUid = transferSyntax;
    bool sawPixelData = false;
    while (!sawPixelData) {
        const auto group = cursor.peekU16();
        if (!group) {
            return fail(DicomReadStatus::Truncated, transferSyntax);
        }
        const bool fileMeta = *group == 0x0002;
        const ReadResult decoded =
            explicitLittleEndian || fileMeta ? readExplicit(cursor) : readImplicit(cursor);
        if (decoded.unsupportedPixels) {
            return fail(DicomReadStatus::UnsupportedPixelData, transferSyntax);
        }
        if (!decoded.ok) {
            return fail(cursor.truncated() ? DicomReadStatus::Truncated
                                           : DicomReadStatus::MissingAttribute,
                        transferSyntax);
        }
        const Element& element = decoded.element;
        const std::string_view text = elementText(element);
        const auto tag = (static_cast<std::uint32_t>(element.group) << 16) | element.element;
        if (tag == 0x0020000E) {
            instance.seriesInstanceUid = trimmed(text);
        } else if (tag == 0x00200032) {
            const auto values = parseDoubles(trimmed(text));
            if (values.size() != 3) {
                return fail(DicomReadStatus::MissingAttribute, transferSyntax);
            }
            instance.geometry.imagePositionPatient = core::Vec3{values[0], values[1], values[2]};
        } else if (tag == 0x00200037) {
            const auto values = parseDoubles(trimmed(text));
            if (values.size() != 6) {
                return fail(DicomReadStatus::MissingAttribute, transferSyntax);
            }
            std::copy(values.begin(), values.end(),
                      instance.geometry.imageOrientationPatient.begin());
        } else if (tag == 0x00280010 && element.value.size() == 2) {
            instance.rows = element.value[0] | (element.value[1] << 8);
        } else if (tag == 0x00280011 && element.value.size() == 2) {
            instance.columns = element.value[0] | (element.value[1] << 8);
        } else if (tag == 0x00280030) {
            const auto values = parseDoubles(trimmed(text));
            if (values.size() != 2) {
                return fail(DicomReadStatus::MissingAttribute, transferSyntax);
            }
            instance.geometry.rowSpacingMm = values[0];
            instance.geometry.columnSpacingMm = values[1];
        } else if (tag == 0x00280100 && element.value.size() == 2) {
            const int bits = element.value[0] | (element.value[1] << 8);
            if (bits != 16) {
                return fail(DicomReadStatus::UnsupportedPixelData, transferSyntax);
            }
        } else if (tag == 0x00281052) {
            const auto values = parseDoubles(trimmed(text));
            if (values.size() != 1) {
                return fail(DicomReadStatus::MissingAttribute, transferSyntax);
            }
            instance.rescale.intercept = values[0];
        } else if (tag == 0x00281053) {
            const auto values = parseDoubles(trimmed(text));
            if (values.size() != 1) {
                return fail(DicomReadStatus::MissingAttribute, transferSyntax);
            }
            instance.rescale.slope = values[0];
        } else if (tag == 0x7FE00010) {
            if (element.vr != "OW" && element.vr != "OB") {
                return fail(DicomReadStatus::UnsupportedPixelData, transferSyntax);
            }
            if (element.value.size() % 2 != 0) {
                return fail(DicomReadStatus::UnsupportedPixelData, transferSyntax);
            }
            instance.storedValues.resize(element.value.size() / 2);
            for (std::size_t index = 0; index < instance.storedValues.size(); ++index) {
                const std::uint16_t word = static_cast<std::uint16_t>(
                    element.value[index * 2] | (element.value[index * 2 + 1] << 8));
                instance.storedValues[index] = static_cast<std::int16_t>(word);
            }
            sawPixelData = true;
        }
    }

    if (instance.seriesInstanceUid.empty() || instance.rows <= 0 || instance.columns <= 0 ||
        instance.geometry.rowSpacingMm <= 0 || instance.geometry.columnSpacingMm <= 0 ||
        static_cast<int>(instance.storedValues.size()) != instance.rows * instance.columns) {
        return fail(DicomReadStatus::MissingAttribute, transferSyntax);
    }
    instance.status = DicomReadStatus::Ok;
    return instance;
}

bool looksLikeExplicitDataset(const std::vector<std::uint8_t>& bytes) {
    if (bytes.size() < 8) {
        return false;
    }
    const auto group = static_cast<std::uint16_t>(bytes[0] | (bytes[1] << 8));
    if (group != 0x0002 && group != 0x0008 && group != 0x0010 && group != 0x0020 &&
        group != 0x0028) {
        return false;
    }
    const auto first = static_cast<unsigned char>(bytes[4]);
    const auto second = static_cast<unsigned char>(bytes[5]);
    if (std::isupper(first) == 0 || std::isupper(second) == 0) {
        return false;
    }
    const char vr[2] = {static_cast<char>(first), static_cast<char>(second)};
    return isKnownVr(std::string_view(vr, 2));
}

bool looksLikeImplicitDataset(const std::vector<std::uint8_t>& bytes) {
    if (bytes.size() < 8) {
        return false;
    }
    const auto group = static_cast<std::uint16_t>(bytes[0] | (bytes[1] << 8));
    if (group != 0x0008 && group != 0x0010 && group != 0x0020 && group != 0x0028) {
        return false;
    }
    const auto length = static_cast<std::uint32_t>(bytes[4] | (bytes[5] << 8) | (bytes[6] << 16) |
                                                   (bytes[7] << 24));
    return length <= bytes.size() - 8;
}

} // namespace

DicomCtInstance readUncompressedCt(const std::vector<std::uint8_t>& bytes) {
    const bool part10 = bytes.size() >= kPreambleBytes + 4 &&
                        std::memcmp(bytes.data() + kPreambleBytes, "DICM", 4) == 0;
    if (!part10) {
        if (looksLikeExplicitDataset(bytes)) {
            Cursor cursor(bytes.data(), bytes.size());
            return parseDataset(cursor, true, "1.2.840.10008.1.2.1");
        }
        if (looksLikeImplicitDataset(bytes)) {
            Cursor cursor(bytes.data(), bytes.size());
            return parseDataset(cursor, false, "1.2.840.10008.1.2");
        }
        return fail(DicomReadStatus::NotDicom);
    }

    Cursor cursor(bytes.data() + kPreambleBytes + 4, bytes.size() - kPreambleBytes - 4);
    std::string transferSyntax;
    bool sawTransferSyntax = false;
    while (!sawTransferSyntax) {
        const ReadResult decoded = readExplicit(cursor);
        if (!decoded.ok) {
            return fail(cursor.truncated() ? DicomReadStatus::Truncated
                                           : DicomReadStatus::MissingAttribute);
        }
        if (decoded.element.group != 0x0002) {
            return fail(DicomReadStatus::MissingAttribute);
        }
        if (decoded.element.element == 0x0010) {
            transferSyntax = trimmed(elementText(decoded.element));
            sawTransferSyntax = true;
        }
    }

    const bool implicit = transferSyntax == "1.2.840.10008.1.2";
    const bool explicitLittleEndian = transferSyntax == "1.2.840.10008.1.2.1";
    if (!implicit && !explicitLittleEndian) {
        return fail(DicomReadStatus::UnsupportedTransferSyntax, transferSyntax);
    }
    return parseDataset(cursor, explicitLittleEndian, transferSyntax);
}

} // namespace medforge::imaging
