#pragma once

#ifndef ECC_ALIGNMENT_H
#define ECC_ALIGNMENT_H

#include <cstddef>
#include <cstdint>

namespace ecc::util {

constexpr bool is_aligned(size_t alignment, size_t value) {
    return value % alignment == 0;
}

constexpr bool is_addr_aligned(size_t alignment, const void *addr) {
    return is_aligned(alignment, reinterpret_cast<uintptr_t>(addr));
}

constexpr size_t align_to(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1U);
}

constexpr uintptr_t align_addr(const void *addr, size_t alignment) {
    auto arith_addr = reinterpret_cast<uintptr_t>(addr);

    return align_to(arith_addr, alignment);
}

} // namespace ecc::util

#endif