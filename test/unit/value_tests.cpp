#include <bit>
#include <cstdint>
#include <unordered_map>

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
    RC_ASSERT(pr_rank(result.primtype()) >= PrimTypeRank::INT32);
}

RC_GTEST_PROP(ValueTypePromotion, ResultRankAtLeastBothInputs, ()) {
    Value a      = *gen_value();
    Value b      = *gen_value();
    Value result = a + b;
    assert_structural_valid(result);
    RC_ASSERT(pr_rank(result.primtype()) >= pr_rank(a.primtype()));
    RC_ASSERT(pr_rank(result.primtype()) >= pr_rank(b.primtype()));
}

// ── Type promotion — signedness ───────────────────────────────────────────────

RC_GTEST_PROP(ValueTypePromotion, UnsignedWins, ()) {
    // One operand is forced unsigned; the other is any integer.
    Value a      = *gen_integer_value();
    Value b      = *gen_unsigned_value();
    Value result = a + b;
    assert_structural_valid(result);
    RC_ASSERT(!pr_is_signed(result.primtype()));
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
    RC_ASSERT(pr_is_signed(result.primtype()));
}

// ── Type promotion — float dominance ─────────────────────────────────────────

RC_GTEST_PROP(ValueTypePromotion, FloatDominatesNonFloat, ()) {
    Value f      = *gen_float_value();
    Value i      = *gen_non_float_value();
    Value result = f + i;
    assert_structural_valid(result);
    RC_ASSERT(pr_is_float(result.primtype()));
}

RC_GTEST_PROP(ValueTypePromotion, F64DominatesF32, ()) {
    Value f64 = *gen_value_of(PrimType::F64);
    Value f32 = *gen_value_of(PrimType::F32);
    RC_ASSERT((f64 + f32).primtype() == PrimType::F64);
    RC_ASSERT((f32 + f64).primtype() == PrimType::F64);
}

// ── Type promotion — other structural properties ──────────────────────────────

RC_GTEST_PROP(ValueTypePromotion, ArithmeticNeverProducesBool, ()) {
    RC_ASSERT((*gen_value() + *gen_value()).primtype() != PrimType::BOOL);
}

RC_GTEST_PROP(ValueTypePromotion, TypeIndependentOfValue, ()) {
    // Same pair of primtypes, different raw values → same result type.
    PrimType pt1 = *gen_primtype();
    PrimType pt2 = *gen_primtype();
    PrimType type1 = (*gen_value_of(pt1) + *gen_value_of(pt2)).primtype();
    PrimType type2 = (*gen_value_of(pt1) + *gen_value_of(pt2)).primtype();
    RC_ASSERT(type1 == type2);
}

RC_GTEST_PROP(ValueTypePromotion, CommutativeOpsHaveSameResultType, ()) {
    Value a = *gen_integer_value();
    Value b = *gen_integer_value();
    RC_ASSERT((a + b).primtype() == (b + a).primtype());
    RC_ASSERT((a * b).primtype() == (b * a).primtype());
    RC_ASSERT((a & b).primtype() == (b & a).primtype());
    RC_ASSERT((a | b).primtype() == (b | a).primtype());
    RC_ASSERT((a ^ b).primtype() == (b ^ a).primtype());
}

// ── Value::promote consistency ────────────────────────────────────────────────

RC_GTEST_PROP(ValueTypePromotion, PromotePairHasEqualTypes, ()) {
    Value a         = *gen_value();
    Value b         = *gen_value();
    auto [pa, pb]   = Value::promote(a, b);
    assert_structural_valid(pa);
    assert_structural_valid(pb);
    RC_ASSERT(pa.primtype() == pb.primtype());
}

RC_GTEST_PROP(ValueTypePromotion, PromotePairTypeMatchesPrPromote, ()) {
    Value a       = *gen_value();
    Value b       = *gen_value();
    auto [pa, pb] = Value::promote(a, b);
    RC_ASSERT(pa.primtype() == pr_promote(a.primtype(), b.primtype()));
}

// ── Comparison operators always produce Bool ──────────────────────────────────

RC_GTEST_PROP(ValueArithmetic, ComparisonResultIsBool, ()) {
    Value a = *gen_value();
    Value b = *gen_value();
    RC_ASSERT((a == b).primtype() == PrimType::BOOL);
    RC_ASSERT((a != b).primtype() == PrimType::BOOL);
    RC_ASSERT((a <  b).primtype() == PrimType::BOOL);
    RC_ASSERT((a >  b).primtype() == PrimType::BOOL);
    RC_ASSERT((a <= b).primtype() == PrimType::BOOL);
    RC_ASSERT((a >= b).primtype() == PrimType::BOOL);
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
    RC_ASSERT(result.primtype() == PrimType::U32);
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
    PrimType expected_type = pr_rank(a.primtype()) >= PrimTypeRank::INT32
        ? a.primtype()
        : pr_from_rank(PrimTypeRank::INT32, pr_is_signed(a.primtype()));

    Value result = a << b;
    assert_structural_valid(result);
    RC_ASSERT(result.primtype() == expected_type);
}

// ── bits() ────────────────────────────────────────────────────────────────────
//
// bits() returns the value's raw bit pattern as a uint64_t: signed integers are
// sign-extended, unsigned integers are zero-extended, floats are bit-cast then
// zero-extended, and doubles are bit-cast directly.

RC_GTEST_PROP(ValueBits, SignedIntegersSignExtend, ()) {
    int8_t v8   = *rc::gen::arbitrary<int8_t>();
    int16_t v16 = *rc::gen::arbitrary<int16_t>();
    int32_t v32 = *rc::gen::arbitrary<int32_t>();
    int64_t v64 = *rc::gen::arbitrary<int64_t>();

    RC_ASSERT(Value(v8).bits() == static_cast<uint64_t>(static_cast<int64_t>(v8)));
    RC_ASSERT(Value(v16).bits() == static_cast<uint64_t>(static_cast<int64_t>(v16)));
    RC_ASSERT(Value(v32).bits() == static_cast<uint64_t>(static_cast<int64_t>(v32)));
    RC_ASSERT(Value(v64).bits() == static_cast<uint64_t>(v64));
}

RC_GTEST_PROP(ValueBits, UnsignedIntegersZeroExtend, ()) {
    uint8_t v8   = *rc::gen::arbitrary<uint8_t>();
    uint16_t v16 = *rc::gen::arbitrary<uint16_t>();
    uint32_t v32 = *rc::gen::arbitrary<uint32_t>();
    uint64_t v64 = *rc::gen::arbitrary<uint64_t>();

    RC_ASSERT(Value(v8).bits() == static_cast<uint64_t>(v8));
    RC_ASSERT(Value(v16).bits() == static_cast<uint64_t>(v16));
    RC_ASSERT(Value(v32).bits() == static_cast<uint64_t>(v32));
    RC_ASSERT(Value(v64).bits() == v64);
}

RC_GTEST_PROP(ValueBits, FloatBitCastThenZeroExtended, ()) {
    float f = *rc::gen::arbitrary<float>();
    RC_ASSERT(Value(f).bits() == static_cast<uint64_t>(std::bit_cast<uint32_t>(f)));
    // Upper 32 bits must be clear -- bits() must not sign-extend a float's bit pattern.
    RC_ASSERT((Value(f).bits() >> 32) == 0U); // NOLINT
}

RC_GTEST_PROP(ValueBits, DoubleBitCastDirectly, ()) {
    double d = *rc::gen::arbitrary<double>();
    RC_ASSERT(Value(d).bits() == std::bit_cast<uint64_t>(d));
}

TEST(ValueBits, BoolBitsAreZeroOrOne) {
    EXPECT_EQ(Value(true).bits(), 1U);
    EXPECT_EQ(Value(false).bits(), 0U);
}

// Negative signed integers must not collide with any unsigned bit pattern of the
// same width -- sign extension into the upper bits is what keeps them distinct.
TEST(ValueBits, NegativeI8DiffersFromU8SameLowByte) {
    Value negative(static_cast<int8_t>(-1));
    Value unsigned_same_byte(static_cast<uint8_t>(0xFF));
    EXPECT_NE(negative.bits(), unsigned_same_byte.bits());
}

// Rebuilds a Value holding the exact same stored representation as `v`, but via
// a distinct construction path (std::visit + the scalar constructor, rather than
// the copy constructor) -- so the two comparisons below aren't tautological.
static Value rebuild_same_representation(const Value& v) {
    return std::visit([](auto raw) { return Value(raw); }, v.value());
}

// bits() is a pure function of the stored representation: two Values holding the
// same primtype and raw representation always yield the same bits().
RC_GTEST_PROP(ValueBits, DeterministicForEqualConstruction, ()) {
    Value a = *gen_value();
    Value b = rebuild_same_representation(a);
    RC_ASSERT(a.primtype() == b.primtype());
    RC_ASSERT(a.bits() == b.bits());
}

// ── ValueHash / ValueStructEq ────────────────────────────────────────────────
//
// ValueStructEq compares primtype() and bits() -- unlike Value::operator==,
// which performs a value-level comparison and can equate values of different
// primtype (e.g. I32(0) == U8(0)). ValueHash is consistent with ValueStructEq:
// equal-under-ValueStructEq values must hash equal.

// A Value structurally equals itself.
RC_GTEST_PROP(ValueStructEqProp, ReflexiveOnSelf, ()) {
    Value v = *gen_value();
    ValueStructEq eq;
    RC_ASSERT(eq(v, v));
}

// Two Values built from the same primtype and raw bit pattern compare structurally equal.
RC_GTEST_PROP(ValueStructEqProp, EqualForSamePrimtypeAndBits, ()) {
    Value a = *gen_value();
    Value b = rebuild_same_representation(a);
    ValueStructEq eq;
    RC_ASSERT(eq(a, b));
}

// Same bit pattern, different primtype: ValueStructEq says unequal even though
// Value::operator== (a value-level comparison) may say equal.
TEST(ValueStructEqProp, DifferentPrimtypeSameBitsAreNotStructurallyEqual) {
    Value i32_zero(static_cast<int32_t>(0));
    Value u8_zero(static_cast<uint8_t>(0));
    ValueStructEq eq;

    EXPECT_FALSE(eq(i32_zero, u8_zero))
        << "ValueStructEq must distinguish primtype even when bits() matches";
    EXPECT_TRUE(static_cast<bool>(i32_zero == u8_zero))
        << "sanity check: Value::operator== is value-level and does treat these as equal";
}

// Same primtype, different value: not structurally equal.
RC_GTEST_PROP(ValueStructEqProp, DifferentBitsAreNotStructurallyEqual, ()) {
    PrimType pt = *gen_integer_primtype();
    Value a     = *gen_value_of(pt);
    Value b     = *gen_value_of(pt);
    RC_PRE(a.bits() != b.bits());
    ValueStructEq eq;
    RC_ASSERT(!eq(a, b));
}

// ValueHash is consistent with ValueStructEq: values that structurally compare
// equal must hash to the same value.
RC_GTEST_PROP(ValueHashProp, ConsistentWithStructEq, ()) {
    Value a = *gen_value();
    Value b = rebuild_same_representation(a);
    ValueStructEq eq;
    ValueHash hash;
    RC_PRE(eq(a, b));
    RC_ASSERT(hash(a) == hash(b));
}

// Value can be used as a key in an unordered_map keyed by ValueHash/ValueStructEq
// (the intended use case, mirroring sema::SwitchTracker's case-value map).
TEST(ValueHashProp, UsableAsUnorderedMapKey) {
    std::unordered_map<Value, std::string, ValueHash, ValueStructEq> map;

    map.insert({Value(static_cast<int32_t>(1)), "one"});
    map.insert({Value(static_cast<int32_t>(2)), "two"});
    // Same bit pattern as the I32 key above, but a different primtype: must not collide.
    map.insert({Value(static_cast<uint8_t>(1)), "one-u8"});

    EXPECT_EQ(map.size(), 3U);
    EXPECT_EQ(map.at(Value(static_cast<int32_t>(1))), "one");
    EXPECT_EQ(map.at(Value(static_cast<int32_t>(2))), "two");
    EXPECT_EQ(map.at(Value(static_cast<uint8_t>(1))), "one-u8");
    EXPECT_EQ(map.find(Value(static_cast<int32_t>(3))), map.end());
}
