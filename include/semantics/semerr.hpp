#pragma once

#ifndef ECC_SEMERR_H
#define ECC_SEMERR_H

#include <sstream>

#include "error.hpp"
#include "util.hpp"

namespace ecc::sema {
using namespace ecc;



class InvalidCaseError : public EccSemError {
public:
    InvalidCaseError(Location err_loc)
    : EccSemError("case not in switch", err_loc) {}
};

class InvalidBreakError : public EccSemError {
public:
    InvalidBreakError(Location err_loc)
    : EccSemError("break not in switch or loop", err_loc) {}
};

class InvalidContError : public EccSemError {
public:
    InvalidContError(Location err_loc)
        : EccSemError("continue not in loop", err_loc) {}
};

class InvalidCallExprError : public EccSemError {
public:
    InvalidCallExprError(std::string ident, Location err_loc)
        : EccSemError("invalid function call", err_loc), ident(std::move(ident)) {}

    std::string ident;
};

class InvalidInitializerError : public EccSemError {
public:
    InvalidInitializerError(std::string err, Location err_loc)
        : EccSemError("invalid initializer", err_loc), err(std::move(err)) {}

    std::string err;
};

class TypeNotDefinedError : public EccSemError {
public:
    TypeNotDefinedError(std::string name, Location err_loc)
        : EccSemError("use of undefined type", err_loc), name(std::move(name)) {}

    std::string name;

    std::string to_string() override {
        std::stringstream ss;
        ss << EccSemError::to_string() 
           << "type \'" << name << "\' is not declared\n";

        return ss.str();
    }
};

class IdentNotDefinedError : public EccSemError {
public:
    IdentNotDefinedError(std::string name, Location err_loc)
        : EccSemError("identifier not defined", err_loc), name(std::move(name)) {}

    std::string name;

    std::string to_string() override {
        std::stringstream ss;
        ss << EccError::what() 
           << "identifier \'" << name << "\' is not declared\n";

        return ss.str();
    }
};

class InvalidIdentifierError : public EccSemError {
public:
    InvalidIdentifierError(std::string name, Location err_loc)
        : EccSemError("invalid identifier", err_loc), name(std::move(name)) {}

    std::string name;

    std::string to_string() override {
        std::stringstream ss;
        ss << EccError::what() << "\n" 
           << "identifier \'" << name << "\' must reference a function or variable";

        return ss.str();
    }
};



class TypeAlrDefinedError : public EccSemError {
public:
    TypeAlrDefinedError(std::string err, Location err_loc, Location def_loc) 
        : EccSemError(std::move(err), err_loc), def_loc(def_loc) {}

    Location def_loc;

    std::string to_string() override {
        std::stringstream ss;
        ss << EccError::what() << "\n" << "type previously defined at " << def_loc;

        return ss.str();
    }
};

class SymbolAlrDecldError : public EccSemError {
public:
    SymbolAlrDecldError(std::string err, Location err_loc, Location def_loc)
        : EccSemError(std::move(err), err_loc), def_loc(def_loc) {}

    Location def_loc;

    std::string to_string() override {
        std::stringstream ss;
        ss << EccError::what() << "\n" << "symbol previously declared at " << def_loc;

        return ss.str();
    }
};

}

#endif