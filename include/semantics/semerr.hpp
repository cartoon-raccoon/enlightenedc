#pragma once

#ifndef ECC_SEMERR_H
#define ECC_SEMERR_H

#include <sstream>

#include "error.hpp"
#include "util.hpp"
#include "semantics/types.hpp"

namespace ecc::sema {
using namespace ecc;

class InvalidCaseError : public EccSemError {
public:
    InvalidCaseError(Location err_loc) : EccSemError("case label not in switch", err_loc) {}
};

class InvalidBreakError : public EccSemError {
public:
    InvalidBreakError(Location err_loc) : EccSemError("break not in switch or loop", err_loc) {}
};

class InvalidContError : public EccSemError {
public:
    InvalidContError(Location err_loc) : EccSemError("continue not in loop", err_loc) {}
};

class InvalidReturnError : public EccSemError {
public:
    InvalidReturnError(Location err_loc) : EccSemError("return statement not in function", err_loc) {}
};

class InvalidCallExprError : public EccSemError {
public:
    InvalidCallExprError(types::Type *type, Location err_loc)
        : EccSemError("invalid call expression", err_loc), typestr(type->formal()) {}

    std::string typestr;

    std::string elab() override {
        std::stringstream ss;
        ss << typestr << " is not callable";

        return ss.str();
    }
};

class InvalidConditionError : public EccSemError {
public:
    InvalidConditionError(types::Type *type, Location err_loc)
        : EccSemError("invalid condition", err_loc), typestr(type->formal()) {}

    std::string typestr;

    std::string elab() override {
        std::stringstream ss;
        ss << typestr << " cannot be used as a condition";

        return ss.str();
    }
};

class InvalidCoerceError : public EccSemError {
public:
    InvalidCoerceError(types::Type *from, types::Type *to, Location err_loc)
        : EccSemError("invalid implicit cast", err_loc), 
        from(from->formal()), to(to->formal()) {}

    std::string from, to;

    std::string elab() override {
        std::stringstream ss;
        ss << "cannot implicitly cast " << from << " to " << to;

        return ss.str();
    }
};

class InvalidInitializerError : public EccSemError {
public:
    InvalidInitializerError(std::string err, Location err_loc)
        : EccSemError("invalid initializer", err_loc), err(std::move(err)) {}

    std::string err;

    std::string elab() override {
        return err;
    }
};

class InvalidTypeError : public EccSemError {
public:
    InvalidTypeError(std::string err, types::Type *type, Location err_loc)
        : EccSemError("invalid type: " + type->formal(), err_loc), err(std::move(err)) {}

    std::string err;

    std::string elab() override {
        return err;
    }
};

class TypeNotDefinedError : public EccSemError {
public:
    TypeNotDefinedError(std::string name, Location err_loc)
        : EccSemError("use of undefined type", err_loc), name(std::move(name)) {}

    std::string name;

    std::string elab() override {
        std::stringstream ss;
        ss << "type \'" << name << "\' is not declared\n";

        return ss.str();
    }
};

class IdentNotDefinedError : public EccSemError {
public:
    IdentNotDefinedError(std::string name, Location err_loc)
        : EccSemError("identifier not defined", err_loc), name(std::move(name)) {}

    std::string name;

    std::string elab() override {
        std::stringstream ss;
        ss << "identifier \'" << name << "\' is not declared\n";

        return ss.str();
    }
};

class InvalidIdentifierError : public EccSemError {
public:
    InvalidIdentifierError(std::string name, Location err_loc)
        : EccSemError("invalid identifier", err_loc), name(std::move(name)) {}

    std::string name;

    std::string elab() override {
        std::stringstream ss;
        ss << "identifier \'" << name
           << "\' must reference a function or variable";

        return ss.str();
    }
};

class TypeAlrDefinedError : public EccSemError {
public:
    TypeAlrDefinedError(std::string err, Location err_loc, Location def_loc)
        : EccSemError(std::move(err), err_loc), def_loc(def_loc) {}

    Location def_loc;

    std::string elab() override {
        std::stringstream ss;
        ss << "type previously defined at <" << def_loc << ">";

        return ss.str();
    }
};

class SymbolAlrDecldError : public EccSemError {
public:
    SymbolAlrDecldError(std::string err, Location err_loc, Location def_loc)
        : EccSemError(std::move(err), err_loc), def_loc(def_loc) {}

    Location def_loc;

    std::string elab() override {
        std::stringstream ss;
        ss << "symbol previously declared at <" << def_loc << ">";

        return ss.str();
    }
};

} // namespace ecc::sema

#endif