#pragma once

#ifndef ECC_ERROR_H
#define ECC_ERROR_H

#include <exception>
#include <sstream>
#include <string>

#include "location.hpp"
#include "util.hpp"

using namespace ecc::util;
using namespace ecc::location;
namespace ecc {

enum class ErrorSource : uint8_t {
    NONE, // bodge for now
    PREPROC,
    PARSE,
    SEMANTIC,
    LOWER,
    LLVM,
};

/**
The main error class for Ecc.

The error reporting model for Ecc revolves around a two-tiered message system.
Errors return a toplevel error message, and an optional elaboration, as well as
an optional location for the elaboration.
*/
class EccError : public std::exception {
public:
    EccError(ErrorSource src, std::string err, Location loc)
        : src(src), msg(std::move(err)), loc(loc) {}

    EccError(ErrorSource src, std::string err) : src(src), msg(std::move(err)) {}

    EccError(std::string err, Location loc) : msg(std::move(err)), loc(loc) {}

    EccError(std::string err) : msg(std::move(err)) {}

    ErrorSource src = ErrorSource::NONE;
    std::string msg;
    Optional<Location> loc;

    void add_loc(Location loc) {
        if (!this->loc) {
            this->loc = loc;
        }
    }

    const char *what() const throw() override { return msg.c_str(); }

    virtual std::string elab() { return ""; }

    virtual Optional<Location> elab_loc() { return {}; }

    /**
    A virtual function to concatenate both the main message and the elaboration.

    Error reporting will directly use elab(), and elab_loc() in the future to
    do proper error reporting with carets and colours.
    */
    virtual std::string to_string() {

        std::stringstream ss;
        if (loc) {
            ss << "error <" << *loc << ">: ";
        } else {
            ss << "error: ";
        }

        ss << msg;

        auto elaboration = this->elab();
        if (!elaboration.empty()) {
            ss << "\n" << elaboration;
        }

        return ss.str();
    }
};

class UnableToContinue : public EccError {
public:
    UnableToContinue() : EccError("unable to continue") {}

    std::string to_string() override { return ""; }
};

class EccSemError : public EccError {
public:
    EccSemError(std::string msg) : EccError(ErrorSource::SEMANTIC, std::move(msg)) {}
    EccSemError(std::string msg, Location err_loc)
        : EccError(ErrorSource::SEMANTIC, std::move(msg), err_loc) {}
};

} // namespace ecc

#endif