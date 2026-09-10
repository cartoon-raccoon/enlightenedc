#pragma once

#ifndef ECC_VALUE_H
#define ECC_VALUE_H

#include <cstdint>
#include <ostream>
#include <utility>

#include "error.hpp"
#include "semantics/primitives.hpp"
#include "tokens.hpp"
#include "util/hash.hpp"
#include "prelude.hpp"

using namespace ecc;
using namespace ecc::util;

namespace ecc::sema::types {
    class TypeContext;
}

namespace ecc::eval {

/**
A pointer value.
*/
struct PtrValue {
    size_t address;

    /**
    The pointer stride. Defaults to empty.
    */
    Optional<size_t> width;

    PtrValue(size_t address) : address(address) {}

    PtrValue(size_t address, size_t width): address(address) {
        if (width == 0) {
            this->width = {};
        } else {
            this->width = width;
        }
    }

    bool operator==(const PtrValue& other) const {
        return address == other.address;
    }

    bool operator!=(const PtrValue& other) const {
        return address != other.address;
    }

    bool operator<(const PtrValue& other) const {
        return address < other.address;
    }

    bool operator>(const PtrValue& other) const {
        return address > other.address;
    }

    bool operator<=(const PtrValue& other) const {
        return address <= other.address;
    }

    bool operator>=(const PtrValue& other) const {
        return address >= other.address;
    }
};

}

template <>
struct std::formatter<ecc::eval::PtrValue> : std::formatter<std::string_view> {
    auto format(const ecc::eval::PtrValue&, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format("ptr", ctx); // fixme
    }
};

namespace ecc::eval {

/**
An error thrown when an expression cannot be compile-time evaluated.

This does not mean there is a semantic error in the subtree being evaluated, it simply
means it cannot be evaluated at compile time (an runtime identifier, call expression, etc.).
*/
class InvalidCompileTimeEval : public EccSemError {
public:
    InvalidCompileTimeEval(std::string msg) : EccSemError(std::move(msg)) {}

    InvalidCompileTimeEval(std::string msg, Location loc) : EccSemError(std::move(msg), loc) {}
};

/**
An error thrown when there is a semantic error in the subtree being evaluated.

This means that the subtree being evaluated is not a valid EnlightenedC program, and compilation
should be terminated.
*/
class EvalSemanticError : public EccSemError {
public:
    EvalSemanticError(std::string msg) : EccSemError(std::move(msg)) {}

    EvalSemanticError(std::string msg, Location loc) : EccSemError(std::move(msg), loc) {}
};

class InvalidValueRange : public EccSemError {
public:
    InvalidValueRange(std::string msg) : EccSemError(std::move(msg)) {}
};

using ValueType = std::variant<
    int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t, float, double, bool, PtrValue>;

enum class ValueEnum : uint8_t {
    START = std::to_underlying(tokens::TokenKind::U8),
    U8 = std::to_underlying(tokens::TokenKind::U8),
    U16 = std::to_underlying(tokens::TokenKind::U16),
    U32 = std::to_underlying(tokens::TokenKind::U32),
    U64 = std::to_underlying(tokens::TokenKind::U64),
    I8 = std::to_underlying(tokens::TokenKind::I8),
    I16 = std::to_underlying(tokens::TokenKind::I16),
    I32 = std::to_underlying(tokens::TokenKind::I32),
    I64 = std::to_underlying(tokens::TokenKind::I64),
    F32 = std::to_underlying(tokens::TokenKind::F32),
    F64 = std::to_underlying(tokens::TokenKind::F64),
    BOOL = std::to_underlying(tokens::TokenKind::BOOL),
    PTR = std::to_underlying(tokens::TokenKind::PTR),
    BACK = std::to_underlying(tokens::TokenKind::PTR),
    END = BACK + 1,
    COUNT = END - START,
};

static_assert(tokens::ContiguousSubToken<ValueEnum>);

/*
A value of a primitive type.

A Value is a pure, functional, immutable container over a contained primitive type.
Combining two Values in an operation does not modify either - it produces an entirely
new value.

This is also why certain operators are not overloaded - they operate on memory, at runtime.
They are meaningless on Values.
*/
class [[nodiscard]] Value {

    /**
    The inner
    */
    ValueType inner;

    ValueEnum type;

public:
    Value() : inner((int32_t)0), type(ValueEnum::I32) {}

    Value(int8_t v) : inner(v), type(ValueEnum::I8) {}

    Value(int16_t v) : inner(v), type(ValueEnum::I16) {}

    Value(int32_t v) : inner(v), type(ValueEnum::I32) {}

    Value(int64_t v) : inner(v), type(ValueEnum::I64) {}

    Value(uint8_t v) : inner(v), type(ValueEnum::U8) {}

    Value(uint16_t v) : inner(v), type(ValueEnum::U16) {}

    Value(uint32_t v) : inner(v), type(ValueEnum::U32) {}

    Value(uint64_t v) : inner(v), type(ValueEnum::U64) {}

    Value(float v) : inner(v), type(ValueEnum::F32) {}

    Value(double v) : inner(v), type(ValueEnum::F64) {}

    Value(bool v) : inner(v), type(ValueEnum::BOOL) {}

    Value(PtrValue v) : inner(v), type(ValueEnum::PTR) {}

    Value(const Value& other) : inner(other.inner), type(other.type) {}

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

    static Value pointer(size_t address, size_t width) { return Value(PtrValue{address, width}); }

    static Value null() { return Value(PtrValue{0, {}}); }

    static Value null(size_t width) { return Value(PtrValue{0, width}); } 

    static ValueEnum valenum(tokens::PrimType ptype) {
        using PT = tokens::PrimType;
        using VE = ValueEnum;

        switch (ptype) {
        case PT::U8:
            return VE::U8;
        case PT::U16:
            return VE::U16;
        case PT::U32:
            return VE::U32;
        case PT::U64:
            return VE::U64;
        case PT::I8:
            return VE::I8;
        case PT::I16:
            return VE::I16;
        case PT::I32:
            return VE::I32;
        case PT::I64:
            return VE::I64;
        case PT::F32:
            return VE::F32;
        case PT::F64:
            return VE::F64;
        case PT::BOOL:
            return VE::BOOL;
        default:
            ECC_UNREACHABLE("subtoken control value used");
        }
    }

    template <typename T>
        requires VariantMember<T, ValueType>
    static Value from_ptype(tokens::PrimType ptype, T value) {
        if constexpr (std::is_same_v<T, PtrValue>) {
            // todo: create primitive value from ptrtype
            todo();
        } else {
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
            default:
                ECC_UNREACHABLE("subtoken control value used");
            }
        }
    }

    Value& operator=(const Value& other) {
        inner = other.inner;
        type = other.type;
        return *this;
    }

    Value& operator=(Value&& other) noexcept {
        inner = other.inner;
        type = other.type;
        return *this;
    }

    Optional<tokens::PrimType> primtype() const { return tokens::narrow<tokens::PrimType>(type); }

    ValueEnum valuetype() const { return type; }

    ValueType value() const { return inner; }

    /**
    Returns the bit pattern of the underlying value, as an unsigned 64-bit integer.
    */
    uint64_t bits() const;

    bool is_primitive() const {
        return !is_pointer();
    }

    bool is_integer() const {
        if (is_pointer()) return false;
        return sema::prim::pr_is_integer(tokens::expect<tokens::PrimType>(type));
    }

    bool is_float() const {
        if (is_pointer()) return false;
        return sema::prim::pr_is_float(tokens::expect<tokens::PrimType>(type));
    }

    bool is_bool() const {
        if (is_pointer()) return false;
        return sema::prim::pr_is_bool(tokens::expect<tokens::PrimType>(type));
    }

    bool is_signed() const {
        if (is_pointer()) return false;
        return sema::prim::pr_is_signed(tokens::expect<tokens::PrimType>(type));
    }

    bool is_pointer() const { return tokens::token(type) == tokens::TokenKind::PTR; }

    bool is_nullptr() const {
        if (!is_pointer()) return false;

        PtrValue ptrval = std::get<PtrValue>(inner);
        return ptrval.address == 0;
    }

    bool is_pointer_with_stride() const {
        if (!is_pointer()) {
            return false;
        }

        PtrValue ptr = std::get<PtrValue>(inner);
        return ptr.width.has_value();
    }

    Optional<sema::prim::PrimTypeRank> pr_rank() const {
        if (is_pointer()) {
            return {};
        }
        return sema::prim::pr_rank(tokens::expect<tokens::PrimType>(type));
    }

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
        return std::visit(
            match{
                [](const PtrValue& v) -> T {
                    if constexpr (!std::is_integral_v<T>) {
                        throw EvalSemanticError("cannot cast pointer to a floating point");
                    } else {
                        return static_cast<T>(v.address);
                    }
                },
                [](const auto& v) -> T { return static_cast<T>(v); },
            },
            inner);
    }

    /**
    Cast the Value to a new variant.

    Note about casting to a pointer: This is a lossy operation, as the new pointer stride
    will reset to none. For a width-aware cast, use `cast_to_pointer`.
    */
    Value pr_cast(ValueEnum ve) const;

    /**
    Cast the Value to another primitive type.
    */
    Value pr_cast(tokens::PrimType pr) const { return pr_cast(valenum(pr)); }

    /**
    Cast the Value to a pointer with stride `width`.

    Throws `InvalidCompileTimeEval` if `this` is a floating point.
    */
    Value cast_to_pointer(size_t width) const {
        if (is_float()) {
            throw EvalSemanticError("cannot cast a floating point to a pointer");
        }
        return PtrValue(bits(), width); 
    }

    /**
    Cast the Value to a pointer with no stride.

    Throws `InvalidCompileTimeEval` if `this` is a floating point.
    */
    Value cast_to_pointer() const {
        if (is_float()) {
            throw EvalSemanticError("cannot cast a floating point to a pointer");
        }
        return PtrValue(bits(), {});
    }

    /** Promote a pair of values to a pair compatible with binary operations. */
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

    Value operator!=(const Value& rhs) const;

    // Binary LEQ
    template <typename T>
    Value operator<=(const T& rhs) const {
        return *this <= Value(rhs);
    }

    Value operator<=(const Value& rhs) const;

    // Binary GEQ
    template <typename T>
    Value operator>=(const T& rhs) const {
        return *this >= Value(rhs);
    }

    Value operator>=(const Value& rhs) const;

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

private:
    // Run a primitive binary operator through the type algebra: validate the
    // operand pair with pr_check_binary_op, coerce both operands to the operand
    // types it requires, run `compute`, then coerce the result to its expr_type.
    template <typename Compute>
    Value apply_binary(tokens::BinaryOp op, const Value& rhs, Compute compute) const;

    Value apply_binary_ptr(tokens::BinaryOp op, const Value& rhs) const;
}; // end class Value

/**
A helper struct for hashing a Value.
*/
struct ValueHash {
    size_t operator()(const eval::Value& v) const noexcept {
        return VarHash<ValueEnum, uint64_t>{}(v.valuetype(), v.bits());
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

        ValueRangeItem operator++(int) {
            ValueRangeItem prev = *this;
            curr                = curr + 1;
            return prev;
        }

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