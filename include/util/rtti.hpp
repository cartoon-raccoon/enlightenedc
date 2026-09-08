#pragma once

#ifndef ECC_UTIL_RTTI_H
#define ECC_UTIL_RTTI_H

#include "util/aliases.hpp"
#include "util/assert.hpp"
#include "allocator/chunk.hpp"

namespace ecc::util {

template <typename To, typename From>
concept HasClassof = requires(const From *f) {
    { To::classof(f) } -> std::convertible_to<bool>;
};

template <typename To, typename From>
bool isa(const From *val) {
    static_assert(std::is_base_of_v<From, To>);
    if constexpr (std::is_same_v<To, From>) {
        return true;
    } else {
        static_assert(HasClassof<To, From>);
        return To::classof(val);
    }
}

template <typename To, typename From>
bool isa(const Box<From>& val) {
    static_assert(std::is_base_of_v<From, To>);
    if constexpr (std::is_same_v<To, From>) {
        return true;
    } else {
        static_assert(HasClassof<To, From>);
        return To::classof(val.get());
    }
}

template <typename To, typename From>
bool isa(const alloc::Chunk<From>& val) {
    static_assert(std::is_base_of_v<From, To>);
    if constexpr (std::is_same_v<To, From>) {
        return true;
    } else {
        static_assert(HasClassof<To, From>);
        return To::classof(val.get());
    }
}

template <typename To, typename From>
To *cast(From *val) {
    ECC_ASSERT(val && isa<To>(val), "dyncast<>: incompatible types of To and From");
    return static_cast<To *>(val);
}

template <typename To, typename From>
const To *cast(const From *val) {
    ECC_ASSERT(val && isa<To>(val), "dyncast<>: incompatible types of To and From");
    return static_cast<const To *>(val);
}

template <typename To, typename From>
To *cast(const Box<From>& val) {
    return cast<To>(val.get());
}

template <typename To, typename From>
To *cast(const alloc::Chunk<From>& val) {
    return cast<To>(val.get());
}

template <typename To, typename From>
To *dyncast(From *val) {
    return val && isa<To>(val) ? static_cast<To *>(val) : nullptr;
}

template <typename To, typename From>
const To *dyncast(const From *val) {
    return val && isa<To>(val) ? static_cast<const To *>(val) : nullptr;
}

template <typename To, typename From>
To *dyncast(Box<From>& val) {
    return dyncast<To>(val.get());
}

template <typename To, typename From>
To *dyncast(alloc::Chunk<From>& val) {
    return dyncast<To>(val.get());
}

}

#endif