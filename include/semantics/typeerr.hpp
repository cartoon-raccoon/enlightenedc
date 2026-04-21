#pragma once

#include "semantics/types.hpp"
#ifndef ECC_TYPE_ERR_H
#define ECC_TYPE_ERR_H

#include <sstream>

#include "error.hpp"
#include "util.hpp"

namespace ecc::sema {

using namespace ecc;

class TypeSemError : public EccSemError {
public:
    TypeSemError(std::string msg, Location err_loc) : EccSemError(std::move(msg), err_loc) {}
};

class RecursiveTypeError : public TypeSemError {
public:
    RecursiveTypeError(std::string name, Location err_loc)
        : TypeSemError(std::format("recursive class member: {}", name), err_loc) {}

    std::string elab() override {
        return "self-referential class members must be behind a pointer";
    }
};

class IncompleteTypeUseError : public TypeSemError {
public:
    IncompleteTypeUseError(std::string name, Location err_loc)
        : TypeSemError("use of incomplete type", err_loc), name(std::move(name)) {}

    std::string name;
};

class InvalidVoidError : public TypeSemError {
public:
    InvalidVoidError(Location err_loc) : TypeSemError("invalid void", err_loc) {}

    std::string elab() override { return "variables cannot be void"; }
};

class InvalidMemberError : public TypeSemError {
public:
    InvalidMemberError(types::ClassType *cls, std::string member, Location err_loc)
        : TypeSemError("invalid class member", err_loc), type(cls->formal()),
          member(std::move(member)) {}

    std::string type;
    std::string member;

    std::string elab() override {
        std::stringstream ss;
        ss << "no member named \'" << member << "\' in class " << type;

        return ss.str();
    }
};

class UnsizedArrInUserTypeError : public TypeSemError {
public:
    UnsizedArrInUserTypeError(Location err_loc)
        : TypeSemError("unsized array in class or union", err_loc) {}

    std::string elab() override {
        return "array members of classes or unions must have declared sizes";
    }
};

class InvalidReturnTypeError : public TypeSemError {
public:
    InvalidReturnTypeError(Location err_loc)
        : TypeSemError("invalid return type for function", err_loc) {}

    std::string elab() override { return "functions cannot return arrays or other functions"; }
};

class EnumeratorAlrDecldError : public TypeSemError {
public:
    EnumeratorAlrDecldError(std::string name, Location err_loc, Location def_loc)
        : TypeSemError("enumerator name conflict", err_loc), name(std::move(name)),
          def_loc(def_loc) {}

    std::string name;
    Location def_loc;

    std::string elab() override {
        std::stringstream ss;
        ss << "enumerator with name \'" << name << "\' previously defined at <" << def_loc << ">";

        return ss.str();
    }
};

class InvalidEnumUnderlyingError : public TypeSemError {
public:
    InvalidEnumUnderlyingError(Location err_loc)
        : TypeSemError("invalid enum underlying type", err_loc) {}

    std::string elab() override { return "underlying type of an enum must be an integer"; }
};

class TypeDecldAsOtherError : public TypeSemError {
public:
    TypeDecldAsOtherError(std::string err, Location err_loc, Location def_loc)
        : TypeSemError(std::move(err), err_loc), def_loc(def_loc) {}

    Location def_loc;

    std::string elab() override {
        std::stringstream ss;
        ss << "type previously declared at <" << def_loc << ">";

        return ss.str();
    }
};

} // namespace ecc::sema

#endif