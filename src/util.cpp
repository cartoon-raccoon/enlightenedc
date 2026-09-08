#include "prelude.hpp"

#include <cctype>
#include <cstdlib>
#include <iostream>
#include <string_view>

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

[[noreturn]] void ice_fail(std::string_view msg, std::source_location at) {

#ifndef NDEBUG
    if (std::getenv("ECC_ABORT_ON_ICE") != nullptr) {
        std::cerr << InternalError(msg, at).what() << "\n";
        std::abort();
    }
#endif
    throw InternalError(msg, at);
}

} // namespace ecc::util

