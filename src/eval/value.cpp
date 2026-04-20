#include "eval/value.hpp"

#include <stdexcept>

#include "tokens.hpp"

using namespace ecc::eval;
using namespace ecc::tokens;

using ValueType = Value::ValueType;

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
}

Pair<Value, Value> Value::promote(const Value& lhs, const Value& rhs) {
    PrimType promoted = pr_promote(lhs.primtype, rhs.primtype);

    Value ret_lhs = lhs.primtype == promoted ? lhs : lhs.pr_cast(promoted);
    Value ret_rhs = rhs.primtype == promoted ? rhs : rhs.pr_cast(promoted);

    return {ret_lhs, ret_rhs};
}

Value Value::operator|(const Value& rhs) const {
    if (is_float() || rhs.is_float()) {
        // fixme: better error handling
        throw InvalidCompileTimeEval("invalid value types for bitwise OR");
    }
    auto pr = promote(*this, rhs);
    return std::visit(match{
                          [](int8_t l, int8_t r) -> Value { return Value(l | r); },
                          [](int16_t l, int16_t r) -> Value { return Value(l | r); },
                          [](int32_t l, int32_t r) -> Value { return Value(l | r); },
                          [](int64_t l, int64_t r) -> Value { return Value(l | r); },
                          [](uint8_t l, uint8_t r) -> Value { return Value(l | r); },
                          [](uint16_t l, uint16_t r) -> Value { return Value(l | r); },
                          [](uint32_t l, uint32_t r) -> Value { return Value(l | r); },
                          [](uint64_t l, uint64_t r) -> Value { return Value(l | r); },
                          [](bool l, bool r) -> Value { return Value(l | r); },
                          [](auto&& l, auto&& r) -> Value {
                              throw std::runtime_error(
                                  "unexpected type pair while evaluating value");
                          },
                      },
                      pr.first.inner, pr.second.inner);
}

Value Value::operator^(const Value& rhs) const {
    if (is_float() || rhs.is_float()) {
        throw InvalidCompileTimeEval("invalid value types for bitwise XOR");
    }
    auto pr = promote(*this, rhs);
    return std::visit(match{
                          [](int8_t l, int8_t r) -> Value { return Value(l ^ r); },
                          [](int16_t l, int16_t r) -> Value { return Value(l ^ r); },
                          [](int32_t l, int32_t r) -> Value { return Value(l ^ r); },
                          [](int64_t l, int64_t r) -> Value { return Value(l ^ r); },
                          [](uint8_t l, uint8_t r) -> Value { return Value(l ^ r); },
                          [](uint16_t l, uint16_t r) -> Value { return Value(l ^ r); },
                          [](uint32_t l, uint32_t r) -> Value { return Value(l ^ r); },
                          [](uint64_t l, uint64_t r) -> Value { return Value(l ^ r); },
                          [](bool l, bool r) -> Value { return Value(l ^ r); },
                          [](auto&& l, auto&& r) -> Value {
                              throw std::runtime_error(
                                  "unexpected type pair while evaluating value");
                          },
                      },
                      pr.first.inner, pr.second.inner);
}

Value Value::operator&(const Value& rhs) const {
    if (is_float() || rhs.is_float()) {
        throw InvalidCompileTimeEval("invalid value types for bitwise AND");
    }
    auto pr = promote(*this, rhs);
    return std::visit(match{
                          [](int8_t l, int8_t r) -> Value { return Value(l & r); },
                          [](int16_t l, int16_t r) -> Value { return Value(l & r); },
                          [](int32_t l, int32_t r) -> Value { return Value(l & r); },
                          [](int64_t l, int64_t r) -> Value { return Value(l & r); },
                          [](uint8_t l, uint8_t r) -> Value { return Value(l & r); },
                          [](uint16_t l, uint16_t r) -> Value { return Value(l & r); },
                          [](uint32_t l, uint32_t r) -> Value { return Value(l & r); },
                          [](uint64_t l, uint64_t r) -> Value { return Value(l & r); },
                          [](bool l, bool r) -> Value { return Value(l & r); },
                          [](auto&& l, auto&& r) -> Value {
                              throw std::runtime_error(
                                  "unexpected type pair while evaluating value");
                          },
                      },
                      pr.first.inner, pr.second.inner);
}

Value Value::operator<<(const Value& rhs) const {
    if (is_float() || rhs.is_float()) {
        throw InvalidCompileTimeEval("invalid value types for bitshift left");
    }
    auto pr = promote(*this, rhs);
    return std::visit(match{
                          [](int8_t l, int8_t r) -> Value { return Value(l << r); },
                          [](int16_t l, int16_t r) -> Value { return Value(l << r); },
                          [](int32_t l, int32_t r) -> Value { return Value(l << r); },
                          [](int64_t l, int64_t r) -> Value { return Value(l << r); },
                          [](uint8_t l, uint8_t r) -> Value { return Value(l << r); },
                          [](uint16_t l, uint16_t r) -> Value { return Value(l << r); },
                          [](uint32_t l, uint32_t r) -> Value { return Value(l << r); },
                          [](uint64_t l, uint64_t r) -> Value { return Value(l << r); },
                          [](bool l, bool r) -> Value { return Value(l << r); },
                          [](auto&& l, auto&& r) -> Value {
                              throw std::runtime_error(
                                  "unexpected type pair while evaluating value");
                          },
                      },
                      pr.first.inner, pr.second.inner);
}

Value Value::operator>>(const Value& rhs) const {
    if (is_float() || rhs.is_float()) {
        throw InvalidCompileTimeEval("invalid value types for bitshift right");
    }
    auto pr = promote(*this, rhs);
    return std::visit(match{
                          [](int8_t l, int8_t r) -> Value { return Value(l >> r); },
                          [](int16_t l, int16_t r) -> Value { return Value(l >> r); },
                          [](int32_t l, int32_t r) -> Value { return Value(l >> r); },
                          [](int64_t l, int64_t r) -> Value { return Value(l >> r); },
                          [](uint8_t l, uint8_t r) -> Value { return Value(l >> r); },
                          [](uint16_t l, uint16_t r) -> Value { return Value(l >> r); },
                          [](uint32_t l, uint32_t r) -> Value { return Value(l >> r); },
                          [](uint64_t l, uint64_t r) -> Value { return Value(l >> r); },
                          [](bool l, bool r) -> Value { return Value(l >> r); },
                          [](auto&& l, auto&& r) -> Value {
                              throw std::runtime_error(
                                  "unexpected type pair while evaluating value");
                          },
                      },
                      pr.first.inner, pr.second.inner);
}

Value Value::operator%(const Value& rhs) const {
    if (is_float() || rhs.is_float()) {
        throw InvalidCompileTimeEval("invalid value types for MOD");
    }
    auto pr = promote(*this, rhs);
    return std::visit(match{
                          [](int8_t l, int8_t r) -> Value { return Value(l % r); },
                          [](int16_t l, int16_t r) -> Value { return Value(l % r); },
                          [](int32_t l, int32_t r) -> Value { return Value(l % r); },
                          [](int64_t l, int64_t r) -> Value { return Value(l % r); },
                          [](uint8_t l, uint8_t r) -> Value { return Value(l % r); },
                          [](uint16_t l, uint16_t r) -> Value { return Value(l % r); },
                          [](uint32_t l, uint32_t r) -> Value { return Value(l % r); },
                          [](uint64_t l, uint64_t r) -> Value { return Value(l % r); },
                          [](bool l, bool r) -> Value { return Value(l % r); },
                          [](auto&& l, auto&& r) -> Value {
                              throw std::runtime_error(
                                  "unexpected type pair while evaluating value");
                          },
                      },
                      pr.first.inner, pr.second.inner);
}

Value Value::operator==(const Value& rhs) const {
    auto pr = promote(*this, rhs);
    return std::visit(match{
                          [](int8_t l, int8_t r) -> Value { return Value(l == r); },
                          [](int16_t l, int16_t r) -> Value { return Value(l == r); },
                          [](int32_t l, int32_t r) -> Value { return Value(l == r); },
                          [](int64_t l, int64_t r) -> Value { return Value(l == r); },
                          [](uint8_t l, uint8_t r) -> Value { return Value(l == r); },
                          [](uint16_t l, uint16_t r) -> Value { return Value(l == r); },
                          [](uint32_t l, uint32_t r) -> Value { return Value(l == r); },
                          [](uint64_t l, uint64_t r) -> Value { return Value(l == r); },
                          [](float l, float r) -> Value { return Value(l == r); },
                          [](double l, double r) -> Value { return Value(l == r); },
                          [](bool l, bool r) -> Value { return Value(l == r); },
                          [](auto&& l, auto&& r) -> Value {
                              throw std::runtime_error(
                                  "unexpected type pair while evaluating value");
                          },
                      },
                      pr.first.inner, pr.second.inner);
}

Value Value::operator<(const Value& rhs) const {
    auto pr = promote(*this, rhs);
    return std::visit(match{
                          [](int8_t l, int8_t r) -> Value { return Value(l < r); },
                          [](int16_t l, int16_t r) -> Value { return Value(l < r); },
                          [](int32_t l, int32_t r) -> Value { return Value(l < r); },
                          [](int64_t l, int64_t r) -> Value { return Value(l < r); },
                          [](uint8_t l, uint8_t r) -> Value { return Value(l < r); },
                          [](uint16_t l, uint16_t r) -> Value { return Value(l < r); },
                          [](uint32_t l, uint32_t r) -> Value { return Value(l < r); },
                          [](uint64_t l, uint64_t r) -> Value { return Value(l < r); },
                          [](float l, float r) -> Value { return Value(l < r); },
                          [](double l, double r) -> Value { return Value(l < r); },
                          [](bool l, bool r) -> Value { return Value(l < r); },
                          [](auto&& l, auto&& r) -> Value {
                              throw std::runtime_error(
                                  "unexpected type pair while evaluating value");
                          },
                      },
                      pr.first.inner, pr.second.inner);
}

Value Value::operator>(const Value& rhs) const {
    auto pr = promote(*this, rhs);
    return std::visit(match{
                          [](int8_t l, int8_t r) -> Value { return Value(l > r); },
                          [](int16_t l, int16_t r) -> Value { return Value(l > r); },
                          [](int32_t l, int32_t r) -> Value { return Value(l > r); },
                          [](int64_t l, int64_t r) -> Value { return Value(l > r); },
                          [](uint8_t l, uint8_t r) -> Value { return Value(l > r); },
                          [](uint16_t l, uint16_t r) -> Value { return Value(l > r); },
                          [](uint32_t l, uint32_t r) -> Value { return Value(l > r); },
                          [](uint64_t l, uint64_t r) -> Value { return Value(l > r); },
                          [](float l, float r) -> Value { return Value(l > r); },
                          [](double l, double r) -> Value { return Value(l > r); },
                          [](bool l, bool r) -> Value { return Value(l > r); },
                          [](auto&& l, auto&& r) -> Value {
                              throw std::runtime_error(
                                  "unexpected type pair while evaluating value");
                          },
                      },
                      pr.first.inner, pr.second.inner);
}

Value Value::operator+(const Value& rhs) const {
    auto pr = promote(*this, rhs);
    return std::visit(match{
                          [](int8_t l, int8_t r) -> Value { return Value(l + r); },
                          [](int16_t l, int16_t r) -> Value { return Value(l + r); },
                          [](int32_t l, int32_t r) -> Value { return Value(l + r); },
                          [](int64_t l, int64_t r) -> Value { return Value(l + r); },
                          [](uint8_t l, uint8_t r) -> Value { return Value(l + r); },
                          [](uint16_t l, uint16_t r) -> Value { return Value(l + r); },
                          [](uint32_t l, uint32_t r) -> Value { return Value(l + r); },
                          [](uint64_t l, uint64_t r) -> Value { return Value(l + r); },
                          [](float l, float r) -> Value { return Value(l + r); },
                          [](double l, double r) -> Value { return Value(l + r); },
                          [](bool l, bool r) -> Value { return Value(l + r); },
                          [](auto&& l, auto&& r) -> Value {
                              throw std::runtime_error(
                                  "unexpected type pair while evaluating value");
                          },
                      },
                      pr.first.inner, pr.second.inner);
}

Value Value::operator-(const Value& rhs) const {
    auto pr = promote(*this, rhs);
    return std::visit(match{
                          [](int8_t l, int8_t r) -> Value { return Value(l - r); },
                          [](int16_t l, int16_t r) -> Value { return Value(l - r); },
                          [](int32_t l, int32_t r) -> Value { return Value(l - r); },
                          [](int64_t l, int64_t r) -> Value { return Value(l - r); },
                          [](uint8_t l, uint8_t r) -> Value { return Value(l - r); },
                          [](uint16_t l, uint16_t r) -> Value { return Value(l - r); },
                          [](uint32_t l, uint32_t r) -> Value { return Value(l - r); },
                          [](uint64_t l, uint64_t r) -> Value { return Value(l - r); },
                          [](float l, float r) -> Value { return Value(l - r); },
                          [](bool l, bool r) -> Value { return Value(l - r); },
                          [](auto&& l, auto&& r) -> Value {
                              throw std::runtime_error(
                                  "unexpected type pair while evaluating value");
                          },
                      },
                      pr.first.inner, pr.second.inner);
}

Value Value::operator*(const Value& rhs) const {
    auto pr = promote(*this, rhs);
    return std::visit(match{
                          [](int8_t l, int8_t r) -> Value { return Value(l * r); },
                          [](int16_t l, int16_t r) -> Value { return Value(l * r); },
                          [](int32_t l, int32_t r) -> Value { return Value(l * r); },
                          [](int64_t l, int64_t r) -> Value { return Value(l * r); },
                          [](uint8_t l, uint8_t r) -> Value { return Value(l * r); },
                          [](uint16_t l, uint16_t r) -> Value { return Value(l * r); },
                          [](uint32_t l, uint32_t r) -> Value { return Value(l * r); },
                          [](uint64_t l, uint64_t r) -> Value { return Value(l * r); },
                          [](float l, float r) -> Value { return Value(l * r); },
                          [](double l, double r) -> Value { return Value(l * r); },
                          [](bool l, bool r) -> Value { return Value(l * r); },
                          [](auto&& l, auto&& r) -> Value {
                              throw std::runtime_error(
                                  "unexpected type pair while evaluating value");
                          },
                      },
                      pr.first.inner, pr.second.inner);
}

Value Value::operator/(const Value& rhs) const {
    auto pr = promote(*this, rhs);
    if (rhs == 0) {
        throw InvalidCompileTimeEval("divide by zero");
    }
    return std::visit(match{
                          [](int8_t l, int8_t r) -> Value { return Value(l / r); },
                          [](int16_t l, int16_t r) -> Value { return Value(l / r); },
                          [](int32_t l, int32_t r) -> Value { return Value(l / r); },
                          [](int64_t l, int64_t r) -> Value { return Value(l / r); },
                          [](uint8_t l, uint8_t r) -> Value { return Value(l / r); },
                          [](uint16_t l, uint16_t r) -> Value { return Value(l / r); },
                          [](uint32_t l, uint32_t r) -> Value { return Value(l / r); },
                          [](uint64_t l, uint64_t r) -> Value { return Value(l / r); },
                          [](float l, float r) -> Value { return Value(l / r); },
                          [](double l, double r) -> Value { return Value(l / r); },
                          [](bool l, bool r) -> Value { return Value(l / r); },
                          [](auto&& l, auto&& r) -> Value {
                              throw std::runtime_error(
                                  "unexpected type pair while evaluating value");
                          },
                      },
                      pr.first.inner, pr.second.inner);
}

Value Value::operator~() const {
    return std::visit(
        match{[](int8_t v) { return Value(~v); }, [](int16_t v) { return Value(~v); },
              [](int32_t v) { return Value(~v); }, [](int64_t v) { return Value(~v); },
              [](uint8_t v) { return Value(~v); }, [](uint16_t v) { return Value(~v); },
              [](uint32_t v) { return Value(~v); }, [](uint64_t v) { return Value(~v); },
              [](bool v) { return Value(~v); },
              [](auto&& v) -> Value {
                  throw InvalidCompileTimeEval("invalid value type for bitwise NOT");
              }},
        inner);
}

Value Value::operator-() const {
    return std::visit(match{
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
    return std::visit(match{
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
    return std::visit(match{[](int8_t v) { return v != 0; }, [](int16_t v) { return v != 0; },
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

    if (promoted.first <= promoted.second) {
        throw InvalidValueRange("invalid range, range start must be less than range end");
    }

    this->start = promoted.first;
    // ranges are inclusive, so our end should be end + 1
    this->finish = promoted.second + 1;

    this->curr = this->start;
}