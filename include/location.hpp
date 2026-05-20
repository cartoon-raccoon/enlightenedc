#pragma once
#ifndef ECC_LOCATION_H
#define ECC_LOCATION_H

#include <iostream>

#include "util.hpp"

namespace ecc::location {

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

    // Location operator&(Location& rhs) const {
    //     return Location{}; // todo
    // }
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

} // namespace ecc::location

// Injecting hash specializations into std namespace
namespace std {

template <>
struct hash<ecc::location::Point> {
    size_t operator()(const ecc::location::Point& pt) {
        ecc::util::VarHash<std::string, int, int> hasher;
        return hasher(*pt.filename, pt.line, pt.column);
    }
};

template <>
struct hash<ecc::location::Location> {
    size_t operator()(const ecc::location::Location& loc) {
        ecc::util::VarHash<ecc::location::Point, ecc::location::Point> hasher;
        return hasher(loc.begin, loc.end);
    }
};

} // namespace std

#endif