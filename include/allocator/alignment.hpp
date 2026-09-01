#pragma once

#ifndef ECC_ALIGNMENT_H
#define ECC_ALIGNMENT_H

#include <cstddef>
#include <cstdint>

namespace ecc::util {

inline bool is_aligned(size_t alignment, size_t value) {
    return value % alignment == 0;
}

inline bool is_addr_aligned(size_t alignment, const void *addr) {
    return is_aligned(alignment, reinterpret_cast<uintptr_t>(addr));
}

constexpr size_t align_to(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1U);
}

inline uintptr_t align_addr(const void *addr, size_t alignment) {
    auto arith_addr = reinterpret_cast<uintptr_t>(addr);

    return align_to(arith_addr, alignment);
}

inline bool is_power_of_2(size_t val) {
    return (val & (val - 1)) == 0;
}

} // namespace ecc::util

#endif