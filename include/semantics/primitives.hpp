#pragma once

#ifndef ECC_PRIMITIVES_H
#define ECC_PRIMITIVES_H

#include <cstdint>

#include "tokens.hpp"

/**
\namespace ecc::sema::prim

The source of truth for all operations involving primitive types.
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

PrimType pr_from_rank(PrimTypeRank rank, bool is_signed);

/**
Promote the LHS and RHS of some binary expression to suitable types.

For binary expressions where the left and right hand sides are not
the same type, we need some deterministic way to get them to the same
type. We do this with type promotion.
*/
PrimType pr_promote(PrimType lhs, PrimType rhs);

}

#endif