#ifndef ECC_UTIL_H
#define ECC_UTIL_H

#include <exception>
#include <memory>
#include <optional>
#include <sstream>
#include <vector>
#include <variant>
#include <type_traits>
#include <source_location>
#include <functional>

#ifndef NDEBUG
#include <iostream>

template <typename ... Args>
void dbprint(Args&&... args) {
    (std::cout << ... << args) << "\n";
}
#else
template <typename T, typename ... Args>
void dbprint(T msg, Args&&... args) {}
#endif

namespace ecc::util {

#define todo() throw Todo(std::source_location::current())

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

    const char *what() const noexcept override {
        return location.c_str();
    }
};

template<typename T>
using Box = std::unique_ptr<T>;

template<typename T>
using Vec = std::vector<T>;

template<typename T>
using Optional = std::optional<T>;

// Overloaded template class for Rust-style pattern matching on variants.
template<class... Ts> struct match : Ts... { using Ts::operator()...; };
template<class... Ts> match(Ts...) -> match<Ts...>;

template <typename ... Types>
struct VarHash {
    // Helper to combine an individual seed with a new value
    void hash_combine(std::size_t& seed, const auto& v) const {
        std::hash<std::decay_t<decltype(v)>> hasher;
        // The Boost "Golden Ratio" formula
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    std::size_t operator()(const Types&... args) const {
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
    : filename(filename), column(col), line(line) {}

    /// Construct an empty Point (no filename, starts at 1:1).
    Point() : column(1), line(1), filename(nullptr) {}

    inline bool empty() {
        return column == 1 && line == 1 && filename == nullptr;
    }

    /// Add `rhs` columns.
    inline Point&
    operator+= (int rhs) {
        this->column += rhs;
        return *this;
    }

    /// Add `lhs` columns.
    inline Point
    operator+ (int rhs) {
        return Point(filename, column + rhs, line);
    }

    inline bool
    operator== (Point& other) const {
        return
            (*filename == *other.filename) &&
            (column == other.column) &&
            (line == other.line);
    }

    void lines(int count = 1) {
        if (count) {
            column = 1;
            line = add(line, count, 1);
        }
    }

private:
    static int add(int lhs, int rhs, int min) {
        return lhs + rhs < min ? min : lhs + rhs;
    }
};

/** \brief Intercept output stream redirection.
** \param ostr the destination output stream
** \param pos a reference to the position to redirect
*/
template <typename T>
std::basic_ostream<T>&
operator<< (std::basic_ostream<T>& ostr, const Point& pos)
{
    if (pos.filename && pos.filename->length() != 0)
        ostr << *pos.filename << ':';
    return ostr << pos.line << '.' << pos.column;
}

class Location {
public:
    Point begin;
    Point end;

    /// Construct a Location from two points.
    Location(Point start, Point end) : begin(std::move(start)), end(std::move(end)) {}

    /// Construct a zero-width location from a Point.
    Location(Point pt) : begin(pt), end(pt) {}

    /// Construct an empty location with a filename.
    Location(std::string *filename) : begin(filename, 1, 1), end(filename, 1, 1) {}

    /// Construct an empty location.
    Location() {}

    void step() {
        begin = end;
    }

    void columns(int count = 1) {
        end += count;
    }

    void lines(int count = 1) {
        end.lines(count);
    }

    /**
    Check if a Location crosses a file boundary.
    */
    inline bool crosses_files() {
        return begin.filename != end.filename;
    }

    inline bool empty() {
        return begin.empty() && end.empty();
    }

    /**
    Check if `other` overlaps with `this`.
    */
    inline bool 
    operator||(Location& other) const {
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
    inline bool 
    operator&&(Location& other) const {
        if (((begin.filename != other.begin.filename) || (end.filename != other.end.filename))) {
            return false;
        }

        return false; // todo
    }

    inline Location
    operator&(Location& rhs) const {
        return Location {}; // todo
    }
};

/** \brief Intercept output stream redirection.
** \param ostr the destination output stream
** \param loc a reference to the location to redirect
**
** Avoid duplicate information.
*/
template <typename T>
std::basic_ostream<T>&
operator<< (std::basic_ostream<T>& ostr, const Location& loc) {
    int end_col = 0 < loc.end.column ? loc.end.column - 1 : 0;
    ostr << loc.begin;
    if ((loc.begin.filename || loc.begin.filename) && loc.begin.filename != loc.end.filename)
        ostr << '-' << *loc.end.filename << ':' << loc.end.line << '.' << end_col;
    else if (loc.begin.line < loc.end.line)
        ostr << '-' << loc.end.line << '.' << end_col;
    else if (loc.begin.column < end_col)
        ostr << '-' << end_col;
    return ostr;
}

// Helper to check if T is in the list of Types...
template <typename T, typename Variant>
struct is_variant_member;

template <typename T, typename... Types>
struct is_variant_member<T, std::variant<Types...>> 
    : std::bool_constant<(std::is_same_v<T, Types> || ...)> {};

// Concept to check if a type T is a member of a std::variant,
template <typename T, typename Variant>
concept VariantMember = is_variant_member<T, Variant>::value;

}

// Injecting hash specializations into std namespace
namespace std {

template<>
struct hash<ecc::util::Point> {
    size_t operator() (const ecc::util::Point& pt) {
        ecc::util::VarHash<std::string, int, int> hasher;
        return hasher(*pt.filename, pt.line, pt.column);
    }
};


template<>
struct hash<ecc::util::Location> {
    size_t operator() (const ecc::util::Location& loc) {
        ecc::util::VarHash<ecc::util::Point, ecc::util::Point> hasher;
        return hasher(loc.begin, loc.end);
    }
};

}

#endif