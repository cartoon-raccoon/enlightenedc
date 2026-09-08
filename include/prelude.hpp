#pragma once

#ifndef ECC_UTIL_H
#define ECC_UTIL_H

#include <compare>
#include <concepts>
#include <exception>
#include <source_location>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

// IWYU pragma: begin_exports
#include "util/aliases.hpp"
#include "util/rtti.hpp"
#include "util/assert.hpp"
// IWYU pragma: end_exports

using namespace ecc::alloc;

/*
 * DEBUG PRINTING AND TODOS
 */

#ifndef NDEBUG
#include <iostream>

template <typename... Args>
void dbprint(Args&&...args) {
    (std::cerr << ... << std::forward<Args>(args)) << "\n";
}
#else
template <typename T, typename... Args>
void dbprint(T msg, Args&&...args) {
}
#endif

#define DO_ACCEPT(tyname, vistype) /*NOLINT*/            \
    void tyname::accept(vistype& visitor) /* NOLINT */ { \
        visitor.visit(*this);                            \
    }

#define VISIT_NO_IMPL(_node)           /* NOLINT */                                    \
    void visit(_node& node) override { /*NOLINT */                                     \
        ECC_UNREACHABLE("visit() was not implemented for the current visitable node"); \
    }

#define todo() throw Todo(std::source_location::current()) // NOLINT

namespace ecc::util {

/**
An exception class to indicate that a region of code is currently unimplemented.
*/
class Todo : public std::exception {
public:
    std::string location;

    Todo(std::source_location at) {
        std::stringstream ss;
        ss << at.file_name() << " - ";
        ss << at.function_name();
        ss << " (" << at.line() << ":" << at.column() << ")";
        location = ss.str();
    }

    const char *what() const noexcept override { return location.c_str(); }
};

/*
 * CONCEPTS
 */

// Helper to check if T is in the list of Types...
template <typename T, typename Variant>
struct is_variant_member;

template <typename T, typename... Types>
struct is_variant_member<T, std::variant<Types...>>
    : std::bool_constant<(std::is_same_v<T, Types> || ...)> {};

// Concept to check if a type T is a member of a std::variant,
template <typename T, typename Variant>
concept VariantMember = is_variant_member<T, Variant>::value;

/**
A concept expressing that a container owns its type.

Box and Chunk are both Owners.
*/
template <typename Container, typename T>
concept Owner = std::movable<Container> && requires(Container o) {
    { o.get() } -> std::convertible_to<T *>;
    { o.operator->() } -> std::convertible_to<T *>;
    { o.release() } -> std::convertible_to<T *>;
};

/*
 * UTILITY CLASSES
 */

/**
A counter that keeps increasing.
*/
template <typename I>
    requires std::is_integral_v<I>
class MonotonicCtr {
    I val;

public:
    MonotonicCtr<I>() : val(0) {}

    MonotonicCtr(I val) : val(val) {}

    MonotonicCtr(const MonotonicCtr<I>& c) : val(c.val) {}

    MonotonicCtr(MonotonicCtr<I>&& c) noexcept : val(c.val) {
        c.val = 0;
        // reset the moved-from counter to 0, since it's monotonic and should never decrease.
    }

    I value() const { return val; }

    I inc() { return val++; }

    I add(I n) { return val += n; }

    I operator*() { return val; }

    I operator++() { return ++val; }

    I operator++(int) { return val++; }

    I operator+(I n) const { return val + n; }

    I operator-(I n) const { return val - n; }

    I operator+=(I n) { return add(n); }

    std::strong_ordering operator<=>(const MonotonicCtr<I>& other) { return val <=> other.val; }

    std::strong_ordering operator<=>(const I& other) { return val <=> other; }
};

/*
 * OWNER CONCEPT
 */

/*
 * NOCOPY, NOMOVE
 */

class NoCopy { // NOLINT(cppcoreguidelines-special-member-functions)
public:
    NoCopy(NoCopy const&)            = delete;
    NoCopy& operator=(NoCopy const&) = delete;

    NoCopy(NoCopy&&)            = default;
    NoCopy& operator=(NoCopy&&) = default;
    NoCopy()                    = default;
};

class NoMove { // NOLINT(cppcoreguidelines-special-member-functions)
public:
    NoMove(NoMove&&)            = delete;
    NoMove& operator=(NoMove&&) = delete;

    NoMove() = default;
};

/*
 * MANUAL RTTI FUNCTIONALITY
 */

} // namespace ecc::util

#endif