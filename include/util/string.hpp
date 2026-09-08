#pragma once

#ifndef ECC_UTIL_STRING_H
#define ECC_UTIL_STRING_H


#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <string>
#include <string_view>

#include <llvm/ADT/StringRef.h>

#include "util/assert.hpp"

namespace ecc::util {

/*
 * STRING UTILITIES
 */

/**
Converts non-alphanumeric characters in a string into their escape-sequence
form (e.g. a newline byte becomes the two characters '\\' and 'n'), mirroring
the escape handling performed by `decode_string_literal()` in the lexer, but
in reverse. Backslashes, quote characters, and non-printable bytes are
escaped (falling back to `\xHH` when there's no named escape sequence);
printable punctuation and spaces are left untouched.
*/
std::string encode_string_literal(std::string_view raw);

}

namespace ecc {

/**
Represents a constant reference to a string, i.e. a character array and size,
which may or may not be null-terminated.

Note: StrRef does not own its data. It assumes the backing data is owned by some
other buffer that outlives StrRef. For this reason, it is generally not safe to
store a StrRef.

Credit: shamelessly stolen from LLVM.
*/
class StringRef {
public:
    static constexpr size_t npos = ~size_t(0);
    using iterator = const char *;
    using const_iterator = const char *;
    using size_type = size_t;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
    /** The start of the string, in an external buffer. */
    const char *data_ = nullptr;
    /** The length of the string. */
    size_t size_ = 0;

public:
    StringRef() = default;

    constexpr StringRef(const char *str) : StringRef(std::string_view(str)) {}

    constexpr StringRef(const char *str, size_t length) : data_(str), size_(length) {}

    constexpr StringRef(const std::string& str) : data_(str.data()), size_(str.length()) {}

    constexpr StringRef(std::string_view strv) : data_(strv.data()), size_(strv.size()) {}

    constexpr StringRef(llvm::StringRef& strf) : data_(strf.data()), size_(strf.size()) {}

    iterator begin() const { return data(); }
 
    iterator end() const { return data() + size(); }
 
    reverse_iterator rbegin() const { return std::reverse_iterator(end()); }
 
    reverse_iterator rend() const { return std::reverse_iterator(begin()); }

    constexpr const char *data() const { return data_; }

    constexpr bool empty() const { return size_ == 0; }

    size_t size() const { return size_; }

    [[nodiscard]] char front() const {
        ECC_ASSERT_N(!empty());

        return data()[0];
    }

    [[nodiscard]] char back() const {
        ECC_ASSERT_N(!empty());

        return data()[size() - 1];
    }

    template <typename Allocator>
    [[nodiscard]] StringRef copy(Allocator& a) {
        if (empty()) {
            return StringRef();
        }

        char *s = a.template Allocate<char>(size());
        std::copy(begin(), end(), s);
        return StringRef(s, size());
    }

    // todo: full llvm::StringRef API

};

} // end namespace ecc

#endif