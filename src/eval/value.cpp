#include "eval/value.hpp"
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

    case P::F64:
        return cast<double>();
    }
}

Pair<Value, Value> Value::promote(const Value& lhs, const Value& rhs) {
    PrimType promoted = pr_promote(lhs.primtype, rhs.primtype);

    return {lhs.pr_cast(promoted), rhs.pr_cast(promoted)};
}

Value Value::operator|(const Value& rhs) const {
    auto pr = promote(*this, rhs);
    todo();
}

Value Value::operator^(const Value& rhs) const {
    auto pr = promote(*this, rhs);
    todo();
}

Value Value::operator&(const Value& rhs) const {
    auto pr = promote(*this, rhs);
    todo();
}

Value Value::operator==(const Value& rhs) const {
    auto pr = promote(*this, rhs);
    todo();
}

Value Value::operator<(const Value& rhs) const {
    auto pr = promote(*this, rhs);
    todo();
}

Value Value::operator>(const Value& rhs) const {
    auto pr = promote(*this, rhs);
    todo();
}

Value Value::operator+(const Value& rhs) const {
    auto pr = promote(*this, rhs);
    todo();
}

Value Value::operator-(const Value& rhs) const {
    auto pr = promote(*this, rhs);
    todo();
}

Value Value::operator*(const Value& rhs) const {
    auto pr = promote(*this, rhs);
    todo();
}

Value Value::operator/(const Value& rhs) const {
    auto pr = promote(*this, rhs);
    todo();
}

Value Value::operator~() const {
    todo();
}

Value::operator bool() const  {
    return std::visit(match{[](int8_t v) { return v != 0; }, 
                            [](int16_t v) { return v != 0; },
                            [](int32_t v) { return v != 0; }, 
                            [](int64_t v) { return v != 0; }, 
                            [](uint8_t v) { return v != 0; }, 
                            [](uint16_t v) { return v != 0; }, 
                            [](uint32_t v) { return v != 0; }, 
                            [](uint64_t v) { return v != 0; }, 
                            [](double v) { return v != 0.0; }, 
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