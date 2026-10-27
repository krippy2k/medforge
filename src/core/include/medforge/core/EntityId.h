#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace medforge::core {

class EntityId {
public:
    EntityId() = default;

    static EntityId fromValue(std::uint64_t value);
    static EntityId generate();
    static EntityId fromString(std::string_view text);

    bool valid() const noexcept { return value_ != 0; }
    std::uint64_t value() const noexcept { return value_; }
    std::string toString() const;

    friend bool operator==(const EntityId& left, const EntityId& right) noexcept {
        return left.value_ == right.value_;
    }

private:
    explicit EntityId(std::uint64_t value) : value_(value) {}

    std::uint64_t value_ = 0;
};

} // namespace medforge::core
