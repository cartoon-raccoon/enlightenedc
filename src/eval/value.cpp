#include "eval/value.hpp"

using namespace ecc::eval;

ValueRange::ValueRange(Value start, Value end) {
    if (!start.is<long>() || !end.is<long>()) {
        throw InvalidValueRange("invalid range, types must be integers");
    }

    this->start = *start.value_as<long>();
    // ranges are inclusive, so our end should be end + 1
    this->finish = *end.value_as<long>() + 1;

    this->curr = this->start;
}