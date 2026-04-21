#include "semantics/primitives.hpp"

namespace ecc::sema::prim {

bool unaryop_is_const_foldable(UnaryOp op) {
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

PrimTypeRank pr_rank(PrimType pr) {
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
    case P::F64:
        return PR::FLT64;
    }
}

PrimType pr_promote(PrimType lhs, PrimType rhs) {
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
                ? lhs // if lhs_rank is already that of a 4-byte integer or higher, use it
                // otherwise, promote it to a 4-byte integer with precalculated signedness
                : pr_from_rank(PrimTypeRank::INT32, use_signed);

        return ret;
    } else {
        PrimType ret =
            rhs_rank >= PrimTypeRank::INT32 ? rhs : pr_from_rank(PrimTypeRank::INT32, use_signed);

        return ret;
    }
}

PrimType pr_from_rank(PrimTypeRank rank, bool is_signed) {
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

bool pr_is_integer(PrimType pr) {
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

bool pr_is_float(PrimType pr) {
    return pr == PrimType::F64 || pr == PrimType::F32;
}

bool pr_is_bool(PrimType pr) {
    return pr == PrimType::BOOL;
}

bool pr_is_signed(PrimType pr) {
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

} // namespace ecc::sema::prim