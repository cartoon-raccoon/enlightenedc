#pragma once
#pragma clang diagnostic ignored "-Wunused-parameter"

#ifndef ECC_UTIL_H
#define ECC_UTIL_H

#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <source_location>
#include <sstream>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#ifndef NDEBUG
#include <iostream>

template <typename... Args> void dbprint(Args&&...args) {
    (std::cerr << ... << std::forward<Args>(args)) << "\n";
}
#else
template <typename T, typename... Args> void dbprint(T msg, Args&&...args) {
}
#endif

#define DO_ACCEPT(tyname, vistype) /*NOLINT*/            \
    void tyname::accept(vistype& visitor) /* NOLINT */ { \
        visitor.visit(*this);                            \
    }

#define VISIT_NO_IMPL(_node)           /* NOLINT */                                             \
    void visit(_node& node) override { /*NOLINT */                                              \
        throw std::runtime_error("visit() was not implemented for the current visitable node"); \
    }

#define todo() throw Todo(std::source_location::current()) // NOLINT

constexpr std::size_t BOOST_GOLDEN_RATIO = 0x9e3779b9;
constexpr std::size_t HASH_SHL           = 6;
constexpr std::size_t HASH_SHR           = 2;

namespace ecc::util {

/**
An exception class to indicate that a region of code is currently unimplemented.
*/
class Todo : std::exception {
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

template <typename T> using Box = std::unique_ptr<T>;

template <typename T, typename... Args>
auto make_box(Args&&...args) -> decltype(std::make_unique<T>(std::forward<Args>(args)...)) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template <typename T> using Vec = std::vector<T>;

template <typename T> using Optional = std::optional<T>;

template <typename T> using Ref = std::reference_wrapper<T>;

template <typename T1, typename T2> using Pair = std::pair<T1, T2>;

// Overloaded template class for Rust-style pattern matching on variants.
template <class... Ts> struct match : Ts... {
    using Ts::operator()...;
};
template <class... Ts> match(Ts...) -> match<Ts...>;

template <typename... Types> struct VarHash {
    // Helper to combine an individual seed with a new value
    void hash_combine(std::size_t& seed, const auto& val) const {
        std::hash<std::decay_t<decltype(val)>> hasher;
        // The Boost "Golden Ratio" formula
        seed ^= hasher(val) + BOOST_GOLDEN_RATIO + (seed << HASH_SHL) + (seed >> HASH_SHR);
    }

    std::size_t operator()(const Types&...args) const {
        std::size_t seed = 0;
        // C++17 Fold Expression: applies hash_combine to every argument in args
        (hash_combine(seed, args), ...);
        return seed;
    }
};

/*
A basic point in a source file.
*/
class Point {
public:
    int column;
    int line;
    const std::string *filename;

    Point(const std::string *filename, int col, int line)
        : column(col), line(line), filename(filename) {}

    /// Construct an empty Point (no filename, starts at 1:1).
    Point() : column(1), line(1), filename(nullptr) {}

    bool empty() const { return column == 1 && line == 1 && filename == nullptr; }

    /// Add `rhs` columns.
    Point& operator+=(int rhs) {
        this->column += rhs;
        return *this;
    }

    /// Add `lhs` columns.
    Point operator+(int rhs) const { return Point(filename, column + rhs, line); }

    bool operator==(Point& other) const {
        return (*filename == *other.filename) && (column == other.column) && (line == other.line);
    }

    void lines(int count = 1) {
        if (count) {
            column = 1;
            line   = add(line, count, 1);
        }
    }

private:
    static int add(int lhs, int rhs, int min) { return lhs + rhs < min ? min : lhs + rhs; }
};

/** \brief Intercept output stream redirection.
** \param ostr the destination output stream
** \param pos a reference to the position to redirect
*/
template <typename T>
std::basic_ostream<T>& operator<<(std::basic_ostream<T>& ostr, const Point& pos) {
    if (pos.filename && pos.filename->empty())
        ostr << *pos.filename << ':';
    return ostr << pos.line << '.' << pos.column;
}

class Location {
public:
    Point begin;
    Point end;

    /**
    Construct a Location from two points.
    */
    Location(Point start, Point end) : begin(start), end(end) {}

    /// Construct a zero-width location from a Point.
    Location(Point pt) : begin(pt), end(pt) {}

    /// Construct an empty location with a filename.
    Location(std::string *filename) : begin(filename, 1, 1), end(filename, 1, 1) {}

    /// Construct an empty location.
    Location() {}

    void step() { begin = end; }

    void columns(int count = 1) { end += count; }

    void lines(int count = 1) { end.lines(count); }

    /**
    Check if a Location crosses a file boundary.
    */
    bool crosses_files() const { return begin.filename != end.filename; }

    bool empty() const { return begin.empty() && end.empty(); }

    /**
    Check if `other` overlaps with `this`.
    */
    bool operator||(Location& other) const {
        if (((begin.filename != other.begin.filename) || (end.filename != other.end.filename))) {
            return false;
        }

        // overlap: begin.line <= other.begin.line <= other.end

        return false; // todo
    }

    /**
    Check if `other` is fully contained within `this`.

    Note that unlike `||`, this is not symmetric;
    `other` being fully contained within `this` does not imply
    `this` is fully contained within `other`.
    */
    bool operator&&(Location& other) const {
        if (((begin.filename != other.begin.filename) || (end.filename != other.end.filename))) {
            return false;
        }

        return false; // todo
    }

    Location operator&(Location& rhs) const {
        return Location{}; // todo
    }
};

/** \brief Intercept output stream redirection.
** \param ostr the destination output stream
** \param loc a reference to the location to redirect
**
** Avoid duplicate information.
*/
template <typename T>
std::basic_ostream<T>& operator<<(std::basic_ostream<T>& ostr, const Location& loc) {
    int end_col = 0 < loc.end.column ? loc.end.column - 1 : 0;
    ostr << loc.begin;
    if ((loc.begin.filename || loc.begin.filename) && loc.begin.filename != loc.end.filename) {
        ostr << '-' << *loc.end.filename << ':' << loc.end.line << '.' << end_col;
    } else if (loc.begin.line < loc.end.line) {
        ostr << '-' << loc.end.line << '.' << end_col;
    } else if (loc.begin.column < end_col) {
        ostr << '-' << end_col;
    }
    return ostr;
}

// Helper to check if T is in the list of Types...
template <typename T, typename Variant> struct is_variant_member;

template <typename T, typename... Types>
struct is_variant_member<T, std::variant<Types...>>
    : std::bool_constant<(std::is_same_v<T, Types> || ...)> {};

// Concept to check if a type T is a member of a std::variant,
template <typename T, typename Variant>
concept VariantMember = is_variant_member<T, Variant>::value;

/**
A counter that keeps increasing.
*/
template <typename I>
    requires std::is_integral_v<I>
class MonotonicCtr {
    I val;

public:
    MonotonicCtr(I val) : val(val) {}
    MonotonicCtr(const MonotonicCtr<I>& c) : val(c.val) {}

    I value() const { return val; }

    void inc() { val++; }

    I operator*() { return val; }

    I operator++() { return val++; }

    I operator++(int) { return ++val; }

    bool operator==(const MonotonicCtr<I>& other) { return val == other.val; }

    bool operator==(const I& other) { return val == other; }

    bool operator<(const MonotonicCtr<I>& other) { return val < other.val; }

    bool operator>(const I& other) { return val < other; }
};

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

} // namespace ecc::util

// Injecting hash specializations into std namespace
namespace std {

template <> struct hash<ecc::util::Point> {
    size_t operator()(const ecc::util::Point& pt) {
        ecc::util::VarHash<std::string, int, int> hasher;
        return hasher(*pt.filename, pt.line, pt.column);
    }
};

template <> struct hash<ecc::util::Location> {
    size_t operator()(const ecc::util::Location& loc) {
        ecc::util::VarHash<ecc::util::Point, ecc::util::Point> hasher;
        return hasher(loc.begin, loc.end);
    }
};

} // namespace std

#endif