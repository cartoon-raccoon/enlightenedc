#pragma once

#ifndef ECC_PRIMITIVES_H
#define ECC_PRIMITIVES_H

#include <cstdint>

#include "tokens.hpp"
#include "prelude.hpp"

using namespace ecc::util;

/**
\namespace ecc::sema::prim

Primitive type algebra functionality.

This namespace serves as source of truth for all operations involving primitive
types, how primitive types relate to each other, and how they relate with operations.

## Rank

Each primitive type is assigned a rank, that affects how it is
*/
namespace ecc::sema::prim {

using namespace ecc::tokens;

/**
\brief The rank of a primitive type.

Each primitive type in HolyC receives a rank. This rank plays a type in
type promotion (see `pr_promote`). The rank increases with size, except
with floats at the very top.
*/
enum class PrimTypeRank : uint8_t {
    BOOL  = 1,
    INT8  = 2,
    INT16 = 3,
    INT32 = 4,
    INT64 = 5,
    FLT32 = 6,
    FLT64 = 7,
};

/**
\brief Check if an unary operator can be used in a ConstExpression.

The unary operators `++` and `--`, as well as `&` and `*`, cannot be
constant-folded, because they work on lvalues (i.e. memory locations).
Since ConstExpressions only work with immediate values, they are invalid
in expressions that need to be constant folded.

\return Whether the operator can be constant-folded.
*/
[[nodiscard]] constexpr bool unaryop_is_const_foldable(UnaryOp op) {
    switch (op) {
    case UnaryOp::INC:
    case UnaryOp::DEC:
    case UnaryOp::REF:
    case UnaryOp::DEREF:
        return false;

    case UnaryOp::POS:
    case UnaryOp::NEG:
    case UnaryOp::TILDE:
    case UnaryOp::NOT:
        return true;
    }
}

[[nodiscard]] constexpr PrimTypeRank pr_rank(PrimType pr) {
    using P  = PrimType;
    using PR = PrimTypeRank;

    switch (pr) {
    case P::BOOL:
        return PR::BOOL;
    case P::U8:
    case P::I8:
        return PR::INT8;
    case P::U16:
    case P::I16:
        return PR::INT16;
    case P::U32:
    case P::I32:
        return PR::INT32;
    case P::U64:
    case P::I64:
        return PR::INT64;
    case P::F32:
        return PR::FLT32;
    case P::F64:
        return PR::FLT64;
    }
}

[[nodiscard]] constexpr bool pr_is_integer(PrimType pr) {
    // We maintain strict integral definitions for determining whether
    // this type is an integer: it cannot be Bool or Float, and it has to be sized.
    switch (pr) {
    case PrimType::U8:
    case PrimType::U16:
    case PrimType::U32:
    case PrimType::U64:
    case PrimType::I8:
    case PrimType::I16:
    case PrimType::I32:
    case PrimType::I64:
        return true;

    case PrimType::F32:
    case PrimType::F64:
    case PrimType::BOOL:
        return false;
    }
}

[[nodiscard]] constexpr bool pr_is_float(PrimType pr) {
    return pr == PrimType::F64 || pr == PrimType::F32;
}

[[nodiscard]] constexpr bool pr_is_bool(PrimType pr) {
    return pr == PrimType::BOOL;
}

[[nodiscard]] constexpr bool pr_is_signed(PrimType pr) {
    switch (pr) {
    case PrimType::U8:
    case PrimType::U16:
    case PrimType::U32:
    case PrimType::U64:
    case PrimType::BOOL:
        return false;

    case PrimType::I8:
    case PrimType::I16:
    case PrimType::I32:
    case PrimType::I64:
    case PrimType::F32:
    case PrimType::F64:
        return true;
    }
}

constexpr size_t ONE_BYTE    = 1;
constexpr size_t TWO_BYTES   = 2;
constexpr size_t FOUR_BYTES  = 4;
constexpr size_t EIGHT_BYTES = 8;

constexpr size_t BYTE_WIDTH = 8;

/**
Gets the size of the primitive type in bytes.
*/
[[nodiscard]] constexpr size_t pr_size(PrimType pr) {
    switch (pr) {
    case PrimType::BOOL:
    case PrimType::U8:
    case PrimType::I8:
        return ONE_BYTE;
    case PrimType::U16:
    case PrimType::I16:
        return TWO_BYTES;
    case PrimType::U32:
    case PrimType::I32:
    case PrimType::F32:
        return FOUR_BYTES;
    case PrimType::U64:
    case PrimType::I64:
    case PrimType::F64:
        return EIGHT_BYTES;
    }
}

/**
Gets the size of the primitive type in bits.
*/
[[nodiscard]] constexpr size_t pr_size_in_bits(PrimType pr) {
    return pr_size(pr) * BYTE_WIDTH;
}

[[nodiscard]] constexpr PrimType pr_from_rank(PrimTypeRank rank, bool is_signed) {
    using P  = PrimType;
    using PR = PrimTypeRank;

    switch (rank) {
    case PR::BOOL:
        return P::BOOL;
    case PR::INT8:
        return is_signed ? P::I8 : P::U8;
    case PR::INT16:
        return is_signed ? P::I16 : P::U16;
    case PR::INT32:
        return is_signed ? P::I32 : P::U32;
    case PR::INT64:
        return is_signed ? P::I64 : P::U64;
    case PR::FLT32:
        return P::F32;
    case PR::FLT64:
        return P::F64;
    }
}

/**
Get the unsigned counterpart of `pr`.

This is identity for all types that don't have an unsigned counterpart,
namely Bool, F32, and F64.
*/
[[nodiscard]] constexpr PrimType pr_unsigned(PrimType pr) {
    using P = PrimType;

    switch (pr) {
    case P::BOOL:
        return P::BOOL;
    case P::U8:
    case P::I8:
        return P::U8;
    case P::U16:
    case P::I16:
        return P::U16;
    case P::U32:
    case P::I32:
        return P::U32;
    case P::U64:
    case P::I64:
        return P::U64;
    case P::F32:
        return P::F32;
    case P::F64:
        return P::F64;
    }
}

/**
Promote the LHS and RHS of some binary expression to suitable types.

For binary expressions where the left and right hand sides are not
the same type, we need some deterministic way to get them to the same
type. We do this with type promotion.

## Invariants
- Type commutativity `promote(I16, U32) = promote(U32, I16)`.
- Unsigned wins if both types differ in signedness, that is
  `promote(U32, I32) = U32`.
- Both types are promoted to at least I32, that is
  `promote(I16, I8) = I32`.
*/
[[nodiscard]] constexpr PrimType pr_promote(PrimType lhs, PrimType rhs) {
    PrimTypeRank lhs_rank = pr_rank(lhs);
    PrimTypeRank rhs_rank = pr_rank(rhs);
    bool lhs_signed       = pr_is_signed(lhs);
    bool rhs_signed       = pr_is_signed(rhs);

    // Use signed only if both are signed.
    bool use_signed = lhs_signed && rhs_signed;

    // Take the higher of the two ranks, tie-breaking to the left
    if (lhs_rank >= rhs_rank) {
        PrimType ret =
            lhs_rank >= PrimTypeRank::INT32
                // if lhs_rank is already that of a 4-byte integer or higher, use it
                ? (use_signed ? lhs : pr_unsigned(lhs))
                // otherwise, promote it to a 4-byte integer with precalculated signedness
                : pr_from_rank(PrimTypeRank::INT32, use_signed);

        return ret;
    } else {
        PrimType ret = rhs_rank >= PrimTypeRank::INT32
                           ? (use_signed ? rhs : pr_unsigned(rhs))
                           : pr_from_rank(PrimTypeRank::INT32, use_signed);

        return ret;
    }
}

/**
Promote a single type to a minimum type in an expression.

This just applies a floor of I32 rank, preserving signedness, hence it
is identity over any PrimitiveType of where rank >= INT32.
*/
[[nodiscard]] constexpr PrimType pr_single_promote(PrimType pr) {
    if (pr_rank(pr) < PrimTypeRank::INT32) {
        return pr_from_rank(PrimTypeRank::INT32, pr_is_signed((pr)));
    } else {
        return pr;
    }
}

/**
A struct describing the resulting types involved in a binary expression.

This struct contains the types that the operands will be promoted to, and
the final type of the expression.
*/
struct PrimExprTypes {
    Pair<PrimType, PrimType> operand_types;
    PrimType expr_type;
};

/**
Check PrimitiveType compatibility with the given operation `op`.
*/
[[nodiscard]] constexpr Optional<PrimExprTypes> pr_check_binary_op(BinaryOp op, PrimType lhs, PrimType rhs) {

    switch (op) {
    case BinaryOp::LSHIFT:
    case BinaryOp::RSHIFT: {
        // exception to promotion: if operation is a bitshift, we only promote the lhs
        lhs = pr_single_promote(lhs);
        if (pr_is_integer(lhs) && pr_is_integer(rhs)) {
            return PrimExprTypes{
                {lhs, rhs},
                lhs
            };
        }
        break;
    }
    case BinaryOp::PLUS:
    case BinaryOp::MINUS:
    case BinaryOp::MUL:
    case BinaryOp::DIV: {
        auto promoted = pr_promote(lhs, rhs);
        lhs           = promoted;
        rhs           = promoted;
        if ((pr_is_integer(lhs) || pr_is_float(lhs)) && (pr_is_integer(rhs) || pr_is_float(rhs))) {
            return PrimExprTypes{
                {lhs, rhs},
                lhs
            };
        }

        break;
    }
    case BinaryOp::AND:
    case BinaryOp::OR:
    case BinaryOp::XOR:
    case BinaryOp::MOD: {
        auto promoted = pr_promote(lhs, rhs);
        lhs           = promoted;
        rhs           = promoted;
        if (pr_is_integer(lhs) && pr_is_integer(rhs)) {
            return PrimExprTypes{
                {lhs, rhs},
                lhs
            };
        }

        break;
    }
    case BinaryOp::EQ:
    case BinaryOp::NE: {
        auto promoted = pr_promote(lhs, rhs);
        lhs           = promoted;
        rhs           = promoted;
        if ((pr_is_integer(lhs) || pr_is_float(lhs) || pr_is_bool(lhs)) &&
            (pr_is_integer(rhs) || pr_is_float(rhs) || pr_is_bool(rhs))) {
            return PrimExprTypes{
                {lhs, rhs},
                PrimType::BOOL,
            };
        }
        break;
    }
    case BinaryOp::ANDAND:
    case BinaryOp::OROR:
    case BinaryOp::LT:
    case BinaryOp::GT:
    case BinaryOp::LE:
    case BinaryOp::GE: {
        auto promoted = pr_promote(lhs, rhs);
        lhs           = promoted;
        rhs           = promoted;
        if ((pr_is_integer(lhs) || pr_is_float(lhs)) && (pr_is_integer(rhs) || pr_is_float(rhs))) {
            return PrimExprTypes{
                {lhs, rhs},
                PrimType::BOOL,
            };
        }
        break;
    }
    default:
        // for any operators we don't explicitly check, just return true and let the codegen handle
        // it. OROR and ANDAND implicitly convert their operands to bool, which all primitive types
        // can do. BINCOMMA is used to sequencing operations, and does not require any specific
        // type.
        auto promoted = pr_promote(lhs, rhs);
        lhs           = promoted;
        rhs           = promoted;
        return PrimExprTypes{
            {lhs, rhs},
            lhs
        };
    }

    return {};
}

/**
Returns the corresponding BinaryOp for `op`.
*/
[[nodiscard]] constexpr Optional<BinaryOp> pr_assignop_to_binop(AssignOp op) {
    switch (op) {
    case AssignOp::ASSIGN:
        return {};
    case AssignOp::MULEQ:
        return BinaryOp::MUL;
    case AssignOp::DIVEQ:
        return BinaryOp::DIV;
    case AssignOp::MODEQ:
        return BinaryOp::MOD;
    case AssignOp::PLUSEQ:
        return BinaryOp::PLUS;
    case AssignOp::MINUSEQ:
        return BinaryOp::MINUS;
    case AssignOp::LSHIFTEQ:
        return BinaryOp::LSHIFT;
    case AssignOp::RSHIFTEQ:
        return BinaryOp::RSHIFT;
    case AssignOp::ANDEQ:
        return BinaryOp::AND;
    case AssignOp::XOREQ:
        return BinaryOp::XOR;
    case AssignOp::OREQ:
        return BinaryOp::OR;
    }
}

} // namespace ecc::sema::prim

#endif