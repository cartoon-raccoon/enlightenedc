#pragma once

#ifndef ECC_ERROR_H
#define ECC_ERROR_H

#include <exception>
#include <sstream>
#include <string>

#include "location.hpp"
#include "prelude.hpp"

using namespace ecc::util;
using namespace ecc::location;
namespace ecc {

/**
The abstract class representing a generic diagnostic issued by Ecc.
*/
class EccDiagnostic {
public:
    EccDiagnostic(std::string msg) : msg(std::move(msg)) {}
    EccDiagnostic(std::string msg, Location loc) : msg(std::move(msg)), loc(loc) {}

    virtual ~EccDiagnostic() = default;

    std::string msg;
    Optional<Location> loc;

    void add_loc(Location loc) {
        if (!this->loc) {
            this->loc = loc;
        }
    }

    virtual std::string to_string() = 0;
};

/**
The main warning class for Ecc.
*/
class EccWarning : public EccDiagnostic {
public:
    EccWarning(std::string msg) : EccDiagnostic(std::move(msg)) {}

    EccWarning(std::string msg, Location loc) : EccDiagnostic(std::move(msg), loc) {}

    std::string to_string() override {
        std::stringstream ss;

        if (loc) {
            ss << "warning <" << *loc << ">: ";
        } else {
            ss << "warning: ";
        }

        ss << msg;

        return ss.str();
    }
};

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
class EccError : public EccDiagnostic, public std::exception {
public:
    EccError(ErrorSource src, std::string err, Location loc)
        : EccDiagnostic(std::move(err), loc), src(src) {}

    EccError(ErrorSource src, std::string err)
        : EccDiagnostic(std::move(err)), src(src) {}

    EccError(std::string err, Location loc)
        : EccDiagnostic(std::move(err), loc) {}

    EccError(std::string err)
        : EccDiagnostic(std::move(err)) {}

    ErrorSource src = ErrorSource::NONE;

    const char *what() const throw() override { return msg.c_str(); }

    virtual std::string elab() { return ""; }

    virtual Optional<Location> elab_loc() { return {}; }

    /**
    A virtual function to concatenate both the main message and the elaboration.

    Error reporting will directly use elab(), and elab_loc() in the future to
    do proper error reporting with carets and colours.
    */
    std::string to_string() override {

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