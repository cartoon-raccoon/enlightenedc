#pragma once

#ifndef ECC_UTIL_ALIASES_H
#define ECC_UTIL_ALIASES_H

#include <boost/container_hash/hash.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "allocator/alloc.hpp"

namespace ecc::util {

/**
A convenient type alias for `std::unique_ptr`.
*/
template <typename T>
using Box = std::unique_ptr<T>;

template <typename T, typename... Args>
auto make_box(Args&&...args) -> decltype(std::make_unique<T>(std::forward<Args>(args)...)) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template <typename T>
using Rc = std::shared_ptr<T>;

template <typename T, typename... Args>
auto make_rc(Args&&...args) -> decltype(std::make_shared<T>(std::forward<Args>(args)...)) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

/**
An arena-allocated string.
*/
using ArenaStr = std::basic_string<char, std::char_traits<char>, alloc::ArenaAllocator<char>>;

/**
A convenient type alias for `std::string`.
*/
using Str = std::string;

/**
A convenient type alias for `std::monostate`.
*/
using EmptyVar = std::monostate;

/**
A convenient type alias for `std::vector`.
*/
template <typename T>
using Vec = std::vector<T>;

/**
A convenient type alias for `boost::unordered_map`.
*/
template <typename... Args>
using HashMap = boost::unordered_map<Args...>;

/**
A convenient type alias for `boost::unordered_set`.
*/
template <typename... Args>
using HashSet = boost::unordered_set<Args...>;

/**
A convenient type alias for `std::span`.
*/
template <typename T>
using Span = std::span<T>;

/**
A convenient type alias for `std::optional`.
*/
template <typename T>
using Optional = std::optional<T>;

/**
A convenient type alias for `std::reference_wrapper`.
*/
template <typename T>
using Ref = std::reference_wrapper<T>;

/**
A convenient type alias for `std::pair`.
*/
template <typename T1, typename T2>
using Pair = std::pair<T1, T2>;

// Overloaded template class for Rust-style pattern matching on variants.
template <class... Ts>
struct match : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
match(Ts...) -> match<Ts...>;

}

#endif