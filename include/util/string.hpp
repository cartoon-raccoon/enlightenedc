#pragma once

#ifndef ECC_UTIL_STRING_H
#define ECC_UTIL_STRING_H


#include <algorithm>
#include <cstddef>
#include <cstring>
#include <format>
#include <iterator>
#include <ostream>
#include <string>
#include <string_view>

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/xxhash.h>
#include <boost/unordered/unordered_set.hpp>

#include "util/assert.hpp"

namespace ecc {
class StringRef;
}

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
std::string encode_string_literal(StringRef raw);

}

namespace ecc {

/**
Represents a constant reference to a string, i.e. a character array and size,
which may or may not be null-terminated.

`StringRef` is the central object in `ecc` with which strings are passed and
queried. It does not own its string, which makes passing it around copy-free and
cheap. `ecc` dedups strings and stores them on the global arena, which lasts as
long as the translation unit and thus outlives the AST/IR nodes themselves, and
so having nodes store a reference back to the string instead of storing the string
itself means passing strings around and between nodes is very efficient.

Note: StrRef does not own its data. It assumes the backing data is owned by some
other buffer that outlives StrRef. For this reason, it is generally not safe to
store a StrRef, outside of situations (like in `ecc`) where strings are owned by
a long-lived backing store such as an arena.

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

    static int compare_memory(const char *lhs, const char *rhs, size_t n) {
        if (n == 0) return 0;

        return ::memcmp(lhs, rhs, n);
    }

public:
    StringRef() = default;

    constexpr StringRef(const char *str) : StringRef(std::string_view(str)) {}

    constexpr StringRef(const char *str, size_t length) : data_(str), size_(length) {}

    /** From any `std::basic_string<char>`, regardless of allocator (e.g. `ArenaStr`). */
    template <typename Alloc>
    constexpr StringRef(const std::basic_string<char, std::char_traits<char>, Alloc>& str)
        : data_(str.data()), size_(str.length()) {}

    constexpr StringRef(std::string_view strv) : data_(strv.data()), size_(strv.size()) {}

    constexpr StringRef(llvm::StringRef& strf) : data_(strf.data()), size_(strf.size()) {}

    constexpr operator std::string_view() const { return std::string_view(data_, size_); }

    constexpr operator llvm::StringRef() const { return llvm::StringRef(data_, size_); }

    iterator begin() const { return data(); }
 
    iterator end() const { return data() + size(); }
 
    reverse_iterator rbegin() const { return std::reverse_iterator(end()); }
 
    reverse_iterator rend() const { return std::reverse_iterator(begin()); }

    constexpr const char *data() const { return data_; }

    constexpr bool empty() const { return size_ == 0; }

    constexpr size_t size() const { return size_; }

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

    /** Materialise an owning copy on the heap. */
    [[nodiscard]] std::string str() const { return std::string(data_, size_); }

    /** 
    Compare two strings; the result is negative, zero, or positive if this
    string is lexicographically less than, equal to, or greater than the
    RHS.
    */
    [[nodiscard]] int compare(StringRef& rhs) const {
        // Check the prefix for a mismatch.
        if (int Res = compare_memory(data(), rhs.data(), std::min(size(), rhs.size())))
            return Res < 0 ? -1 : 1;
    
        // Otherwise the prefixes match, so we only need to check the lengths.
        if (size() == rhs.size()) return 0;

        return size() < rhs.size() ? -1 : 1;
    }


    // todo: full llvm::StringRef API

};


inline bool operator==(StringRef lhs, StringRef rhs) {
    if (lhs.size() != rhs.size()) return false;

    if (lhs.empty()) return true;

    return ::memcmp(lhs.data(), rhs.data(), lhs.size()) == 0;
}

inline bool operator>(StringRef lhs, StringRef rhs) {
    return lhs.compare(rhs) > 0;
}

inline bool operator<(StringRef lhs, StringRef rhs) {
    return lhs.compare(rhs) < 0;
}

inline bool operator>=(StringRef lhs, StringRef rhs) {
    return lhs.compare(rhs) >= 0;
}

inline bool operator<=(StringRef lhs, StringRef rhs) {
    return lhs.compare(rhs) <= 0;
}

inline std::ostream& operator<<(std::ostream& os, StringRef s) {
    if (!s.empty()) {
        os.write(s.data(), static_cast<std::streamsize>(s.size()));
    }
    return os;
}

/*
Hash and equality functors for keying a hash container on string content.

Both take `StringRef` by value. `std::string`, `ArenaStr`, `std::string_view`, and
`const char *` each convert to `StringRef` in a single step, so a single overload
stays unambiguous and still gives transparent (heterogeneous) lookup.
*/
struct StringRefHash {
    using is_transparent = void;

    size_t operator()(StringRef s) const {
        // use llvm's xxhash for now
        return llvm::xxh3_64bits(llvm::StringRef(s));
    }
};

struct StringRefEq {
    using is_transparent = void;

    bool operator()(StringRef a, StringRef b) const { return a == b; }
};

using StringHashSet = boost::unordered_set<StringRef, StringRefHash, StringRefEq>;

/**
Intern `s`: copy it once into an `ArenaStr` owned by a global pool and return a
`StringRef` to those bytes. Equal strings share one copy, so repeated calls
return the same pointer.

The pool lives as long as the global arena. `intern_reset()` empties it and must
run whenever `alloc::reset()` runs.
*/
[[nodiscard]] StringRef intern_string(StringRef s);

/** Intern the concatenation of `a` and `b`. */
[[nodiscard]] StringRef intern_concat(StringRef a, StringRef b);

/** Drop every interned string. Call alongside `alloc::reset()`. */
void intern_reset();

} // end namespace ecc

/** Format `StringRef` the same way as `std::string_view`. */
template <>
struct std::formatter<ecc::StringRef> : std::formatter<std::string_view> {
    auto format(ecc::StringRef s, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(std::string_view(s), ctx);
    }
};

#endif