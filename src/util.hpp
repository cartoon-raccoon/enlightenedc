#ifndef ECC_UTIL_H
#define ECC_UTIL_H

#include <memory>
#include <vector>
#include <variant>
#include <type_traits>

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

template<typename T>
using Box = std::unique_ptr<T>;

template<typename T>
using Vec = std::vector<T>;

// Overloaded template class for Rust-style pattern matching on variants.
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

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

#endif