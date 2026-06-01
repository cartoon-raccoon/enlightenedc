#pragma once

#ifndef ECC_PRIMITIVES_H
#define ECC_PRIMITIVES_H

#include <cstdint>

#include "tokens.hpp"
#include "util.hpp"

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
bool unaryop_is_const_foldable(UnaryOp op);

PrimTypeRank pr_rank(PrimType pr);

bool pr_is_integer(PrimType pr);

bool pr_is_float(PrimType pr);

bool pr_is_bool(PrimType pr);

bool pr_is_signed(PrimType pr);

constexpr size_t ONE_BYTE    = 1;
constexpr size_t TWO_BYTES   = 2;
constexpr size_t FOUR_BYTES  = 4;
constexpr size_t EIGHT_BYTES = 8;

constexpr size_t BYTE_WIDTH = 8;

/**
Gets the size of the primitive type in bytes.
*/
size_t pr_size(PrimType pr);

/**
Gets the size of the primitive type in bits.
*/
size_t pr_size_in_bits(PrimType pr);

PrimType pr_from_rank(PrimTypeRank rank, bool is_signed);

/**
Get the unsigned counterpart of `pr`.

This is identity for all types that don't have an unsigned counterpart,
namely Bool, F32, and F64.
*/
PrimType pr_unsigned(PrimType pr);

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
PrimType pr_promote(PrimType lhs, PrimType rhs);

/**
Promote a single type to a minimum type in an expression.

This just applies a floor of I32 rank, preserving signedness, hence it
is identity over any PrimitiveType of where rank >= INT32.
*/
PrimType pr_single_promote(PrimType pr);

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
Optional<PrimExprTypes> pr_check_binary_op(BinaryOp op, PrimType lhs, PrimType rhs);

} // namespace ecc::sema::prim

#endif