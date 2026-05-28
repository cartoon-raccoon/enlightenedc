#pragma once

#ifndef ECC_TYPE_ERR_H
#define ECC_TYPE_ERR_H

#include <cstdint>
#include <sstream>

#include "error.hpp"
#include "location.hpp"
#include "semantics/types.hpp"

namespace ecc::sema {

using namespace ecc;
using namespace location;

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

    std::string type, member;

    std::string elab() override {
        std::stringstream ss;
        ss << "no member named \'" << member << "\' in class " << type;

        return ss.str();
    }
};

class MemberNameCollision : public TypeSemError {
public:
    MemberNameCollision(Location err_loc, std::string member_name, Location def_loc)
        : TypeSemError(std::format("member already exists: {}", member_name), err_loc),
          member_name(std::move(member_name)), def_loc(def_loc) {}

    std::string member_name;
    Location def_loc;

    std::string elab() override {
        std::stringstream ss;
        ss << "member previously defined at <" << def_loc << ">";

        return ss.str();
    }
};

/**
An error for when a UnionType contains a member that is larger than the type representative.
*/
class UnionTypeRepSizeOverflow : public TypeSemError {
public:
    UnionTypeRepSizeOverflow(
        Location err_loc, Location member_loc, types::Type *member_type, size_t max_size,
        size_t err_size)
        : TypeSemError("member larger than union type representative", err_loc),
          member_loc(member_loc), member_type(member_type->formal()), max_size(max_size),
          err_size(err_size) {}

    Location member_loc;
    std::string member_type;
    size_t max_size, err_size;

    std::string elab() override {
        std::stringstream ss;
        ss << "type representative has size " << max_size << ", found member of type " << member_type
           << " with size " << err_size;

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

class EnumeratorCountOverflow : public TypeSemError {
public:
    EnumeratorCountOverflow(Location err_loc, uint64_t max_ct, uint64_t enumerator_ct)
        : TypeSemError("more enumerators than maximum size of enum", err_loc), max_ct(max_ct),
          enumerator_ct(enumerator_ct) {}

    uint64_t max_ct, enumerator_ct;

    std::string elab() override {
        std::stringstream ss;

        ss << "enum can have maximum " << max_ct << " enumerators, but contains " << enumerator_ct;

        return ss.str();
    }
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