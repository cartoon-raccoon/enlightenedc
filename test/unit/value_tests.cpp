#include <cstdint>

#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "primhelpers.hpp"


// ── Structural invariant ──────────────────────────────────────────────────────

RC_GTEST_PROP(ValueStructural, GeneratedValuesAreValid, ()) {
    assert_structural_valid(*gen_value());
}

RC_GTEST_PROP(ValueStructural, AdditionPreservesStructure, ()) {
    assert_structural_valid(*gen_value() + *gen_value());
}

RC_GTEST_PROP(ValueStructural, IntegerBitwisePreservesStructure, ()) {
    Value a = *gen_integer_value();
    Value b = *gen_integer_value();
    assert_structural_valid(a & b);
    assert_structural_valid(a | b);
    assert_structural_valid(a ^ b);
}

// Shift amount in [0,31]: valid for all promoted types (min 32 bits after promotion).
RC_GTEST_PROP(ValueStructural, ShiftPreservesStructure, ()) {
    Value a = *gen_integer_value();
    Value b(*rc::gen::inRange<uint32_t>(0, 32)); // NOLINT
    assert_structural_valid(a << b);
    assert_structural_valid(a >> b);
}

// ── Type promotion — rank floor and ordering ──────────────────────────────────

RC_GTEST_PROP(ValueTypePromotion, RankFloorIsI32, ()) {
    Value result = *gen_value() + *gen_value();
    assert_structural_valid(result);
    RC_ASSERT(pr_rank(result.primtype) >= PrimTypeRank::INT32);
}

RC_GTEST_PROP(ValueTypePromotion, ResultRankAtLeastBothInputs, ()) {
    Value a      = *gen_value();
    Value b      = *gen_value();
    Value result = a + b;
    assert_structural_valid(result);
    RC_ASSERT(pr_rank(result.primtype) >= pr_rank(a.primtype));
    RC_ASSERT(pr_rank(result.primtype) >= pr_rank(b.primtype));
}

// ── Type promotion — signedness ───────────────────────────────────────────────

RC_GTEST_PROP(ValueTypePromotion, UnsignedWins, ()) {
    // One operand is forced unsigned; the other is any integer.
    Value a      = *gen_integer_value();
    Value b      = *gen_unsigned_value();
    Value result = a + b;
    assert_structural_valid(result);
    RC_ASSERT(!pr_is_signed(result.primtype));
}

RC_GTEST_PROP(ValueTypePromotion, TwoSignedIntegersProduceSignedResult, ()) {
    Value a = *rc::gen::mapcat(
        rc::gen::element(PrimType::I8, PrimType::I16, PrimType::I32, PrimType::I64),
        gen_value_of);
    Value b = *rc::gen::mapcat(
        rc::gen::element(PrimType::I8, PrimType::I16, PrimType::I32, PrimType::I64),
        gen_value_of);
    Value result = a + b;
    assert_structural_valid(result);
    RC_ASSERT(pr_is_signed(result.primtype));
}

// ── Type promotion — float dominance ─────────────────────────────────────────

RC_GTEST_PROP(ValueTypePromotion, FloatDominatesNonFloat, ()) {
    Value f      = *gen_float_value();
    Value i      = *gen_non_float_value();
    Value result = f + i;
    assert_structural_valid(result);
    RC_ASSERT(pr_is_float(result.primtype));
}

RC_GTEST_PROP(ValueTypePromotion, F64DominatesF32, ()) {
    Value f64 = *gen_value_of(PrimType::F64);
    Value f32 = *gen_value_of(PrimType::F32);
    RC_ASSERT((f64 + f32).primtype == PrimType::F64);
    RC_ASSERT((f32 + f64).primtype == PrimType::F64);
}

// ── Type promotion — other structural properties ──────────────────────────────

RC_GTEST_PROP(ValueTypePromotion, ArithmeticNeverProducesBool, ()) {
    RC_ASSERT((*gen_value() + *gen_value()).primtype != PrimType::BOOL);
}

RC_GTEST_PROP(ValueTypePromotion, TypeIndependentOfValue, ()) {
    // Same pair of primtypes, different raw values → same result type.
    PrimType pt1 = *gen_primtype();
    PrimType pt2 = *gen_primtype();
    PrimType type1 = (*gen_value_of(pt1) + *gen_value_of(pt2)).primtype;
    PrimType type2 = (*gen_value_of(pt1) + *gen_value_of(pt2)).primtype;
    RC_ASSERT(type1 == type2);
}

RC_GTEST_PROP(ValueTypePromotion, CommutativeOpsHaveSameResultType, ()) {
    Value a = *gen_integer_value();
    Value b = *gen_integer_value();
    RC_ASSERT((a + b).primtype == (b + a).primtype);
    RC_ASSERT((a * b).primtype == (b * a).primtype);
    RC_ASSERT((a & b).primtype == (b & a).primtype);
    RC_ASSERT((a | b).primtype == (b | a).primtype);
    RC_ASSERT((a ^ b).primtype == (b ^ a).primtype);
}

// ── Value::promote consistency ────────────────────────────────────────────────

RC_GTEST_PROP(ValueTypePromotion, PromotePairHasEqualTypes, ()) {
    Value a         = *gen_value();
    Value b         = *gen_value();
    auto [pa, pb]   = Value::promote(a, b);
    assert_structural_valid(pa);
    assert_structural_valid(pb);
    RC_ASSERT(pa.primtype == pb.primtype);
}

RC_GTEST_PROP(ValueTypePromotion, PromotePairTypeMatchesPrPromote, ()) {
    Value a       = *gen_value();
    Value b       = *gen_value();
    auto [pa, pb] = Value::promote(a, b);
    RC_ASSERT(pa.primtype == pr_promote(a.primtype, b.primtype));
}

// ── Comparison operators always produce Bool ──────────────────────────────────

RC_GTEST_PROP(ValueArithmetic, ComparisonResultIsBool, ()) {
    Value a = *gen_value();
    Value b = *gen_value();
    RC_ASSERT((a == b).primtype == PrimType::BOOL);
    RC_ASSERT((a != b).primtype == PrimType::BOOL);
    RC_ASSERT((a <  b).primtype == PrimType::BOOL);
    RC_ASSERT((a >  b).primtype == PrimType::BOOL);
    RC_ASSERT((a <= b).primtype == PrimType::BOOL);
    RC_ASSERT((a >= b).primtype == PrimType::BOOL);
}

// ── Commutativity of values ───────────────────────────────────────────────────

RC_GTEST_PROP(ValueArithmetic, AddIsCommutative, ()) {
    Value a = *gen_integer_value();
    Value b = *gen_integer_value();
    RC_ASSERT(static_cast<bool>((a + b) == (b + a)));
}

RC_GTEST_PROP(ValueArithmetic, MulIsCommutative, ()) {
    Value a = *gen_integer_value();
    Value b = *gen_integer_value();
    RC_ASSERT(static_cast<bool>((a * b) == (b * a)));
}

RC_GTEST_PROP(ValueArithmetic, BitwiseOpsAreCommutative, ()) {
    Value a = *gen_integer_value();
    Value b = *gen_integer_value();
    RC_ASSERT(static_cast<bool>((a & b) == (b & a)));
    RC_ASSERT(static_cast<bool>((a | b) == (b | a)));
    RC_ASSERT(static_cast<bool>((a ^ b) == (b ^ a)));
}

// ── Division / modulo ─────────────────────────────────────────────────────────

// (a/b)*b + a%b == a. Restricted to unsigned to avoid signed overflow UB.
RC_GTEST_PROP(ValueArithmetic, DivModEuclideanIdentity, ()) {
    Value a       = *gen_unsigned_value();
    Value b       = *gen_unsigned_value();
    auto [pa, pb] = Value::promote(a, b);
    RC_PRE(static_cast<bool>(pb));  // discard b == 0
    Value result  = (pa / pb) * pb + pa % pb;
    assert_structural_valid(result);
    RC_ASSERT(static_cast<bool>(result == pa));
}

// ── Unsigned wrapping ─────────────────────────────────────────────────────────

// Unsigned addition must wrap modulo 2^32. Uses U32 directly so the expected
// value is simply the C++ unsigned wraparound of the raw inputs.
RC_GTEST_PROP(ValueArithmetic, UnsignedU32AdditionWraps, ()) {
    uint32_t a_raw = *rc::gen::arbitrary<uint32_t>();
    uint32_t b_raw = *rc::gen::arbitrary<uint32_t>();
    uint32_t expected = a_raw + b_raw;  // defined modular wrap for unsigned
    Value result = Value(a_raw) + Value(b_raw);
    assert_structural_valid(result);
    RC_ASSERT(result.primtype == PrimType::U32);
    RC_ASSERT(result.cast<uint32_t>() == expected);
}

// ── Shift result type (C-standard rule — DISABLED until fix is implemented) ───
//
// C11 §6.5.7: shift result type = independently-promoted left operand.
// The right operand's rank and signedness must NOT affect the result type.
// Currently pr_promote() (usual arithmetic conversions) is applied instead,
// so e.g. I32 << U64 gives U64 rather than I32.
// Enable this test after adding pr_promote_shift to value.cpp.

RC_GTEST_PROP(ValueTypePromotion, DISABLED_ShiftResultTypeIsPromotedLeft, ()) {
    Value a = *gen_integer_value();
    // Bounded shift amount avoids UB from oversized or negative shifts.
    Value b(*rc::gen::inRange<uint32_t>(0, 32)); // NOLINT

    // Expected: independently promote left to at least I32, ignore right.
    PrimType expected_type = pr_rank(a.primtype) >= PrimTypeRank::INT32
        ? a.primtype
        : pr_from_rank(PrimTypeRank::INT32, pr_is_signed(a.primtype));

    Value result = a << b;
    assert_structural_valid(result);
    RC_ASSERT(result.primtype == expected_type);
}
