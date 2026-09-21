#include "prelude.hpp"

#include <cctype>
#include <cstdlib>

namespace ecc::util {

namespace {
// "\xHH" plus the null terminator written by snprintf.
constexpr std::size_t HEX_ESCAPE_BUF_SIZE = 5;
} // namespace

std::string encode_string_literal(StringRef raw) {
    std::string out;
    out.reserve(raw.size());

    for (unsigned char c : raw) {
        switch (c) {
        case '\\':
            out += "\\\\";
            break;
        case '"':
            out += "\\\"";
            break;
        case '\'':
            out += "\\'";
            break;
        case '\a':
            out += "\\a";
            break;
        case '\b':
            out += "\\b";
            break;
        case '\f':
            out += "\\f";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        case '\v':
            out += "\\v";
            break;
        default:
            if (std::isprint(c)) {
                out.push_back(static_cast<char>(c));
            } else {
                char buf[HEX_ESCAPE_BUF_SIZE];
                std::snprintf(buf, sizeof(buf), "\\x%02X", c);
                out += buf;
            }
        }
    }

    return out;
}

} // namespace ecc::util

namespace ecc {

namespace {

/**
The global string pool. Node-based, so an `ArenaStr`'s address (and its SSO
buffer) stays fixed for as long as it is in the set, which is what lets a
`StringRef` point into it.
*/
using InternPool = boost::unordered_set<util::ArenaStr, StringRefHash, StringRefEq>;

InternPool& intern_pool() {
    static InternPool pool;
    return pool;
}

} // namespace

StringRef intern_string(StringRef s) {
    InternPool& pool = intern_pool();

    if (auto it = pool.find(s); it != pool.end()) {
        return StringRef(*it);
    }

    auto [it, _] = pool.emplace(s.begin(), s.end());
    return StringRef(*it);
}

StringRef intern_concat(StringRef a, StringRef b) {
    util::ArenaStr joined;
    joined.reserve(a.size() + b.size());
    joined.append(a.begin(), a.end());
    joined.append(b.begin(), b.end());
    return intern_string(joined);
}

void intern_reset() { intern_pool().clear(); }

} // namespace ecc

