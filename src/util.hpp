#ifndef ECC_UTIL_H
#define ECC_UTIL_H

#include <memory>
#include <vector>

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
    int col;
    int line;
    std::string filename;

    Point(std::string filename, int col, int line)
    : filename(filename), col(col), line(line) {}

    /// Add `rhs` columns.
    inline Point&
    operator+= (int rhs) {
        this->col += rhs;
        return *this;
    }

    /// Add `lhs` columns.
    inline Point
    operator+ (int rhs) {
        return Point(filename, col + rhs, line);
    }

    void step_line(int count = 1) {
        line += count;
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
    if (pos.filename.length() != 0)
        ostr << pos.filename << ':';
    return ostr << pos.line << '.' << pos.col;
}

class Location {
public:
    Point start;
    Point end;

    /// Construct a Location from two points.
    Location(Point start, Point end) : start(std::move(start)), end(std::move(end)) {}

    /// Construct a zero-width location from a Point.
    Location(Point pt) : start(pt), end(pt) {}
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
    int end_col = 0 < loc.end.col ? loc.end.col - 1 : 0;
    ostr << loc.start;
    if (loc.start.filename != loc.end.filename)
        ostr << '-' << loc.end.filename << ':' << loc.end.line << '.' << end_col;
    else if (loc.start.line < loc.end.line)
        ostr << '-' << loc.end.line << '.' << end_col;
    else if (loc.start.col < end_col)
        ostr << '-' << end_col;
    return ostr;
}

}

#endif