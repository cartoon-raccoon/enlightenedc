#pragma once

#include "tokens.hpp"
#ifndef ECC_VALUE_H
#define ECC_VALUE_H

#include "error.hpp"
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

/*
A value of a primitive type.
*/
class Value {
public:
    // fixme: create system closer to HolyC's type system and implicit cast rules
    using ValueType = std::variant<int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t,
                                   uint64_t, double, bool>;

    Value() : inner((int32_t)0), primtype(tokens::PrimType::I32) {}

    Value(int8_t v) : inner(v), primtype(tokens::PrimType::I8) {}

    Value(int16_t v) : inner(v), primtype(tokens::PrimType::I16) {}

    Value(int32_t v) : inner(v), primtype(tokens::PrimType::I32) {}

    Value(int64_t v) : inner(v), primtype(tokens::PrimType::I64) {}

    Value(uint8_t v) : inner(v), primtype(tokens::PrimType::U8) {}

    Value(uint16_t v) : inner(v), primtype(tokens::PrimType::U16) {}

    Value(uint32_t v) : inner(v), primtype(tokens::PrimType::U32) {}

    Value(uint64_t v) : inner(v), primtype(tokens::PrimType::U64) {}

    Value(double v) : inner(v), primtype(tokens::PrimType::F64) {}

    Value(bool v) : inner(v), primtype(tokens::PrimType::BOOL) {}

    Value(const Value& other) : inner(other.inner), primtype(other.primtype) {}

    Value& operator=(const Value& other) {
        inner    = other.inner;
        primtype = other.primtype;
        return *this;
    }

    ValueType inner;

    tokens::PrimType primtype;

    bool is_integer() const { return tokens::pr_is_integer(primtype); }

    bool is_float() const { return tokens::pr_is_float(primtype); }

    bool is_bool() const { return tokens::pr_is_bool(primtype); }

    bool is_signed() const { return tokens::pr_is_signed(primtype); }

    tokens::PrimTypeRank pr_rank() const { return tokens::pr_rank(primtype); }

    template <typename T>
        requires VariantMember<T, ValueType>
    Optional<T> value_as() {
        if (auto *val = std::get_if<T>(&inner)) {
            return *val;
        } else {
            return std::nullopt;
        }
    }

    template <typename T>
        requires VariantMember<T, ValueType>
    T cast() const {
        return std::visit(match{[](auto& v) { return static_cast<T>(v); }}, inner);
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
    template <typename T> Value operator|(const T& rhs) const { return *this | Value(rhs); }

    Value operator|(const Value& rhs) const;

    // Binary bitwise XOR
    template <typename T> Value operator^(const T& rhs) const { return *this ^ Value(rhs); }

    Value operator^(const Value& rhs) const;

    // Binary bitwise AND
    template <typename T> Value operator&(const T& rhs) const { return *this & Value(rhs); }

    Value operator&(const Value& rhs) const;

    // Binary bitshift left
    template <typename T> Value operator<<(const T& rhs) const { return *this << Value(rhs); }

    Value operator<<(const Value& rhs) const;

    // Binary bitshift right
    template <typename T> Value operator>>(const T& rhs) const { return *this >> Value(rhs); }

    Value operator>>(const Value& rhs) const;

    template <typename T> Value operator%(const T& rhs) const { return *this % Value(rhs); }

    Value operator%(const Value& rhs) const;

    // Binary EQ
    template <typename T> Value operator==(const T& rhs) const { return *this == Value(rhs); }

    Value operator==(const Value& rhs) const;

    // Binary NEQ
    template <typename T> Value operator!=(const T& rhs) const { return *this != Value(rhs); }

    Value operator!=(const Value& rhs) const { return !(*this == rhs); }

    // Binary LEQ
    template <typename T> Value operator<=(const T& rhs) const { return *this <= Value(rhs); }

    Value operator<=(const Value& rhs) const { return !(*this > rhs); }

    // Binary GEQ
    template <typename T> Value operator>=(const T& rhs) const { return *this >= Value(rhs); }

    Value operator>=(const Value& rhs) const { return !(*this < rhs); }

    // Logical LT
    template <typename T> Value operator<(const T& rhs) const { return *this < Value(rhs); }

    Value operator<(const Value& rhs) const;

    // Logical GT
    template <typename T> Value operator>(const T& rhs) const { return *this > Value(rhs); }

    Value operator>(const Value& rhs) const;

    // Binary ADD
    template <typename T> Value operator+(const T& rhs) const { return *this + Value(rhs); }

    Value operator+(const Value& rhs) const;

    // Binary SUB
    template <typename T> Value operator-(const T& rhs) const { return *this - Value(rhs); }

    Value operator-(const Value& rhs) const;

    // Binary MUL
    template <typename T> Value operator*(const T& rhs) const { return *this * Value(rhs); }

    Value operator*(const Value& rhs) const;

    // Binary DIV
    template <typename T> Value operator/(const T& rhs) const { return *this / Value(rhs); }

    Value operator/(const Value& rhs) const;

    // Unary logical NOT
    Value operator!() const { return !*this; }

    // Unary bitwise NOT
    Value operator~() const;

    // Unary NEG
    Value operator-() const;

    // Unary POS
    Value operator+() const;

    std::string to_string() {
        return std::visit([this](const auto& v) { return std::format("{}", v); }, inner);
    }
}; // end class Value

class ValueRange {
public:
    ValueRange(Value& start, Value& end);

    class ValueRangeIter {
        Value val;
        ValueRange *range;

    public:
        using difference_type = std::ptrdiff_t;
        using value_type      = Value;

        ValueRangeIter(Value& val, ValueRange *range) : val(val), range(range) {}

        Value operator*() const { return val; }

        ValueRangeIter& operator++(int) {
            range->curr = range->curr + 1;
            return *this;
        }

        ValueRangeIter& operator++() {
            range->curr = range->curr + 1;
            return *this;
        }

        bool operator==(ValueRangeIter& other) const { return val == other.val; }

        bool operator!=(ValueRangeIter& other) const { return !(*this == other); }
    };

    ValueRangeIter begin() { return ValueRangeIter(start, this); }

    ValueRangeIter end() { return ValueRangeIter(finish, this); }

private:
    Value start, finish;
    Value curr;
};

static_assert(std::input_iterator<ValueRange::ValueRangeIter>);

} // namespace ecc::eval

#endif