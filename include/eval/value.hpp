#pragma once

#ifndef ECC_VALUE_H
#define ECC_VALUE_H

#include <cstdint>
#include <ostream>

#include "error.hpp"
#include "semantics/primitives.hpp"
#include "tokens.hpp"
#include "util.hpp"

using namespace ecc;
using namespace ecc::util;

namespace ecc::eval {

class InvalidCompileTimeEval : public EccSemError {
public:
    InvalidCompileTimeEval(std::string msg) : EccSemError(std::move(msg)) {}

    InvalidCompileTimeEval(std::string msg, Location loc) : EccSemError(std::move(msg), loc) {}
};

class InvalidValueRange : public EccSemError {
public:
    InvalidValueRange(std::string msg) : EccSemError(std::move(msg)) {}
};

using ValueType = std::variant<
    int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t, float, double, bool>;

/*
A value of a primitive type.

A Value is a pure, functional, immutable container over a contained primitive type.
Combining two Values in an operation does not modify either - it produces an entirely
new value.

This is also why certain operators are not overloaded - they operate on memory, at runtime.
They are meaningless on Values.
*/
class Value {

    /**
    The inner
    */
    ValueType inner;

    tokens::PrimType ptype;

public:
    Value() : inner((int32_t)0), ptype(tokens::PrimType::I32) {}

    Value(int8_t v) : inner(v), ptype(tokens::PrimType::I8) {}

    Value(int16_t v) : inner(v), ptype(tokens::PrimType::I16) {}

    Value(int32_t v) : inner(v), ptype(tokens::PrimType::I32) {}

    Value(int64_t v) : inner(v), ptype(tokens::PrimType::I64) {}

    Value(uint8_t v) : inner(v), ptype(tokens::PrimType::U8) {}

    Value(uint16_t v) : inner(v), ptype(tokens::PrimType::U16) {}

    Value(uint32_t v) : inner(v), ptype(tokens::PrimType::U32) {}

    Value(uint64_t v) : inner(v), ptype(tokens::PrimType::U64) {}

    Value(float v) : inner(v), ptype(tokens::PrimType::F32) {}

    Value(double v) : inner(v), ptype(tokens::PrimType::F64) {}

    Value(bool v) : inner(v), ptype(tokens::PrimType::BOOL) {}

    Value(const Value& other) : inner(other.inner), ptype(other.ptype) {}

    static Value from_literal(uint64_t lit) {
        if (lit <= INT32_MAX) {
            return Value((int32_t)lit);
        } else if (lit <= INT64_MAX) {
            return Value((int64_t)lit);
        } else {
            return Value(lit);
        }
    }

    static Value from_literal(double lit) { return Value(lit); }

    static Value from_literal(char lit) { return Value((int8_t)lit); }

    static Value from_literal(bool lit) { return Value(lit); }

    template <typename T>
        requires VariantMember<T, ValueType>
    static Value from_ptype(tokens::PrimType ptype, T value) {
        switch (ptype) {
        case tokens::PrimType::U8:
            return Value(static_cast<uint8_t>(value));
        case tokens::PrimType::U16:
            return Value(static_cast<uint16_t>(value));
        case tokens::PrimType::U32:
            return Value(static_cast<uint32_t>(value));
        case tokens::PrimType::U64:
            return Value(static_cast<uint64_t>(value));
        case tokens::PrimType::I8:
            return Value(static_cast<int8_t>(value));
        case tokens::PrimType::I16:
            return Value(static_cast<int16_t>(value));
        case tokens::PrimType::I32:
            return Value(static_cast<int32_t>(value));
        case tokens::PrimType::I64:
            return Value(static_cast<int64_t>(value));
        case tokens::PrimType::F32:
            return Value(static_cast<float>(value));
        case tokens::PrimType::F64:
            return Value(static_cast<double>(value));
        case tokens::PrimType::BOOL:
            return Value(static_cast<bool>(value));
        }
    }

    Value& operator=(const Value& other) {
        inner = other.inner;
        ptype = other.ptype;
        return *this;
    }

    Value& operator=(Value&& other) noexcept {
        inner = other.inner;
        ptype = other.ptype;
        return *this;
    }

    tokens::PrimType primtype() const { return ptype; }

    ValueType value() const { return inner; }

    /**
    Returns the bit pattern of the underlying value, as an unsigned 64-bit integer.
    */
    uint64_t bits() const;

    bool is_integer() const { return sema::prim::pr_is_integer(ptype); }

    bool is_float() const { return sema::prim::pr_is_float(ptype); }

    bool is_bool() const { return sema::prim::pr_is_bool(ptype); }

    bool is_signed() const { return sema::prim::pr_is_signed(ptype); }

    sema::prim::PrimTypeRank pr_rank() const { return sema::prim::pr_rank(ptype); }

    /**
    Get a value's compiler-type value.
    */
    template <typename T>
        requires VariantMember<T, ValueType>
    Optional<T> value_as_exact() {
        if (auto *val = std::get_if<T>(&inner)) {
            return *val;
        } else {
            return std::nullopt;
        }
    }

    template <typename T>
        requires std::is_arithmetic_v<T>
    T cast() const {
        return std::visit([](const auto& v) { return static_cast<T>(v); }, inner);
    }

    Value pr_cast(tokens::PrimType pr) const;

    /// Promote a pair of Values to a compatible
    static Pair<Value, Value> promote(const Value& lhs, const Value& rhs);

    template <typename T>
        requires VariantMember<T, ValueType>
    bool is() {
        return std::holds_alternative<T>(inner);
    }

    operator bool() const;

    // We don't need to overload logical OR or AND, since bool() handles that for us.

    // Dereference operator: get the inner type
    ValueType operator*() const { return inner; }

    // Binary bitwise OR
    template <typename T>
    Value operator|(const T& rhs) const {
        return *this | Value(rhs);
    }

    Value operator|(const Value& rhs) const;

    // Binary bitwise XOR
    template <typename T>
    Value operator^(const T& rhs) const {
        return *this ^ Value(rhs);
    }

    Value operator^(const Value& rhs) const;

    // Binary bitwise AND
    template <typename T>
    Value operator&(const T& rhs) const {
        return *this & Value(rhs);
    }

    Value operator&(const Value& rhs) const;

    // Binary bitshift left
    template <typename T>
    Value operator<<(const T& rhs) const {
        return *this << Value(rhs);
    }

    Value operator<<(const Value& rhs) const;

    // Binary bitshift right
    template <typename T>
    Value operator>>(const T& rhs) const {
        return *this >> Value(rhs);
    }

    Value operator>>(const Value& rhs) const;

    template <typename T>
    Value operator%(const T& rhs) const {
        return *this % Value(rhs);
    }

    Value operator%(const Value& rhs) const;

    // Binary EQ
    template <typename T>
    Value operator==(const T& rhs) const {
        return *this == Value(rhs);
    }

    Value operator==(const Value& rhs) const;

    // Binary NEQ
    template <typename T>
    Value operator!=(const T& rhs) const {
        return *this != Value(rhs);
    }

    Value operator!=(const Value& rhs) const { return !(*this == rhs); }

    // Binary LEQ
    template <typename T>
    Value operator<=(const T& rhs) const {
        return *this <= Value(rhs);
    }

    Value operator<=(const Value& rhs) const { return !(*this > rhs); }

    // Binary GEQ
    template <typename T>
    Value operator>=(const T& rhs) const {
        return *this >= Value(rhs);
    }

    Value operator>=(const Value& rhs) const { return !(*this < rhs); }

    // Logical LT
    template <typename T>
    Value operator<(const T& rhs) const {
        return *this < Value(rhs);
    }

    Value operator<(const Value& rhs) const;

    // Logical GT
    template <typename T>
    Value operator>(const T& rhs) const {
        return *this > Value(rhs);
    }

    Value operator>(const Value& rhs) const;

    // Binary ADD
    template <typename T>
    Value operator+(const T& rhs) const {
        return *this + Value(rhs);
    }

    Value operator+(const Value& rhs) const;

    // Binary SUB
    template <typename T>
    Value operator-(const T& rhs) const {
        return *this - Value(rhs);
    }

    Value operator-(const Value& rhs) const;

    // Binary MUL
    template <typename T>
    Value operator*(const T& rhs) const {
        return *this * Value(rhs);
    }

    Value operator*(const Value& rhs) const;

    // Binary DIV
    template <typename T>
    Value operator/(const T& rhs) const {
        return *this / Value(rhs);
    }

    Value operator/(const Value& rhs) const;

    // Unary logical NOT
    Value operator!() const;

    // Unary bitwise NOT
    Value operator~() const;

    // Unary NEG
    Value operator-() const;

    // Unary POS
    Value operator+() const;

    std::string to_string() const {
        return std::visit([&](const auto& v) { return std::format("{}", v); }, inner);
    }
}; // end class Value

/**
A helper struct for hashing a Value.
*/
struct ValueHash {
    size_t operator()(const eval::Value& v) const noexcept {
        return VarHash<tokens::PrimType, uint64_t>{}(v.primtype(), v.bits());
    }
};

/**
A helper struct for structurally comparing two Values.
*/
struct ValueStructEq {
    bool operator()(const eval::Value& a, const eval::Value& b) const noexcept {
        return a.primtype() == b.primtype() && a.bits() == b.bits();
    }
};

template <typename T>
std::basic_ostream<T>& operator<<(std::basic_ostream<T>& ostr, const Value& val) {
    return ostr << val.to_string();
}

class ValueRange {
public:
    ValueRange(Value& start, Value& end);

    class ValueRangeItem {
        Value curr;

    public:
        using difference_type = std::ptrdiff_t;
        using value_type      = Value;

        ValueRangeItem(Value val) : curr(val) {} // NOLINT

        Value operator*() const { return curr; }

        ValueRangeItem operator++(int) { return ValueRangeItem(curr + 1); }

        ValueRangeItem& operator++() {
            curr = curr + 1;
            return *this;
        }

        bool operator==(const ValueRangeItem& other) const { return curr == other.curr; }

        bool operator!=(const ValueRangeItem& other) const { return !(*this == other); }
    }; // end class ValueRangeItem

    ValueRangeItem begin() { return ValueRangeItem(start); }

    ValueRangeItem end() { return ValueRangeItem(finish); }

private:
    Value start, finish;
};

static_assert(std::input_iterator<ValueRange::ValueRangeItem>);

} // namespace ecc::eval

#endif