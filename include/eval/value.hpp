#pragma once

#ifndef ECC_VALUE_H
#define ECC_VALUE_H

#include <cstddef>
#include <format>
#include <iterator>
#include <string>
#include <variant>

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

class Value {
public:
    // fixme: create system closer to HolyC's type system and implicit cast rules
    using ValueType = std::variant<char, long, double, bool, std::string>;

    Value() : inner((long)0) {}
    Value(char v) : inner(v) {}
    Value(long v) : inner(v) {}
    Value(double v) : inner(v) {}
    Value(bool v) : inner(v) {}
    Value(std::string v) : inner(v) {}

    ValueType inner;

    template <typename T>
        requires VariantMember<T, ValueType>
    Optional<T> value_as() {
        if (auto *v = std::get_if<T>(&inner)) {
            return *v;
        } else {
            return std::nullopt;
        }
    }

    template <typename T>
        requires VariantMember<T, ValueType>
    bool is() {
        return std::holds_alternative<T>(inner);
    }

    operator bool() const {
        return std::visit(match{[](char v) { return v != 0; }, [](long v) { return v != 0; },
                                [](double v) { return v != 0.0; }, [](bool v) { return v; },
                                [](const std::string& v) { return !v.empty(); }},
                          inner);
    }

    ValueType operator*() const { return inner; }

    Value operator||(const Value& rhs) const {
        return Value(static_cast<bool>(*this) || static_cast<bool>(rhs));
    }

    Value operator&&(const Value& rhs) const {
        return Value(static_cast<bool>(*this) && static_cast<bool>(rhs));
    }

    // Binary bitwise OR
    Value operator|(const Value& rhs) {
        return std::visit(match{[](auto& a, auto& b) -> Value {
                              using A = std::decay_t<decltype(a)>;
                              using B = std::decay_t<decltype(b)>;

                              if constexpr ((std::is_same_v<A, char> || std::is_same_v<A, long> ||
                                             std::is_same_v<A, bool>) &&
                                            (std::is_same_v<B, char> || std::is_same_v<B, long> ||
                                             std::is_same_v<B, bool>)) {
                                  return Value((long)a | (long)b);
                              } else {
                                  throw InvalidCompileTimeEval("Invalid types for bitwise OR");
                              }
                          }},
                          inner, rhs.inner);
    }

    // Binary bitwise XOR
    Value operator^(const Value& rhs) {
        return std::visit(match{[](auto& a, auto& b) -> Value {
                              using A = std::decay_t<decltype(a)>;
                              using B = std::decay_t<decltype(b)>;

                              if constexpr ((std::is_same_v<A, char> || std::is_same_v<A, long> ||
                                             std::is_same_v<A, bool>) &&
                                            (std::is_same_v<B, char> || std::is_same_v<B, long> ||
                                             std::is_same_v<B, bool>)) {
                                  return Value((long)a ^ (long)b);
                              } else {
                                  throw InvalidCompileTimeEval("Invalid types for bitwise XOR");
                              }
                          }},
                          inner, rhs.inner);
    }

    // Binary bitwise AND
    Value operator&(const Value& rhs) {
        return std::visit(match{[](auto& a, auto& b) -> Value {
                              using A = std::decay_t<decltype(a)>;
                              using B = std::decay_t<decltype(b)>;

                              if constexpr ((std::is_same_v<A, char> || std::is_same_v<A, long> ||
                                             std::is_same_v<A, bool>) &&
                                            (std::is_same_v<B, char> || std::is_same_v<B, long> ||
                                             std::is_same_v<B, bool>)) {
                                  return Value((long)a & (long)b);
                              } else {
                                  throw InvalidCompileTimeEval("Invalid types for bitwise AND");
                              }
                          }},
                          inner, rhs.inner);
    }

    // Binary EQ
    Value operator==(const Value& rhs) {
        return std::visit(
            match{[](long a, long b) { return Value(a == b); },
                  [](char a, long b) { return Value(a == b); },
                  [](long a, char b) { return Value(a == b); },
                  [](double a, double b) { return Value(a == b); },
                  [](long a, double b) { return Value((double)a == b); },
                  [](char a, double b) { return Value((double)a == b); },
                  [](double a, long b) { return Value(a == (double)b); },
                  [](double a, char b) { return Value(a == (double)b); },
                  [](const std::string& a, const std::string& b) { return Value(a == b); },
                  [](auto&&, auto&&) -> Value {
                      throw InvalidCompileTimeEval("Invalid types for addition");
                  }},
            inner, rhs.inner);
    }

    // Binary NEQ
    Value operator!=(const Value& rhs) { return !(*this == rhs); }

    // Binary LEQ
    Value operator<=(const Value& rhs) { return !(*this > rhs); }

    // Binary GEQ
    Value operator>=(const Value& rhs) { return !(*this < rhs); }

    // Binary LT
    Value operator<(const Value& rhs) {
        return std::visit(
            match{[](long a, long b) { return Value(a < b); },
                  [](char a, long b) { return Value(a < b); },
                  [](long a, char b) { return Value(a < b); },
                  [](double a, double b) { return Value(a < b); },
                  [](long a, double b) { return Value((double)a < b); },
                  [](char a, double b) { return Value((double)a < b); },
                  [](double a, long b) { return Value(a < (double)b); },
                  [](double a, char b) { return Value(a < (double)b); },
                  [](const std::string& a, const std::string& b) { return Value(a < b); },
                  [](auto&&, auto&&) -> Value {
                      throw InvalidCompileTimeEval("Invalid types for addition");
                  }},
            inner, rhs.inner);
    }

    Value operator>(const Value& rhs) {
        return std::visit(
            match{[](long a, long b) { return Value(a > b); },
                  [](char a, long b) { return Value(a > b); },
                  [](long a, char b) { return Value(a > b); },
                  [](double a, double b) { return Value(a > b); },
                  [](long a, double b) { return Value((double)a > b); },
                  [](char a, double b) { return Value((double)a > b); },
                  [](double a, long b) { return Value(a > (double)b); },
                  [](double a, char b) { return Value(a > (double)b); },
                  [](const std::string& a, const std::string& b) { return Value(a > b); },
                  [](auto&&, auto&&) -> Value {
                      throw InvalidCompileTimeEval("Invalid types for addition");
                  }},
            inner, rhs.inner);
    }

    Value operator+(const Value& rhs) {
        return std::visit(
            match{[](long a, long b) { return Value(a + b); },
                  [](char a, long b) { return Value(a + b); },
                  [](long a, char b) { return Value(a + b); },
                  [](double a, double b) { return Value(a + b); },
                  [](long a, double b) { return Value((double)a + b); },
                  [](char a, double b) { return Value((double)a + b); },
                  [](double a, long b) { return Value(a + (double)b); },
                  [](double a, char b) { return Value(a + (double)b); },
                  [](const std::string& a, const std::string& b) { return Value(a + b); },
                  [](auto&&, auto&&) -> Value {
                      throw InvalidCompileTimeEval("Invalid types for addition");
                  }},
            inner, rhs.inner);
    }

    Value operator-(const Value& rhs) {
        return std::visit(match{[](long a, long b) { return Value(a - b); },
                                [](char a, long b) { return Value(a - b); },
                                [](long a, char b) { return Value(a - b); },
                                [](double a, double b) { return Value(a - b); },
                                [](long a, double b) { return Value((double)a - b); },
                                [](char a, double b) { return Value((double)a - b); },
                                [](double a, long b) { return Value(a - (double)b); },
                                [](double a, char b) { return Value(a - (double)b); },
                                [](auto&&, auto&&) -> Value {
                                    throw InvalidCompileTimeEval("Invalid types for subtraction");
                                }},
                          inner, rhs.inner);
    }

    Value operator*(const Value& rhs) {
        return std::visit(match{[](long a, long b) { return Value(a * b); },
                                [](char a, long b) { return Value(a * b); },
                                [](long a, char b) { return Value(a * b); },
                                [](double a, double b) { return Value(a * b); },
                                [](long a, double b) { return Value((double)a * b); },
                                [](char a, double b) { return Value((double)a * b); },
                                [](double a, long b) { return Value(a * (double)b); },
                                [](double a, char b) { return Value(a * (double)b); },
                                [](auto&&, auto&&) -> Value {
                                    throw InvalidCompileTimeEval(
                                        "Invalid types for multiplication");
                                }},
                          inner, rhs.inner);
    }

    Value operator/(const Value& rhs) {
        return std::visit(match{[](long a, long b) { return Value(a / b); },
                                [](char a, long b) { return Value(a / b); },
                                [](long a, char b) { return Value(a / b); },
                                [](double a, double b) { return Value(a / b); },
                                [](long a, double b) { return Value((double)a / b); },
                                [](char a, double b) { return Value((double)a / b); },
                                [](double a, long b) { return Value(a / (double)b); },
                                [](double a, char b) { return Value(a / (double)b); },
                                [](auto&&, auto&&) -> Value {
                                    throw InvalidCompileTimeEval("Invalid types for division");
                                }},
                          inner, rhs.inner);
    }

    // Unary logical NOT
    Value operator!() const { return Value(!static_cast<bool>(*this)); }

    /*
    Unary bitwise NOT
    */
    Value operator~() {
        return std::visit(match{[](auto& v) -> Value {
                              using T = std::decay_t<decltype(v)>;

                              if constexpr (std::is_same_v<T, char> || std::is_same_v<T, long> ||
                                            std::is_same_v<T, bool>) {
                                  return Value(~(long)v);
                              } else {
                                  throw InvalidCompileTimeEval("Invalid type for bitwise NOT",
                                                               Location{});
                              }
                          }},
                          inner);
    }

    // Unary NEG
    Value operator-() {
        return std::visit(
            match{[](long v) { return Value(-v); }, [](char v) { return Value(-(long)v); },
                  [](double v) { return Value(-v); },
                  [](auto&&) -> Value {
                      throw InvalidCompileTimeEval("Invalid type for unary minus", Location{});
                  }},
            inner);
    }

    // Prefix INC
    Value operator++() {
        return std::visit(match{[this](long& v) -> Value { return Value(++v); },
                                [this](char& v) -> Value { return Value(++v); },
                                [this](double& v) -> Value { return Value(++v); },
                                [](auto&) -> Value {
                                    throw InvalidCompileTimeEval("Invalid type for prefix ++",
                                                                 Location{});
                                }},
                          inner);
    }

    // Prefix DEC
    Value operator--() {
        return std::visit(match{[this](long& v) -> Value { return Value(--v); },
                                [this](char& v) -> Value { return Value(--v); },
                                [this](double& v) -> Value { return Value(--v); },
                                [](auto&) -> Value {
                                    throw InvalidCompileTimeEval("Invalid type for prefix --",
                                                                 Location{});
                                }},
                          inner);
    }

    /*
    For postfix declarators, this should return nothing,
    since the return value is the one before the side-effect
    (the actual value increment/decrement).
    */

    // Postfix INC
    Value operator++(int) {
        throw InvalidCompileTimeEval("Postfix ++ not allowed in constant expressions", Location{});
    }

    // Postfix DEC
    Value operator--(int) {
        throw InvalidCompileTimeEval("Postfix -- not allowed in constant expressions", Location{});
    }

    std::string to_string() {
        return std::visit([this](const auto& v) { return std::format("{}", v); }, inner);
    }
}; // end class Value

class ValueRange {
public:
    ValueRange(Value start, Value end);

    class ValueRangeIter {
        long val;
        ValueRange *range;

    public:
        using difference_type = std::ptrdiff_t;
        using value_type      = long;

        ValueRangeIter(long val, ValueRange *range) : val(val), range(range) {}

        long operator*() const { return val; }

        ValueRangeIter& operator++(int) {
            range->curr++;
            return *this;
        }

        ValueRangeIter& operator++() {
            range->curr++;
            return *this;
        }

        bool operator==(ValueRangeIter& other) const { return val == other.val; }

        bool operator!=(ValueRangeIter& other) const { return !(*this == other); }
    };

    ValueRangeIter begin() { return ValueRangeIter(start, this); }

    ValueRangeIter end() { return ValueRangeIter(finish, this); }

private:
    long start, finish;
    long curr;
};

static_assert(std::input_iterator<ValueRange::ValueRangeIter>);

} // namespace ecc::eval

#endif