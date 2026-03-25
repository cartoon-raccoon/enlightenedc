#ifndef ECC_ERROR_H
#define ECC_ERROR_H

#include <exception>
#include <string>
#include <optional>
#include <sstream>

#include "util.hpp"

using namespace ecc::util;
namespace ecc {

enum class ErrorSource {
    NONE, // bodge for now
    PREPROC,
    PARSE,
    SEMANTIC,
    LOWER,
    LLVM,
};

class EccError : public std::exception {
public:
    EccError(ErrorSource src, std::string err, Location loc)
        : src(src) ,msg(err), loc(loc) {}

    EccError(ErrorSource src, std::string err)
        : src(src) ,msg(err) {}

    EccError(std::string err, Location loc)
        : msg(err), loc(loc) {}

    EccError(std::string err) : msg(err) {}

    ErrorSource src = ErrorSource::NONE;
    std::string msg;
    Optional<Location> loc;

    const char *what() const throw() override {
        return msg.c_str();
    }

    virtual std::string to_string() {
        
        std::stringstream ss;
        if (loc) {
            ss << "error <" << *loc << ">: ";
        } else {
            ss << "error: ";
        }

        ss << msg;

        return ss.str();
    }
};

}

#endif