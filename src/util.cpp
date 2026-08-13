#include "util.hpp"

#include <cctype>
#include <cstdio>

namespace ecc::util {

namespace {
// "\xHH" plus the null terminator written by snprintf.
constexpr std::size_t HEX_ESCAPE_BUF_SIZE = 5;
} // namespace

std::string encode_string_literal(std::string_view raw) {
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
