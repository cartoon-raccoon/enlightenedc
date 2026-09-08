#pragma once

#ifndef ECC_UTIL_ASSERT_H
#define ECC_UTIL_ASSERT_H

#include <exception>
#include <source_location>
#include <sstream>
#include <string>
#include <string_view>

namespace ecc::util {

class InternalError : public std::exception {
public:
    InternalError(std::string_view msg, std::source_location at) {
        std::stringstream ss;

        ss << "internal compiler error at " 
           << at.file_name() << "(" << at.line() << ":" << at.column() << ") "
           << "in function " << at.function_name() << ":"
           << msg;
        
        full = ss.str();
    }

    const char *what() const noexcept override {return full.c_str();}

private:
    std::string full;
};

[[noreturn]] void
ice_fail(std::string_view msg, std::source_location at = std::source_location::current());

} // end namespace ecc::util

#define ECC_ASSERT(cond, msg) \
    (static_cast<bool>(cond) ? void(0) : ::ecc::util::ice_fail(msg))

#define ECC_ASSERT_N(cond) \
    (static_cast<bool>(cond) ? void(0) : ::ecc::util::ice_fail("assertion failed"))

#define ECC_UNREACHABLE(msg) \
    ::ecc::util::ice_fail(msg)

#endif