#pragma once

#ifndef ECC_UTIL_MATH_H
#define ECC_UTIL_MATH_H

#include <cstdint>

namespace ecc::util {

constexpr bool is_power_of_2(size_t n) {
    return (n & (n - 1)) == 0;
}

constexpr uint32_t next_power_of_2(uint32_t n) {
    n--;

    n |= (n >> 1);
    n |= (n >> 2);
    n |= (n >> 4);
    n |= (n >> 8);
    n |= (n >> 16);

    return n;
}

constexpr int32_t next_power_of_2(int32_t n) {
    if (n <= 1) return 1;

    return static_cast<int32_t>(next_power_of_2(static_cast<uint32_t>(n)));

}

constexpr uint64_t next_power_of_2(uint64_t n) {
    n--;

    n |= (n >> 1);
    n |= (n >> 2);
    n |= (n >> 4);
    n |= (n >> 8);
    n |= (n >> 16);
    n |= (n >> 32);

    return n;
}

constexpr int64_t next_power_of_2(int64_t n) {
    if (n <= 1) return 1;

    return static_cast<int32_t>(next_power_of_2(static_cast<uint64_t>(n)));
}

}

#endif