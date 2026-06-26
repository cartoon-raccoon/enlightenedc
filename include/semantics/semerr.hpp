#pragma once

#ifndef ECC_SEMERR_H
#define ECC_SEMERR_H

#include <sstream>

#include "error.hpp"
#include "eval/value.hpp"
#include "location.hpp"
#include "semantics/types.hpp"
#include "tokens.hpp"

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
    enum class Kind : uint8_t {
        NotInFunction,
        RetValueFromVoid,
    };
    InvalidReturnError(Kind kind, Location err_loc)
        : EccSemError("invalid return statement", err_loc), kind(kind) {}

    Kind kind;

    std::string elab() override {
        switch (kind) {
        case Kind::NotInFunction:
            return "return statement not in function";
        case Kind::RetValueFromVoid:
            return "returning a value in a void function";
        }
    }
};

class TooManyArgsError : public EccSemError {
public:
    TooManyArgsError(Location err_loc, size_t expected, size_t got)
        : EccSemError("too many arguments to call expression", err_loc), expected(expected),
          got(got) {}

    size_t expected, got;

    std::string elab() override {
        std::stringstream ss;
        ss << "expected " << expected << ", got " << got;

        return ss.str();
    }
};

class InvalidSwitchCtrlError : public EccSemError {
public:
    InvalidSwitchCtrlError(types::Type *ctrl_type, Location err_loc)
        : EccSemError("invalid switch control expression", err_loc),
          ctrl_type(ctrl_type->formal()) {}

    std::string ctrl_type;

    std::string elab() override {
        std::stringstream ss;
        ss << "switch statement control expression cannot be `" << ctrl_type
           << "`; must be an integer or enum";

        return ss.str();
    }
};

class InvertedCaseRangeError : public EccSemError {
public:
    InvertedCaseRangeError(const eval::Value& start, const eval::Value& end, Location err_loc)
        : EccSemError("invalid case range", err_loc), start(start), end(end) {}

    eval::Value start, end;

    std::string elab() override {
        return "start value used in a case range label must be lesser than the end value";
    }
};

class InvalidCaseRangeError : public EccSemError {
public:
    InvalidCaseRangeError(const eval::Value& start, const eval::Value& end, Location err_loc)
        : EccSemError("invalid case range", err_loc), start(start), end(end) {}

    eval::Value start, end;

    std::string elab() override { return "values used in a case range label must be integers"; }
};

class InvalidCaseValueError : public EccSemError {
public:
    InvalidCaseValueError(const eval::Value& val, Location err_loc)
        : EccSemError("invalid case value", err_loc), invalid_val(val) {}

    eval::Value invalid_val;

    std::string elab() override { return "values used in a case label must be integers"; }
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

class InvalidBinaryOpError : public EccSemError {
public:
    InvalidBinaryOpError(
        std::string err, tokens::BinaryOp op, types::Type *lhs, types::Type *rhs, Location err_loc)
        : EccSemError(std::format("invalid binary operator: {}", err), err_loc), op(op),
          lhsstr(lhs->formal()), rhsstr(rhs->formal()) {}

    tokens::BinaryOp op;
    std::string lhsstr, rhsstr;

    std::string elab() override {
        std::stringstream ss;
        ss << "operator \'" << tokens::binop_to_string(op) << "\' cannot be applied to types "
           << lhsstr << " and " << rhsstr;

        return ss.str();
    }
};

class InvalidPointerArithmetic : public EccSemError {
public:
    enum class Kind : uint8_t {
        InvalidOperator,
        InvalidPrimOperand,
        IncompatiblePtrOperands,
    };

    InvalidPointerArithmetic(Kind kind, Location err_loc)
        : EccSemError("invalid pointer arithmetic", err_loc), kind(kind) {}

    Kind kind;

    std::string elab() override {
        switch (kind) {
        case Kind::InvalidOperator:
            return "pointer-to-pointer operations can only be subtraction";
        case Kind::InvalidPrimOperand:
            return "invalid primitive operand, operand must be an integer";
        case Kind::IncompatiblePtrOperands:
            return "pointer operands are incompatible";
        }
    }
};

class InvalidUnaryOpError : public EccSemError {
public:
    InvalidUnaryOpError(std::string err, tokens::UnaryOp op, types::Type *operand, Location err_loc)
        : EccSemError(std::format("invalid unary operator: {}", err), err_loc), op(op),
          operandstr(operand->formal()) {}

    tokens::UnaryOp op;
    std::string operandstr;

    std::string elab() override {
        std::stringstream ss;
        ss << "operator \'" << tokens::unop_to_string(op) << "\' cannot be applied to type "
           << operandstr;

        return ss.str();
    }
};

class InvalidSubscrExprError : public EccSemError {
public:
    InvalidSubscrExprError(std::string err, types::Type *type, Location err_loc)
        : EccSemError("invalid subscript expression", err_loc), err(std::move(err)),
          typestr(type->formal()) {}

    std::string err, typestr;

    std::string elab() override {
        std::stringstream ss;
        ss << err << ": " << typestr;
        return ss.str();
    }
};

class InvalidReintExprError : public EccSemError {
public:
    enum class Kind : uint8_t {
        ObjIsNotPtr,
        ObjIsNotPrim,
        TargetSizeOverflow,
    };

    InvalidReintExprError(Kind kind, Location err_loc)
        : EccSemError("invalid reinterpret expression", err_loc), kind(kind) {}

    Kind kind;

    std::string elab() override {
        switch (kind) {
        case Kind::ObjIsNotPtr:
            return "object of an arrow operator must be a pointer";
        case Kind::ObjIsNotPrim:
            return "object of a reinterpret expression must be a primitive";
        case Kind::TargetSizeOverflow:
            return "target of a reinterpret expression must be smaller than its object";
        }
    }
};

class InvalidPostfixExprError : public EccSemError {
public:
    InvalidPostfixExprError(
        std::string err, tokens::PostfixOp op, types::Type *type, Location err_loc)
        : EccSemError(std::format("invalid postfix expression: {}", err), err_loc), op(op),
          typestr(type->formal()) {}

    tokens::PostfixOp op;
    std::string typestr;

    std::string elab() override {
        std::stringstream ss;
        ss << "operator \'" << tokens::postfixop_to_string(op) << "\' cannot be applied to type "
           << typestr;
        return ss.str();
    }
};

class InvalidCoerceError : public EccSemError {
public:
    InvalidCoerceError(types::Type *from, types::Type *to, Location err_loc)
        : EccSemError("invalid implicit cast", err_loc), from(from->formal()), to(to->formal()) {}

    std::string from, to;

    std::string elab() override {
        std::stringstream ss;
        ss << "cannot implicitly cast " << from << " to " << to;

        return ss.str();
    }
};

class InvalidMemberAccError : public EccSemError {
public:
    enum class Kind : uint8_t {
        IncompatibleObject,
        ObjectIsNotPtr,
    };

    InvalidMemberAccError(Kind kind, types::Type *obj, Location err_loc)
        : EccSemError("invalid member access on incompatible type", err_loc), kind(kind),
          obj_type(obj->formal()) {}

    Kind kind;
    std::string obj_type;

    std::string elab() override {
        switch (kind) {
        case Kind::IncompatibleObject:
            return std::format("type `{}` cannot be used with `.` or `->`", obj_type);
        case Kind::ObjectIsNotPtr:
            return "object of an arrow operator must be a pointer";
        }
    }
};

class NoSuchMemberError : public EccSemError {
public:
    NoSuchMemberError(std::string& member, types::Type *obj, Location err_loc)
        : EccSemError("invalid member access", err_loc), member(member), obj_type(obj->formal()) {}

    std::string member, obj_type;

    std::string elab() override {
        return std::format("no member `{}` on type {}", member, obj_type);
    }
};

class InvalidInitializerError : public EccSemError {
public:
    InvalidInitializerError(std::string err, Location err_loc)
        : EccSemError("invalid initializer", err_loc), err(std::move(err)) {}

    std::string err;

    std::string elab() override { return err; }
};

class InvalidTypeError : public EccSemError {
public:
    InvalidTypeError(std::string err, types::Type *type, Location err_loc)
        : EccSemError("invalid type: " + type->formal(), err_loc), err(std::move(err)) {}

    std::string err;

    std::string elab() override { return err; }
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
        ss << "identifier \'" << name << "\' must reference a function or variable";

        return ss.str();
    }
};

class InvalidDeclError : public EccSemError {
public:
    InvalidDeclError(std::string err, Location err_loc)
        : EccSemError("invalid declaration", err_loc), err(std::move(err)) {}

    std::string err;

    std::string elab() override { return err; }
};

class LabelNotDefinedError : public EccSemError {
public:
    LabelNotDefinedError(std::string name, Location err_loc)
        : EccSemError("use of undefined label", err_loc), name(std::move(name)) {}

    std::string name;

    std::string elab() override {
        std::stringstream ss;
        ss << "label \'" << name << "\' is not defined";
        return ss.str();
    }
};

class InvalidCastError : public EccSemError {
public:
    InvalidCastError(types::Type *from, types::Type *to, Location err_loc)
        : EccSemError("invalid cast", err_loc), from(from->formal()), to(to->formal()) {}

    std::string from, to;

    std::string elab() override {
        std::stringstream ss;
        ss << "cannot cast " << from << " to " << to;
        return ss.str();
    }
};

class InvalidAssignError : public EccSemError {
public:
    enum class Kind : uint8_t {
        CannotAssign,
        NonIntPrimitive,
        NotPrimitive,
    };

    InvalidAssignError(Kind kind, types::Type *expr_type, Location err_loc)
        : EccSemError("invalid assignment", err_loc), kind(kind), expr_type(expr_type->formal()) {}

    Kind kind;
    std::string expr_type;

    std::string elab() override {

        switch (kind) {
        case Kind::CannotAssign: {
            std::stringstream ss;
            ss << "cannot assign to expression of type " << expr_type;
            return ss.str();
        }
        case Kind::NonIntPrimitive:
            return "right operand for this operation must be an integer";
        case Kind::NotPrimitive:
            return "right operand must be a primitive";
        }
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

    Optional<Location> elab_loc() override { return def_loc; }
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

    Optional<Location> elab_loc() override { return def_loc; }
};

} // namespace ecc::sema

#endif