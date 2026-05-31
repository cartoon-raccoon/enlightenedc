#include <cstdint>

#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "eval/value.hpp"

using namespace ecc::eval;
using namespace ecc::tokens;
using namespace ecc::sema::prim;

// ── Helpers ───────────────────────────────────────────────────────────────────

// Maps PrimType to the expected std::variant<...> index in Value::ValueType.
// Mirrors the declaration order in value.hpp:
//   int8_t=0, int16_t=1, int32_t=2, int64_t=3,
//   uint8_t=4, uint16_t=5, uint32_t=6, uint64_t=7,
//   float=8, double=9, bool=10
static size_t expected_variant_idx(PrimType pt) {
    switch (pt) {
    case PrimType::I8:   return 0;
    case PrimType::I16:  return 1;
    case PrimType::I32:  return 2;
    case PrimType::I64:  return 3;
    case PrimType::U8:   return 4;
    case PrimType::U16:  return 5; // NOLINT
    case PrimType::U32:  return 6; // NOLINT
    case PrimType::U64:  return 7; // NOLINT
    case PrimType::F32:  return 8; // NOLINT
    case PrimType::F64:  return 9; // NOLINT
    case PrimType::BOOL: return 10; // NOLINT
    }
}

// Asserts that v.primtype is consistent with the C++ type stored in v.inner.
// Call this after any operation to verify the structural invariant holds.
void assert_structural_valid(const Value& v) {
    RC_ASSERT(v.inner.index() == expected_variant_idx(v.primtype));
}

// ── Generators ────────────────────────────────────────────────────────────────

static rc::Gen<PrimType> gen_primtype() {
    return rc::gen::element(
        PrimType::I8,  PrimType::I16, PrimType::I32, PrimType::I64,
        PrimType::U8,  PrimType::U16, PrimType::U32, PrimType::U64,
        PrimType::F32, PrimType::F64, PrimType::BOOL);
}

static rc::Gen<PrimType> gen_integer_primtype() {
    return rc::gen::element(
        PrimType::I8,  PrimType::I16, PrimType::I32, PrimType::I64,
        PrimType::U8,  PrimType::U16, PrimType::U32, PrimType::U64);
}

static rc::Gen<PrimType> gen_unsigned_primtype() {
    return rc::gen::element(
        PrimType::U8, PrimType::U16, PrimType::U32, PrimType::U64);
}

static rc::Gen<PrimType> gen_float_primtype() {
    return rc::gen::element(PrimType::F32, PrimType::F64);
}

static rc::Gen<PrimType> gen_non_float_primtype() {
    return rc::gen::element(
        PrimType::I8,  PrimType::I16, PrimType::I32, PrimType::I64,
        PrimType::U8,  PrimType::U16, PrimType::U32, PrimType::U64,
        PrimType::BOOL);
}

static rc::Gen<Value> gen_value_of(PrimType pt) {
    switch (pt) {
    case PrimType::I8:   return rc::gen::map(rc::gen::arbitrary<int8_t>(),   [](int8_t v)   { return Value(v); });
    case PrimType::I16:  return rc::gen::map(rc::gen::arbitrary<int16_t>(),  [](int16_t v)  { return Value(v); });
    case PrimType::I32:  return rc::gen::map(rc::gen::arbitrary<int32_t>(),  [](int32_t v)  { return Value(v); });
    case PrimType::I64:  return rc::gen::map(rc::gen::arbitrary<int64_t>(),  [](int64_t v)  { return Value(v); });
    case PrimType::U8:   return rc::gen::map(rc::gen::arbitrary<uint8_t>(),  [](uint8_t v)  { return Value(v); });
    case PrimType::U16:  return rc::gen::map(rc::gen::arbitrary<uint16_t>(), [](uint16_t v) { return Value(v); });
    case PrimType::U32:  return rc::gen::map(rc::gen::arbitrary<uint32_t>(), [](uint32_t v) { return Value(v); });
    case PrimType::U64:  return rc::gen::map(rc::gen::arbitrary<uint64_t>(), [](uint64_t v) { return Value(v); });
    case PrimType::F32:  return rc::gen::map(rc::gen::arbitrary<float>(),    [](float v)    { return Value(v); });
    case PrimType::F64:  return rc::gen::map(rc::gen::arbitrary<double>(),   [](double v)   { return Value(v); });
    case PrimType::BOOL: return rc::gen::map(rc::gen::arbitrary<bool>(),     [](bool v)     { return Value(v); });
    }
}

rc::Gen<Value> gen_value()          { return rc::gen::mapcat(gen_primtype(),          gen_value_of); }
rc::Gen<Value> gen_integer_value()  { return rc::gen::mapcat(gen_integer_primtype(),  gen_value_of); }
rc::Gen<Value> gen_unsigned_value() { return rc::gen::mapcat(gen_unsigned_primtype(), gen_value_of); }
rc::Gen<Value> gen_float_value()    { return rc::gen::mapcat(gen_float_primtype(),    gen_value_of); }
rc::Gen<Value> gen_non_float_value(){ return rc::gen::mapcat(gen_non_float_primtype(),gen_value_of); }