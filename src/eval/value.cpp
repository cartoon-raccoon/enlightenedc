#include "eval/value.hpp"

#include <bit>
#include <limits>
#include <type_traits>

#include "tokens.hpp"
#include "util/assert.hpp"

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
            throw EvalSemanticError("integer overflow in constant expression");
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
            [](PtrValue val) -> uint64_t { return static_cast<uint64_t>(val.address);},

        },
        inner);
}

Value Value::pr_cast(ValueEnum ve) const {
    using VE = ValueEnum;

    if (is_pointer() && is_tok<tokens::PrimType>(ve)) {
        PtrValue ptrval = std::get<PtrValue>(inner);
        switch (ve) {
        case VE::BOOL:
            return static_cast<bool>(ptrval.address);
        case VE::U8:
            return static_cast<uint8_t>(ptrval.address);
        case VE::U16:
            return static_cast<uint16_t>(ptrval.address);
        case VE::U32:
            return static_cast<uint32_t>(ptrval.address);
        case VE::U64:
            return static_cast<uint64_t>(ptrval.address);
        case VE::I8:
            return static_cast<int8_t>(ptrval.address);
        case VE::I16:
            return static_cast<int16_t>(ptrval.address);
        case VE::I32:
            return static_cast<int32_t>(ptrval.address);
        case VE::I64:
            return static_cast<int64_t>(ptrval.address);
        case VE::F32:
        case VE::F64:
            throw EvalSemanticError("cannot cast pointer to a floating point");
        default:
            ECC_UNREACHABLE("subtoken control value used");
        }
    }

    switch (ve) {
    case VE::BOOL:
        return cast<bool>();

    case VE::U8:
        return cast<uint8_t>();

    case VE::U16:
        return cast<uint16_t>();

    case VE::U32:
        return cast<uint32_t>();

    case VE::U64:
        return cast<uint64_t>();

    case VE::I8:
        return cast<int8_t>();

    case VE::I16:
        return cast<int16_t>();

    case VE::I32:
        return cast<int32_t>();

    case VE::I64:
        return cast<int64_t>();

    case VE::F32:
        return cast<float>();

    case VE::F64:
        return cast<double>();

    case VE::PTR:
        return PtrValue(bits(),{});
    default:
        ECC_UNREACHABLE("subtoken control value used");
    }

    ECC_UNREACHABLE("unknown primitive type in pr_cast");
}

Pair<Value, Value> Value::promote(const Value& lhs, const Value& rhs) {
    if (lhs.is_pointer() || rhs.is_pointer()) {
        throw EvalSemanticError("cannot promote a pointer value");
    }

    auto lhs_type = expect<tokens::PrimType>(lhs.type);
    auto rhs_type = expect<tokens::PrimType>(rhs.type);

    PrimType promoted = sema::prim::pr_promote(lhs_type, rhs_type);

    Value ret_lhs = lhs_type == promoted ? lhs : lhs.pr_cast(promoted);
    Value ret_rhs = rhs_type == promoted ? rhs : rhs.pr_cast(promoted);

    return {ret_lhs, ret_rhs};
}

template <typename Compute>
Value Value::apply_binary(tokens::BinaryOp op, const Value& rhs, Compute compute) const {
    if (is_pointer() || rhs.is_pointer()) {
        throw EvalSemanticError("pointer values cannot take part in this binary operation");
    }
    auto my_type = expect<PrimType>(type);
    auto rhs_type = expect<PrimType>(rhs.type);
    // The primitive type algebra (pr_check_binary_op) is the single source of
    // truth for how a binary operator treats its operands and what type it
    // yields. Coerce both operands to the operand types it requires, run the
    // operation, then coerce the result to its expr_type.
    Optional<sema::prim::PrimExprTypes> types =
        sema::prim::pr_check_binary_op(op, my_type, rhs_type);

    if (!types) {
        throw EvalSemanticError("operator not applicable to these value types");
    }

    Value lop = pr_cast(valenum(types->operand_types.first));
    Value rop = rhs.pr_cast(valenum(types->operand_types.second));

    return compute(lop, rop).pr_cast(types->expr_type);
}

Value Value::apply_binary_ptr(BinaryOp op, const Value& rhs) const {
    if (tokens::is_tok<RelationalOp>(op)) {
        if (!(is_pointer() && rhs.is_pointer())) {
            // this is enforcing the same Validator rule that all pointers must be explicitly
            // cast to match before they take part in binary operations.
            throw EvalSemanticError("cannot implicitly coerce primitive to pointer");
        }
        PtrValue lhs_ptr = std::get<PtrValue>(inner);
        PtrValue rhs_ptr = std::get<PtrValue>(rhs.inner);

        switch (op) {
        case BinaryOp::EQ:
            return lhs_ptr == rhs_ptr;
        case BinaryOp::NE:
            return lhs_ptr != rhs_ptr;
        case BinaryOp::LT:
            return lhs_ptr < rhs_ptr;
        case BinaryOp::LE:
            return lhs_ptr <= rhs_ptr;
        case BinaryOp::GT:
            return lhs_ptr > rhs_ptr;
        case BinaryOp::GE:
            return lhs_ptr >= rhs_ptr;
        default:
            ECC_UNREACHABLE("non-relational binary op encountered on relational path");
        }
    } else {
        todo();
    }
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
                    ECC_UNREACHABLE("unexpected operand types for bitwise OR");
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
                    ECC_UNREACHABLE("unexpected operand types for bitwise XOR");
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
                    ECC_UNREACHABLE("unexpected operand types for bitwise AND");
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
        if (cnt < 0 || static_cast<size_t>(cnt) >= sema::prim::pr_size_in_bits(*l.primtype())) {
            // safe to unwrap l.primtype() here, since apply_binary already checks for nullptr
            throw InvalidCompileTimeEval("bitshift count out of range");
        }
        return std::visit(
            match{
                [cnt](int32_t a) -> Value { return Value(static_cast<int32_t>(a << cnt)); },
                [cnt](int64_t a) -> Value { return Value(static_cast<int64_t>(a << cnt)); },
                [cnt](uint32_t a) -> Value { return Value(static_cast<uint32_t>(a << cnt)); },
                [cnt](uint64_t a) -> Value { return Value(static_cast<uint64_t>(a << cnt)); },
                [](auto&&) -> Value {
                    ECC_UNREACHABLE("unexpected operand type for bitshift left");
                },
            },
            *l);
    });
}

Value Value::operator>>(const Value& rhs) const {
    return apply_binary(BinaryOp::RSHIFT, rhs, [](const Value& l, const Value& r) -> Value {
        const int64_t cnt = r.cast<int64_t>();
        if (cnt < 0 || static_cast<size_t>(cnt) >= sema::prim::pr_size_in_bits(*l.primtype())) {
            // safe to unwrap l.primtype() here, since apply_binary already checks for nullptr
            throw EvalSemanticError("bitshift count out of range");
        }
        return std::visit(
            match{
                [cnt](int32_t a) -> Value { return Value(static_cast<int32_t>(a >> cnt)); },
                [cnt](int64_t a) -> Value { return Value(static_cast<int64_t>(a >> cnt)); },
                [cnt](uint32_t a) -> Value { return Value(static_cast<uint32_t>(a >> cnt)); },
                [cnt](uint64_t a) -> Value { return Value(static_cast<uint64_t>(a >> cnt)); },
                [](auto&&) -> Value {
                    ECC_UNREACHABLE("unexpected operand type for bitshift right");
                },
            },
            *l);
    });
}

Value Value::operator%(const Value& rhs) const {
    return apply_binary(BinaryOp::MOD, rhs, [](const Value& l, const Value& r) -> Value {
        if (!static_cast<bool>(r)) {
            throw EvalSemanticError("modulo by zero");
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
                    ECC_UNREACHABLE("unexpected operand types for modulo");
                },
            },
            *l, *r);
    });
}

Value Value::operator==(const Value& rhs) const {
    if (is_pointer() || rhs.is_pointer()) {
        return apply_binary_ptr(BinaryOp::EQ, rhs);
    }
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
                    ECC_UNREACHABLE("unexpected operand types for equality");
                },
            },
            *l, *r);
    });
}

Value Value::operator!=(const Value& rhs) const {
    if (is_pointer() || rhs.is_pointer()) {
        return apply_binary_ptr(BinaryOp::NE, rhs);
    }
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
                    ECC_UNREACHABLE("unexpected operand types for inequality");
                },
            },
            *l, *r);
    });
}

Value Value::operator<(const Value& rhs) const {
    if (is_pointer() || rhs.is_pointer()) {
        return apply_binary_ptr(BinaryOp::LT, rhs);
    }
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
                    ECC_UNREACHABLE("unexpected operand types for less-than");
                },
            },
            *l, *r);
    });
}

Value Value::operator>(const Value& rhs) const {
    if (is_pointer() || rhs.is_pointer()) {
        return apply_binary_ptr(BinaryOp::GT, rhs);
    }
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
                    ECC_UNREACHABLE("unexpected operand types for greater-than");
                },
            },
            *l, *r);
    });
}

Value Value::operator<=(const Value& rhs) const {
    if (is_pointer() || rhs.is_pointer()) {
        return apply_binary_ptr(BinaryOp::LE, rhs);
    }
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
                    ECC_UNREACHABLE("unexpected operand types for less-or-equal");
                },
            },
            *l, *r);
    });
}

Value Value::operator>=(const Value& rhs) const {
    if (is_pointer() || rhs.is_pointer()) {
        return apply_binary_ptr(BinaryOp::GE, rhs);
    }
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
                    ECC_UNREACHABLE("unexpected operand types for greater-or-equal");
                },
            },
            *l, *r);
    });
}

Value Value::operator+(const Value& rhs) const {
    // todo: pointer arithmetic support for all relevant operators
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
                    ECC_UNREACHABLE("unexpected operand types for addition");
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
                    ECC_UNREACHABLE("unexpected operand types for subtraction");
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
                    ECC_UNREACHABLE("unexpected operand types for multiplication");
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
            throw EvalSemanticError("divide by zero");
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
                    ECC_UNREACHABLE("unexpected operand types for division");
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
            [](PtrValue v) { return Value(!v.address); },
            [](auto&&) -> Value {
                throw EvalSemanticError("invalid value type for logical NOT");
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
                throw EvalSemanticError("invalid value type for bitwise NOT");
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
            [](PtrValue) -> Value {
                throw EvalSemanticError("invalid value type for minus");
            },
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
            [](PtrValue) -> Value {
                throw EvalSemanticError("invalid value type for plus");
            },
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
            [](bool v) { return v; },
            [](PtrValue v) { return v.address != 0;},
        },
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
