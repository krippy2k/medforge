#include "medforge/core/EntityId.h"

#include <charconv>
#include <mutex>
#include <random>

namespace medforge::core {
namespace {

constexpr char kHex[] = "0123456789abcdef";

} // namespace

EntityId EntityId::fromValue(std::uint64_t value) {
    return EntityId{value};
}

EntityId EntityId::generate() {
    static std::mutex mutex;
    static std::mt19937_64 generator{std::random_device{}()};
    std::lock_guard<std::mutex> lock(mutex);
    std::uint64_t value = 0;
    do {
        value = generator();
    } while (value == 0);
    return EntityId{value};
}

EntityId EntityId::fromString(std::string_view text) {
    if (text.empty() || text.size() > 16) {
        return EntityId{};
    }
    std::uint64_t value = 0;
    const char* begin = text.data();
    const char* end = text.data() + text.size();
    const auto parsed = std::from_chars(begin, end, value, 16);
    if (parsed.ec != std::errc{} || parsed.ptr != end) {
        return EntityId{};
    }
    return EntityId{value};
}

std::string EntityId::toString() const {
    std::string text(16, '0');
    std::uint64_t value = value_;
    for (int nibble = 15; nibble >= 0; --nibble) {
        text[static_cast<std::size_t>(nibble)] = kHex[static_cast<std::size_t>(value & 0x0fU)];
        value >>= 4U;
    }
    return text;
}

} // namespace medforge::core
