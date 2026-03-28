#pragma once

#ifndef ECC_SEMERR_H
#define ECC_SEMERR_H

#include <sstream>

#include "error.hpp"
#include "util.hpp"

namespace ecc::sema {
using namespace ecc;

class EccSemError : public EccError {
public:
    EccSemError(std::string msg, Location err_loc)
        : EccError(ErrorSource::SEMANTIC, msg, err_loc) {}

    virtual std::string to_string() override {
        std::stringstream ss;
        ss << "error <" << *loc << ">: " << msg << "\n";
        return ss.str();
    }
};

class TypeSemError : public EccSemError {
public:
    TypeSemError(std::string msg, Location err_loc)
        : EccSemError(msg, err_loc) {}
};

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
    : EccSemError("invalid function call", err_loc) {}
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

class TypeNotDefinedError : public EccSemError {
public:
    TypeNotDefinedError(std::string name, Location err_loc)
        : EccSemError("use of undefined type", err_loc), name(name) {}

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
    : EccSemError("identifier not defined", err_loc)
    {
        std::stringstream ss;
        ss << EccError::what() 
           << "identifier \'" << name << "\' is not declared\n";

        msg = ss.str();
    }
};

class InvalidIdentifierError : public EccSemError {
public:
    InvalidIdentifierError(std::string name, Location err_loc)
    : EccSemError("invalid identifier", err_loc)
    {
        std::stringstream ss;
        ss << EccError::what() << "\n" 
           << "identifier \'" << name << "\' must reference a function or variable";

        msg = ss.str();
    }
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

class TypeAlrDefinedError : public EccSemError {
public:
    TypeAlrDefinedError(std::string err, Location err_loc, Location def_loc) 
    : EccSemError(err, err_loc)
    {
        std::stringstream ss;
        ss << EccError::what() << "\n" << "type previously defined at " << def_loc;

        msg = ss.str();
    }
};

class SymbolAlrDecldError : public EccSemError {
public:
    SymbolAlrDecldError(std::string err, Location err_loc, Location def_loc)
    : EccSemError(err, err_loc)
    {
        std::stringstream ss;
        ss << EccError::what() << "\n" << "symbol previously declared at " << def_loc;

        msg = ss.str();
    }
};

}

#endif