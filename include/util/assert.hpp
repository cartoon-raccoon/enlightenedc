#pragma once

#ifndef ECC_UTIL_ASSERT_H
#define ECC_UTIL_ASSERT_H

#include <exception>
#include <iostream> // fixme: this pulls iostream in a widely used header
#include <source_location>
#include <sstream>
#include <string>
#include <string_view>
#include <filesystem>

namespace ecc::util {

using path = std::filesystem::path;

class InternalError : public std::exception {
public:
    InternalError(std::string_view msg, std::source_location at) {
        std::stringstream ss;

        path filepath = at.file_name();

        ss << "internal compiler error at " 
           << filepath.filename() << "(" << at.line() << ":" << at.column() << ") "
           << "in function " << at.function_name() << ":"
           << msg;
        
        full = ss.str();
    }

    const char *what() const noexcept override {return full.c_str();}

private:
    std::string full;
};

[[noreturn]] inline void
ice_fail(std::string_view msg, std::source_location at = std::source_location::current()) {

#ifndef NDEBUG
    if (std::getenv("ECC_ABORT_ON_ICE") != nullptr) {
        std::cerr << InternalError(msg, at).what() << "\n";
        std::abort();
    }
#endif
    throw InternalError(msg, at);
}

} // end namespace ecc::util

#define ECC_ASSERT(cond, msg) \
    (static_cast<bool>(cond) ? void(0) : ::ecc::util::ice_fail(msg))

#define ECC_ASSERT_N(cond) \
    (static_cast<bool>(cond) ? void(0) : ::ecc::util::ice_fail("assertion failed"))

#define ECC_UNREACHABLE(msg) \
    ::ecc::util::ice_fail(msg)

#endif