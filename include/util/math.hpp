#pragma once

#ifndef ECC_UTIL_MATH_H
#define ECC_UTIL_MATH_H

namespace ecc::util {

constexpr bool is_power_of_2(size_t val) {
    return (val & (val - 1)) == 0;
}

}

#endif