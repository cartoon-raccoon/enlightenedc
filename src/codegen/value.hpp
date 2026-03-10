#ifndef ECC_VALUE_H
#define ECC_VALUE_H

#include <cstdint>
#include <variant>
#include <string>

namespace ecc::exec {

struct StructValue {

};

typedef std::variant<
    std::monostate,
    uint8_t,
    uint16_t,
    uint32_t,
    uint64_t,
    int8_t,
    int16_t,
    int32_t,
    int64_t,
    double,
    bool,
    std::string
> Value;

}

#endif