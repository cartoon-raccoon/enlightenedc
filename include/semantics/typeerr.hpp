#pragma once

#ifndef ECC_TYPE_ERR_H
#define ECC_TYPE_ERR_H

#include <sstream>

#include "error.hpp"
#include "util.hpp"

namespace ecc::sema {

using namespace ecc;

class TypeSemError : public EccSemError {
public:
    TypeSemError(std::string msg, Location err_loc)
        : EccSemError(msg, err_loc) {}
};

class RecursiveTypeError : public TypeSemError {
public:
    RecursiveTypeError(std::string name, Location err_loc)
        : TypeSemError("recursive class member", err_loc), name(name) {}

    std::string name;

    std::string to_string() override {
        std::stringstream ss;
        ss << EccSemError::to_string() << ": " << name
           << "self-referential class members must be behind a pointer\n";

        return ss.str();
    }
};

class IncompleteTypeUseError : public TypeSemError {
public:
    IncompleteTypeUseError(std::string name, Location err_loc)
        : TypeSemError("use of incomplete type", err_loc), name(name) {}

    std::string name;
};

class InvalidVoidError : public TypeSemError {
public:
    InvalidVoidError(Location err_loc)
        : TypeSemError("invalid void", err_loc) {}
};

class UnsizedArrInUserTypeError : public TypeSemError {
public:
    UnsizedArrInUserTypeError(Location err_loc)
        : TypeSemError("unsized array in class or union", err_loc) {}

    // array members of classes or unions must have declared sizes
};

class InvalidReturnTypeError : public TypeSemError {
public:
    InvalidReturnTypeError(Location err_loc)
        : TypeSemError("invalid return type for function", err_loc) {}

    // functions cannot return arrays or other functions
};

class EnumeratorAlrDecldError : public TypeSemError {
public:
    EnumeratorAlrDecldError(std::string name, Location err_loc, Location def_loc)
    : TypeSemError("enumerator name conflict", err_loc)
    {
        std::stringstream ss;
        ss << EccError::what() << "\n" << "enumerator with name \'" << name << "\'" 
        << " previously defined at " << def_loc;

        msg = ss.str();
    }
};

class InvalidEnumUnderlyingError : public TypeSemError {
public:
    InvalidEnumUnderlyingError(Location err_loc)
    : TypeSemError("invalid enum underlying type", err_loc)
    {
        std::stringstream ss;
        ss << EccError::what() << "\n" << "underlying type of an enum must be an integer";

        msg = ss.str();
    }
};

class TypeDecldAsOtherError : public TypeSemError {
public:
    TypeDecldAsOtherError(std::string err, Location err_loc, Location def_loc)
    : TypeSemError(err, err_loc)
    {
        std::stringstream ss;
        ss << EccError::what() << "\n" << "type previously declared at " << def_loc;

        msg = ss.str();
    }
};

}

#endif