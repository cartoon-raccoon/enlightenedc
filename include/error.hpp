#pragma once

#ifndef ECC_ERROR_H
#define ECC_ERROR_H

#include <exception>
#include <sstream>
#include <string>

#include "util.hpp"

using namespace ecc::util;
namespace ecc {

enum class ErrorSource : uint8_t {
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

    std::string to_string() override {
        std::stringstream ss;
        ss << "error <" << *loc << ">: " << msg;
        return ss.str();
    }
};

} // namespace ecc

#endif