#pragma once

#ifndef ECC_STRINGSWITCH_H
#define ECC_STRINGSWITCH_H

#include <initializer_list>

#include "util/aliases.hpp"
#include "util/string.hpp"

namespace ecc::util {

/**
A helper class for matching on StringRefs like a switch-case statement.
*/
template <typename T>
class StringSwitch {
    const StringRef str;
    Optional<T> result;
public:
    StringSwitch(StringRef str) : str(str), result() {}

    StringSwitch(StringSwitch&&) = default;

    StringSwitch(const StringSwitch&) = delete;

    // StringSwitch is not assignable, because str is const.
    void operator=(const StringSwitch&) = delete;
    void operator=(StringSwitch&&) = delete;

    // todo: add more of the API as the StringRef API grows

    /**
    A single case.
    */
    StringSwitch& Case(StringLiteral s, T value) {
        case_impl(s, value);
        return *this;
    }

    StringSwitch& StartsWith(StringLiteral s, T value) {
        if (!result && str.starts_with(s)) {
            result = std::move(value);
        }
        return *this;
    }

    StringSwitch& EndsWith(StringLiteral s, T value) {
        if (!result && str.ends_with(s)) {
            result = std::move(value);
        }
        return *this;
    }

    StringSwitch& AnyOf(std::initializer_list<StringLiteral> cases, T value) {
        for (auto s : cases) {
            if (case_impl(s, value)) break;
        }

        return *this;
    }

    [[nodiscard]] T Default(T value) {
        if (result) {
            return *std::move(result);
        }

        return value;
    }

private:
    bool case_impl(StringLiteral s, T value) {
        if (result) {
            return true;
        }

        if (str != s) {
            return false;
        }

        result = std::move(value);
        return true;
    }
};

}

#endif