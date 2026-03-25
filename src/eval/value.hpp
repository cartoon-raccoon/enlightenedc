#ifndef ECC_VALUE_H
#define ECC_VALUE_H

#include <variant>
#include <string>
#include "util.hpp"
#include "error.hpp"

using namespace ecc;
using namespace ecc::util;

namespace ecc::exec {

class InvalidCompileTimeEval : public EccError {
public:
    InvalidCompileTimeEval(std::string msg, Location loc)
    : EccError(msg, loc) {}
};

class Value {
public:
    using ValueType = std::variant<
        std::monostate,
        char,
        long,
        double,
        bool,
        std::string
    >;

    Value() : inner(std::monostate {}) {}
    Value(std::monostate v) : inner(v) {}
    Value(char v) : inner(v) {}
    Value(long v) : inner(v) {}
    Value(double v) : inner(v) {}
    Value(bool v) : inner(v) {}
    Value(std::string v) : inner(v) {}

    ValueType inner;

    template<typename T>
    requires VariantMember<T, ValueType>
    Optional<T> value_as() {
        if (auto *v = std::get_if<T>(&inner)) {
            return *v;
        } else {
            return std::nullopt;
        }
    }

    template<typename T>
    requires VariantMember<T, ValueType>
    bool is() {
        return std::holds_alternative<T>(inner);
    }

    operator bool() const {
        return std::visit(overloaded {
            [](std::monostate) { return false; },
            [](char v) { return v != 0; },
            [](long v) { return v != 0; },
            [](double v) { return v != 0.0; },
            [](bool v) { return v; },
            [](const std::string& v) { return !v.empty(); } 
        }, inner);
    }

    ValueType operator*() const {
        return inner;
    }

    Value operator||(const Value rhs) {
        return Value(static_cast<bool>(*this) || static_cast<bool>(rhs));
    }

    Value operator&&(const Value rhs) {
        return Value(static_cast<bool>(*this) && static_cast<bool>(rhs));
    }

    // Binary bitwise OR
    Value operator|(const Value rhs) {
        return std::visit(
            overloaded{[](auto a, auto b) -> Value {
                using A = std::decay_t<decltype(a)>;
                using B = std::decay_t<decltype(b)>;

                if constexpr ((std::is_same_v<A, char> ||
                               std::is_same_v<A, long> ||
                               std::is_same_v<
                                   A, bool>)&&(std::is_same_v<B, char> ||
                                               std::is_same_v<B, long> ||
                                               std::is_same_v<B, bool>)) {
                    return Value((long)a | (long)b);
                } else {
                    throw InvalidCompileTimeEval("Invalid types for bitwise OR",
                                                 Location{});
                }
            }},
            inner, rhs.inner);
    }

    // Binary bitwise XOR
    Value operator^(const Value rhs) {
        return std::visit(
            overloaded{[](auto a, auto b) -> Value {
                using A = std::decay_t<decltype(a)>;
                using B = std::decay_t<decltype(b)>;

                if constexpr ((std::is_same_v<A, char> ||
                               std::is_same_v<A, long> ||
                               std::is_same_v<
                                   A, bool>)&&(std::is_same_v<B, char> ||
                                               std::is_same_v<B, long> ||
                                               std::is_same_v<B, bool>)) {
                    return Value((long)a ^ (long)b);
                } else {
                    throw InvalidCompileTimeEval(
                        "Invalid types for bitwise XOR", Location{});
                }
            }},
            inner, rhs.inner);
    }

    // Binary bitwise AND
    Value operator&(const Value rhs) {
        return std::visit(
            overloaded{[](auto a, auto b) -> Value {
                using A = std::decay_t<decltype(a)>;
                using B = std::decay_t<decltype(b)>;

                if constexpr ((std::is_same_v<A, char> ||
                               std::is_same_v<A, long> ||
                               std::is_same_v<
                                   A, bool>)&&(std::is_same_v<B, char> ||
                                               std::is_same_v<B, long> ||
                                               std::is_same_v<B, bool>)) {
                    return Value((long)a & (long)b);
                } else {
                    throw InvalidCompileTimeEval(
                        "Invalid types for bitwise AND", Location{});
                }
            }},
            inner, rhs.inner);
    }

    // Binary EQ
    Value operator==(const Value rhs) {
        return std::visit(
            overloaded{[](long a, long b) { return Value(a == b); },
                       [](char a, long b) { return Value(a == b); },
                       [](long a, char b) { return Value(a == b); },
                       [](double a, double b) { return Value(a == b); },
                       [](long a, double b) { return Value((double)a == b); },
                       [](char a, double b) { return Value((double)a == b); },
                       [](double a, long b) { return Value(a == (double)b); },
                       [](double a, char b) { return Value(a == (double)b); },
                       [](const std::string& a, const std::string& b) {
                           return Value(a == b);
                       },
                       [](auto&&, auto&&) -> Value {
                           throw EccError("Invalid types for addition");
                       }},
            inner, rhs.inner);
    }

    // Binary NEQ
    Value operator!=(const Value rhs) {
        return !(*this == rhs);
    }

    // Binary LEQ
    Value operator<=(const Value rhs) {
        return !(*this > rhs);
    }

    // Binary GEQ
    Value operator>=(const Value rhs) {
        return !(*this < rhs);
    }

    // Binary LT
    Value operator<(const Value rhs) {
        return std::visit(
            overloaded{[](long a, long b) { return Value(a < b); },
                       [](char a, long b) { return Value(a < b); },
                       [](long a, char b) { return Value(a < b); },
                       [](double a, double b) { return Value(a < b); },
                       [](long a, double b) { return Value((double)a < b); },
                       [](char a, double b) { return Value((double)a < b); },
                       [](double a, long b) { return Value(a < (double)b); },
                       [](double a, char b) { return Value(a < (double)b); },
                       [](const std::string& a, const std::string& b) {
                           return Value(a < b);
                       },
                       [](auto&&, auto&&) -> Value {
                           throw EccError("Invalid types for addition");
                       }},
            inner, rhs.inner);
    }

    Value operator>(const Value rhs) {
        return std::visit(
            overloaded{[](long a, long b) { return Value(a > b); },
                       [](char a, long b) { return Value(a > b); },
                       [](long a, char b) { return Value(a > b); },
                       [](double a, double b) { return Value(a > b); },
                       [](long a, double b) { return Value((double)a > b); },
                       [](char a, double b) { return Value((double)a > b); },
                       [](double a, long b) { return Value(a > (double)b); },
                       [](double a, char b) { return Value(a > (double)b); },
                       [](const std::string& a, const std::string& b) {
                           return Value(a > b);
                       },
                       [](auto&&, auto&&) -> Value {
                           throw EccError("Invalid types for addition");
                       }},
            inner, rhs.inner);
    }

    Value operator+(const Value rhs) {
        return std::visit(
            overloaded{[](long a, long b) { return Value(a + b); },
                       [](char a, long b) { return Value(a + b); },
                       [](long a, char b) { return Value(a + b); },
                       [](double a, double b) { return Value(a + b); },
                       [](long a, double b) { return Value((double)a + b); },
                       [](char a, double b) { return Value((double)a + b); },
                       [](double a, long b) { return Value(a + (double)b); },
                       [](double a, char b) { return Value(a + (double)b); },
                       [](const std::string& a, const std::string& b) {
                           return Value(a + b);
                       },
                       [](auto&&, auto&&) -> Value {
                           throw EccError("Invalid types for addition");
                       }},
            inner, rhs.inner);
    }

    Value operator-(const Value rhs) {
        return std::visit(
            overloaded{[](long a, long b) { return Value(a - b); },
                       [](char a, long b) { return Value(a - b); },
                       [](long a, char b) { return Value(a - b); },
                       [](double a, double b) { return Value(a - b); },
                       [](long a, double b) { return Value((double)a - b); },
                       [](char a, double b) { return Value((double)a - b); },
                       [](double a, long b) { return Value(a - (double)b); },
                       [](double a, char b) { return Value(a - (double)b); },
                       [](auto&&, auto&&) -> Value {
                           throw EccError("Invalid types for subtraction");
                       }},
            inner, rhs.inner);
    }

    Value operator*(const Value rhs) {
        return std::visit(
            overloaded{[](long a, long b) { return Value(a * b); },
                       [](char a, long b) { return Value(a * b); },
                       [](long a, char b) { return Value(a * b); },
                       [](double a, double b) { return Value(a * b); },
                       [](long a, double b) { return Value((double)a * b); },
                       [](char a, double b) { return Value((double)a * b); },
                       [](double a, long b) { return Value(a * (double)b); },
                       [](double a, char b) { return Value(a * (double)b); },
                       [](auto&&, auto&&) -> Value {
                           throw EccError("Invalid types for multiplication");
                       }},
            inner, rhs.inner);
    }

    Value operator/(const Value rhs) {
        return std::visit(
            overloaded{[](long a, long b) { return Value(a / b); },
                       [](char a, long b) { return Value(a / b); },
                       [](long a, char b) { return Value(a / b); },
                       [](double a, double b) { return Value(a / b); },
                       [](long a, double b) { return Value((double)a / b); },
                       [](char a, double b) { return Value((double)a / b); },
                       [](double a, long b) { return Value(a / (double)b); },
                       [](double a, char b) { return Value(a / (double)b); },
                       [](auto&&, auto&&) -> Value {
                           throw EccError("Invalid types for division");
                       }},
            inner, rhs.inner);
    }

    // Unary logical NOT
    Value operator!() { return Value(!static_cast<bool>(*this)); }

    /*
    Unary bitwise NOT
    */
    Value operator~() {
        return std::visit(overloaded{[](auto v) -> Value {
                              using T = std::decay_t<decltype(v)>;

                              if constexpr (std::is_same_v<T, char> ||
                                            std::is_same_v<T, long> ||
                                            std::is_same_v<T, bool>) {
                                  return Value(~(long)v);
                              } else {
                                  throw InvalidCompileTimeEval(
                                      "Invalid type for bitwise NOT",
                                      Location{});
                              }
                          }},
                          inner);
    }

    // Unary NEG
    Value operator-() {
        return std::visit(overloaded{[](long v) { return Value(-v); },
                                     [](char v) { return Value(-(long)v); },
                                     [](double v) { return Value(-v); },
                                     [](auto&&) -> Value {
                                         throw InvalidCompileTimeEval(
                                             "Invalid type for unary minus",
                                             Location{});
                                     }},
                          inner);
    }

    // Prefix INC
    Value operator++() {
        return std::visit(
            overloaded{[this](long& v) -> Value { return Value(++v); },
                       [this](char& v) -> Value { return Value(++v); },
                       [this](double& v) -> Value { return Value(++v); },
                       [](auto&) -> Value {
                           throw InvalidCompileTimeEval(
                               "Invalid type for prefix ++", Location{});
                       }},
            inner);
    }

    // Prefix DEC
    Value operator--() {
        return std::visit(
            overloaded{[this](long& v) -> Value { return Value(--v); },
                       [this](char& v) -> Value { return Value(--v); },
                       [this](double& v) -> Value { return Value(--v); },
                       [](auto&) -> Value {
                           throw InvalidCompileTimeEval(
                               "Invalid type for prefix --", Location{});
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
        throw InvalidCompileTimeEval(
            "Postfix ++ not allowed in constant expressions", Location{});
    }

    // Postfix DEC
    Value operator--(int) {
        throw InvalidCompileTimeEval(
            "Postfix -- not allowed in constant expressions", Location{});
    }
};

}


#endif