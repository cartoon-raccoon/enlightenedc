#include "eval/value.hpp"

#include <bit>
#include <limits>
#include <stdexcept>
#include <type_traits>

#include "tokens.hpp"

using namespace ecc::eval;
using namespace ecc::tokens;

namespace {

// The only integer division/modulo that is undefined in C and C++: the minimum
// value of a signed type divided by -1. The true result is not representable, so
// the hardware traps. Reject it here instead of crashing the compiler.
template <typename T>
void reject_intdiv_overflow(T lhs, T rhs) {
    if constexpr (std::is_signed_v<T>) {
        if (rhs == T{-1} && lhs == std::numeric_limits<T>::min()) {
            throw InvalidCompileTimeEval("integer overflow in constant expression");
        }
    }
}

} // namespace

uint64_t Value::bits() const {
    return std::visit(
        match{
            [](int8_t val) -> uint64_t {
                return std::bit_cast<uint64_t>(static_cast<int64_t>(val));
            },
            [](int16_t val) -> uint64_t {
                return std::bit_cast<uint64_t>(static_cast<int64_t>(val));
            },
            [](int32_t val) -> uint64_t {
                return std::bit_cast<uint64_t>(static_cast<int64_t>(val));
            },
            [](int64_t val) -> uint64_t { return std::bit_cast<uint64_t>(val); },
            [](uint8_t val) -> uint64_t { return static_cast<uint64_t>(val); },
            [](uint16_t val) -> uint64_t { return static_cast<uint64_t>(val); },
            [](uint32_t val) -> uint64_t { return static_cast<uint64_t>(val); },
            [](uint64_t val) -> uint64_t { return val; },
            [](float val) -> uint64_t {
                return static_cast<uint64_t>(std::bit_cast<uint32_t>(val));
            },
            [](double val) -> uint64_t { return std::bit_cast<uint64_t>(val); },
            [](bool val) -> uint64_t { return static_cast<uint64_t>(val); },

        },
        inner);
}

Value Value::pr_cast(PrimType pr) const {
    using P = PrimType;
    switch (pr) {
    case P::BOOL:
        return cast<bool>();

    case P::U8:
        return cast<uint8_t>();

    case P::U16:
        return cast<uint16_t>();

    case P::U32:
        return cast<uint32_t>();

    case P::U64:
        return cast<uint64_t>();

    case P::I8:
        return cast<int8_t>();

    case P::I16:
        return cast<int16_t>();

    case P::I32:
        return cast<int32_t>();

    case P::I64:
        return cast<int64_t>();

    case P::F32:
        return cast<float>();

    case P::F64:
        return cast<double>();
    }

    throw InvalidCompileTimeEval("unknown primitive type in pr_cast");
}

Pair<Value, Value> Value::promote(const Value& lhs, const Value& rhs) {
    PrimType promoted = sema::prim::pr_promote(lhs.ptype, rhs.ptype);

    Value ret_lhs = lhs.ptype == promoted ? lhs : lhs.pr_cast(promoted);
    Value ret_rhs = rhs.ptype == promoted ? rhs : rhs.pr_cast(promoted);

    return {ret_lhs, ret_rhs};
}

template <typename Compute>
Value Value::apply_binary(tokens::BinaryOp op, const Value& rhs, Compute compute) const {
    // The primitive type algebra (pr_check_binary_op) is the single source of
    // truth for how a binary operator treats its operands and what type it
    // yields. Coerce both operands to the operand types it requires, run the
    // operation, then coerce the result to its expr_type.
    Optional<sema::prim::PrimExprTypes> types =
        sema::prim::pr_check_binary_op(op, ptype, rhs.ptype);

    if (!types) {
        throw InvalidCompileTimeEval("operator not applicable to these value types");
    }

    Value lop = pr_cast(types->operand_types.first);
    Value rop = rhs.pr_cast(types->operand_types.second);

    return compute(lop, rop).pr_cast(types->expr_type);
}

// Operands reaching a `compute` lambda have already been coerced by apply_binary
// to the operand types pr_check_binary_op requires. Promotion floors every
// integer operand to at least I32, so only the I32/I64/U32/U64 (and, for the
// arithmetic and comparison operators, F32/F64) alternatives can occur.

Value Value::operator|(const Value& rhs) const {
    return apply_binary(BinaryOp::OR, rhs, [](const Value& l, const Value& r) -> Value {
        return std::visit(
            match{
                [](int32_t a, int32_t b) -> Value { return Value(a | b); },
                [](int64_t a, int64_t b) -> Value { return Value(a | b); },
                [](uint32_t a, uint32_t b) -> Value { return Value(a | b); },
                [](uint64_t a, uint64_t b) -> Value { return Value(a | b); },
                [](auto&&, auto&&) -> Value {
                    throw std::runtime_error("unexpected operand types for bitwise OR");
                },
            },
            *l, *r);
    });
}

Value Value::operator^(const Value& rhs) const {
    return apply_binary(BinaryOp::XOR, rhs, [](const Value& l, const Value& r) -> Value {
        return std::visit(
            match{
                [](int32_t a, int32_t b) -> Value { return Value(a ^ b); },
                [](int64_t a, int64_t b) -> Value { return Value(a ^ b); },
                [](uint32_t a, uint32_t b) -> Value { return Value(a ^ b); },
                [](uint64_t a, uint64_t b) -> Value { return Value(a ^ b); },
                [](auto&&, auto&&) -> Value {
                    throw std::runtime_error("unexpected operand types for bitwise XOR");
                },
            },
            *l, *r);
    });
}

Value Value::operator&(const Value& rhs) const {
    return apply_binary(BinaryOp::AND, rhs, [](const Value& l, const Value& r) -> Value {
        return std::visit(
            match{
                [](int32_t a, int32_t b) -> Value { return Value(a & b); },
                [](int64_t a, int64_t b) -> Value { return Value(a & b); },
                [](uint32_t a, uint32_t b) -> Value { return Value(a & b); },
                [](uint64_t a, uint64_t b) -> Value { return Value(a & b); },
                [](auto&&, auto&&) -> Value {
                    throw std::runtime_error("unexpected operand types for bitwise AND");
                },
            },
            *l, *r);
    });
}

Value Value::operator<<(const Value& rhs) const {
    // pr_check_binary_op promotes only the left operand for a shift; the right
    // operand keeps its own type and is not part of the usual conversions.
    return apply_binary(BinaryOp::LSHIFT, rhs, [](const Value& l, const Value& r) -> Value {
        const int64_t cnt = r.cast<int64_t>();
        if (cnt < 0 || static_cast<size_t>(cnt) >= sema::prim::pr_size_in_bits(l.primtype())) {
            throw InvalidCompileTimeEval("bitshift count out of range");
        }
        return std::visit(
            match{
                [cnt](int32_t a) -> Value { return Value(static_cast<int32_t>(a << cnt)); },
                [cnt](int64_t a) -> Value { return Value(static_cast<int64_t>(a << cnt)); },
                [cnt](uint32_t a) -> Value { return Value(static_cast<uint32_t>(a << cnt)); },
                [cnt](uint64_t a) -> Value { return Value(static_cast<uint64_t>(a << cnt)); },
                [](auto&&) -> Value {
                    throw std::runtime_error("unexpected operand type for bitshift left");
                },
            },
            *l);
    });
}

Value Value::operator>>(const Value& rhs) const {
    return apply_binary(BinaryOp::RSHIFT, rhs, [](const Value& l, const Value& r) -> Value {
        const int64_t cnt = r.cast<int64_t>();
        if (cnt < 0 || static_cast<size_t>(cnt) >= sema::prim::pr_size_in_bits(l.primtype())) {
            throw InvalidCompileTimeEval("bitshift count out of range");
        }
        return std::visit(
            match{
                [cnt](int32_t a) -> Value { return Value(static_cast<int32_t>(a >> cnt)); },
                [cnt](int64_t a) -> Value { return Value(static_cast<int64_t>(a >> cnt)); },
                [cnt](uint32_t a) -> Value { return Value(static_cast<uint32_t>(a >> cnt)); },
                [cnt](uint64_t a) -> Value { return Value(static_cast<uint64_t>(a >> cnt)); },
                [](auto&&) -> Value {
                    throw std::runtime_error("unexpected operand type for bitshift right");
                },
            },
            *l);
    });
}

Value Value::operator%(const Value& rhs) const {
    return apply_binary(BinaryOp::MOD, rhs, [](const Value& l, const Value& r) -> Value {
        if (!static_cast<bool>(r)) {
            throw InvalidCompileTimeEval("modulo by zero");
        }
        return std::visit(
            match{
                [](int32_t a, int32_t b) -> Value {
                    reject_intdiv_overflow(a, b);
                    return Value(a % b);
                },
                [](int64_t a, int64_t b) -> Value {
                    reject_intdiv_overflow(a, b);
                    return Value(a % b);
                },
                [](uint32_t a, uint32_t b) -> Value { return Value(a % b); },
                [](uint64_t a, uint64_t b) -> Value { return Value(a % b); },
                [](auto&&, auto&&) -> Value {
                    throw std::runtime_error("unexpected operand types for modulo");
                },
            },
            *l, *r);
    });
}

Value Value::operator==(const Value& rhs) const {
    return apply_binary(BinaryOp::EQ, rhs, [](const Value& l, const Value& r) -> Value {
        return std::visit(
            match{
                [](int32_t a, int32_t b) -> Value { return Value(a == b); },
                [](int64_t a, int64_t b) -> Value { return Value(a == b); },
                [](uint32_t a, uint32_t b) -> Value { return Value(a == b); },
                [](uint64_t a, uint64_t b) -> Value { return Value(a == b); },
                [](float a, float b) -> Value { return Value(a == b); },
                [](double a, double b) -> Value { return Value(a == b); },
                [](auto&&, auto&&) -> Value {
                    throw std::runtime_error("unexpected operand types for equality");
                },
            },
            *l, *r);
    });
}

Value Value::operator!=(const Value& rhs) const {
    return apply_binary(BinaryOp::NE, rhs, [](const Value& l, const Value& r) -> Value {
        return std::visit(
            match{
                [](int32_t a, int32_t b) -> Value { return Value(a != b); },
                [](int64_t a, int64_t b) -> Value { return Value(a != b); },
                [](uint32_t a, uint32_t b) -> Value { return Value(a != b); },
                [](uint64_t a, uint64_t b) -> Value { return Value(a != b); },
                [](float a, float b) -> Value { return Value(a != b); },
                [](double a, double b) -> Value { return Value(a != b); },
                [](auto&&, auto&&) -> Value {
                    throw std::runtime_error("unexpected operand types for inequality");
                },
            },
            *l, *r);
    });
}

Value Value::operator<(const Value& rhs) const {
    return apply_binary(BinaryOp::LT, rhs, [](const Value& l, const Value& r) -> Value {
        return std::visit(
            match{
                [](int32_t a, int32_t b) -> Value { return Value(a < b); },
                [](int64_t a, int64_t b) -> Value { return Value(a < b); },
                [](uint32_t a, uint32_t b) -> Value { return Value(a < b); },
                [](uint64_t a, uint64_t b) -> Value { return Value(a < b); },
                [](float a, float b) -> Value { return Value(a < b); },
                [](double a, double b) -> Value { return Value(a < b); },
                [](auto&&, auto&&) -> Value {
                    throw std::runtime_error("unexpected operand types for less-than");
                },
            },
            *l, *r);
    });
}

Value Value::operator>(const Value& rhs) const {
    return apply_binary(BinaryOp::GT, rhs, [](const Value& l, const Value& r) -> Value {
        return std::visit(
            match{
                [](int32_t a, int32_t b) -> Value { return Value(a > b); },
                [](int64_t a, int64_t b) -> Value { return Value(a > b); },
                [](uint32_t a, uint32_t b) -> Value { return Value(a > b); },
                [](uint64_t a, uint64_t b) -> Value { return Value(a > b); },
                [](float a, float b) -> Value { return Value(a > b); },
                [](double a, double b) -> Value { return Value(a > b); },
                [](auto&&, auto&&) -> Value {
                    throw std::runtime_error("unexpected operand types for greater-than");
                },
            },
            *l, *r);
    });
}

Value Value::operator<=(const Value& rhs) const {
    return apply_binary(BinaryOp::LE, rhs, [](const Value& l, const Value& r) -> Value {
        return std::visit(
            match{
                [](int32_t a, int32_t b) -> Value { return Value(a <= b); },
                [](int64_t a, int64_t b) -> Value { return Value(a <= b); },
                [](uint32_t a, uint32_t b) -> Value { return Value(a <= b); },
                [](uint64_t a, uint64_t b) -> Value { return Value(a <= b); },
                [](float a, float b) -> Value { return Value(a <= b); },
                [](double a, double b) -> Value { return Value(a <= b); },
                [](auto&&, auto&&) -> Value {
                    throw std::runtime_error("unexpected operand types for less-or-equal");
                },
            },
            *l, *r);
    });
}

Value Value::operator>=(const Value& rhs) const {
    return apply_binary(BinaryOp::GE, rhs, [](const Value& l, const Value& r) -> Value {
        return std::visit(
            match{
                [](int32_t a, int32_t b) -> Value { return Value(a >= b); },
                [](int64_t a, int64_t b) -> Value { return Value(a >= b); },
                [](uint32_t a, uint32_t b) -> Value { return Value(a >= b); },
                [](uint64_t a, uint64_t b) -> Value { return Value(a >= b); },
                [](float a, float b) -> Value { return Value(a >= b); },
                [](double a, double b) -> Value { return Value(a >= b); },
                [](auto&&, auto&&) -> Value {
                    throw std::runtime_error("unexpected operand types for greater-or-equal");
                },
            },
            *l, *r);
    });
}

Value Value::operator+(const Value& rhs) const {
    return apply_binary(BinaryOp::PLUS, rhs, [](const Value& l, const Value& r) -> Value {
        return std::visit(
            match{
                [](int32_t a, int32_t b) -> Value { return Value(a + b); },
                [](int64_t a, int64_t b) -> Value { return Value(a + b); },
                [](uint32_t a, uint32_t b) -> Value { return Value(a + b); },
                [](uint64_t a, uint64_t b) -> Value { return Value(a + b); },
                [](float a, float b) -> Value { return Value(a + b); },
                [](double a, double b) -> Value { return Value(a + b); },
                [](auto&&, auto&&) -> Value {
                    throw std::runtime_error("unexpected operand types for addition");
                },
            },
            *l, *r);
    });
}

Value Value::operator-(const Value& rhs) const {
    return apply_binary(BinaryOp::MINUS, rhs, [](const Value& l, const Value& r) -> Value {
        return std::visit(
            match{
                [](int32_t a, int32_t b) -> Value { return Value(a - b); },
                [](int64_t a, int64_t b) -> Value { return Value(a - b); },
                [](uint32_t a, uint32_t b) -> Value { return Value(a - b); },
                [](uint64_t a, uint64_t b) -> Value { return Value(a - b); },
                [](float a, float b) -> Value { return Value(a - b); },
                [](double a, double b) -> Value { return Value(a - b); },
                [](auto&&, auto&&) -> Value {
                    throw std::runtime_error("unexpected operand types for subtraction");
                },
            },
            *l, *r);
    });
}

Value Value::operator*(const Value& rhs) const {
    return apply_binary(BinaryOp::MUL, rhs, [](const Value& l, const Value& r) -> Value {
        return std::visit(
            match{
                [](int32_t a, int32_t b) -> Value { return Value(a * b); },
                [](int64_t a, int64_t b) -> Value { return Value(a * b); },
                [](uint32_t a, uint32_t b) -> Value { return Value(a * b); },
                [](uint64_t a, uint64_t b) -> Value { return Value(a * b); },
                [](float a, float b) -> Value { return Value(a * b); },
                [](double a, double b) -> Value { return Value(a * b); },
                [](auto&&, auto&&) -> Value {
                    throw std::runtime_error("unexpected operand types for multiplication");
                },
            },
            *l, *r);
    });
}

Value Value::operator/(const Value& rhs) const {
    return apply_binary(BinaryOp::DIV, rhs, [](const Value& l, const Value& r) -> Value {
        // Integer division by zero is an error; IEEE floating-point division by
        // zero is well-defined (inf / nan) and is left alone.
        if (r.is_integer() && !static_cast<bool>(r)) {
            throw InvalidCompileTimeEval("divide by zero");
        }
        return std::visit(
            match{
                [](int32_t a, int32_t b) -> Value {
                    reject_intdiv_overflow(a, b);
                    return Value(a / b);
                },
                [](int64_t a, int64_t b) -> Value {
                    reject_intdiv_overflow(a, b);
                    return Value(a / b);
                },
                [](uint32_t a, uint32_t b) -> Value { return Value(a / b); },
                [](uint64_t a, uint64_t b) -> Value { return Value(a / b); },
                [](float a, float b) -> Value { return Value(a / b); },
                [](double a, double b) -> Value { return Value(a / b); },
                [](auto&&, auto&&) -> Value {
                    throw std::runtime_error("unexpected operand types for division");
                },
            },
            *l, *r);
    });
}

Value Value::operator!() const {
    return std::visit(
        match{
            [](int8_t v) { return Value(!v); }, [](int16_t v) { return Value(!v); },
            [](int32_t v) { return Value(!v); }, [](int64_t v) { return Value(!v); },
            [](uint8_t v) { return Value(!v); }, [](uint16_t v) { return Value(!v); },
            [](uint32_t v) { return Value(!v); }, [](uint64_t v) { return Value(!v); },
            [](float v) { return Value(!(bool)v); }, [](double v) { return Value(!(bool)v); },
            [](bool v) { return Value(!v); },
            [](auto&&) -> Value {
                throw InvalidCompileTimeEval("invalid value type for logical NOT");
            }},
        inner);
}

Value Value::operator~() const {
    return std::visit(
        match{
            [](int8_t v) { return Value(~v); }, [](int16_t v) { return Value(~v); },
            [](int32_t v) { return Value(~v); }, [](int64_t v) { return Value(~v); },
            [](uint8_t v) { return Value(~v); }, [](uint16_t v) { return Value(~v); },
            [](uint32_t v) { return Value(~v); }, [](uint64_t v) { return Value(~v); },
            // `~` triggers integer promotion in C, so `~true` is `~1 == -2` with
            // type int -- not a bool.
            [](bool v) { return Value(~static_cast<int32_t>(v)); },
            [](auto&&) -> Value {
                throw InvalidCompileTimeEval("invalid value type for bitwise NOT");
            }},
        inner);
}

Value Value::operator-() const {
    return std::visit(
        match{
            [](int8_t v) { return Value(-v); },
            [](int16_t v) { return Value(-v); },
            [](int32_t v) { return Value(-v); },
            [](int64_t v) { return Value(-v); },
            [](uint8_t v) { return Value(-v); },
            [](uint16_t v) { return Value(-v); },
            [](uint32_t v) { return Value(-v); },
            [](uint64_t v) { return Value(-v); },
            [](float v) { return Value(-v); },
            [](double v) { return Value(-v); },
            [](bool v) { return Value(-v); },
        },
        inner);
}

Value Value::operator+() const {
    return std::visit(
        match{
            [](int8_t v) { return Value(+v); },
            [](int16_t v) { return Value(+v); },
            [](int32_t v) { return Value(+v); },
            [](int64_t v) { return Value(+v); },
            [](uint8_t v) { return Value(+v); },
            [](uint16_t v) { return Value(+v); },
            [](uint32_t v) { return Value(+v); },
            [](uint64_t v) { return Value(+v); },
            [](float v) { return Value(+v); },
            [](double v) { return Value(+v); },
            [](bool v) { return Value(+v); },
        },
        inner);
}

Value::operator bool() const {
    return std::visit(
        match{
            [](int8_t v) { return v != 0; }, [](int16_t v) { return v != 0; },
            [](int32_t v) { return v != 0; }, [](int64_t v) { return v != 0; },
            [](uint8_t v) { return v != 0; }, [](uint16_t v) { return v != 0; },
            [](uint32_t v) { return v != 0; }, [](uint64_t v) { return v != 0; },
            [](float v) { return v != 0.0; }, [](double v) { return v != 0.0; },
            [](bool v) { return v; }},
        inner);
}

ValueRange::ValueRange(Value& start, Value& end) {
    if (!start.is_integer() || !end.is_integer()) {
        throw InvalidValueRange("invalid range, types must be integers");
    }

    auto promoted = Value::promote(start, end);

    if (promoted.first >= promoted.second) {
        throw InvalidValueRange("invalid range, range start must be less than range end");
    }

    this->start = promoted.first;
    // ranges are inclusive, so our end should be end + 1
    this->finish = promoted.second + 1;
}
