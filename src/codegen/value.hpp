#ifndef ECC_VALUE_H
#define ECC_VALUE_H

#include "util.hpp"
#include <cstdint>
#include <variant>
#include <string>

using namespace ecc::util;

namespace ecc::exec {

struct StructValue {

};

using Value = std::variant<
    std::monostate,
    uint8_t,
    uint16_t,
    uint32_t,
    uint64_t,
    int8_t,
    int16_t,
    int32_t,
    int64_t,
    double,
    bool,
    std::string
>;

using IntegerValue = std::variant<
    uint8_t,
    uint16_t,
    uint32_t,
    uint64_t,
    int8_t,
    int16_t,
    int32_t,
    int64_t
>;

template<typename T>
requires VariantMember<T, Value>
std::optional<T> value_as(Value& val) {
    if (auto v = std::get_if<T>(val)) {
        return v;
    } else {
        return std::nullopt;
    }
}

template<typename T>
requires VariantMember<T, IntegerValue>
std::optional<T> value_as_int(Value& val) {
    return std::visit([](auto&& arg) -> std::optional<T> {
        using V = std::decay_t<decltype(arg)>;
        if constexpr (VariantMember<V, IntegerValue>) {
            return static_cast<T>(arg);
        }
        return std::nullopt;
    }, val);
}

}

#endif