#pragma once

#ifndef ECC_ARENAVEC_H
#define ECC_ARENAVEC_H

#include <memory>
#include <type_traits>

#include "allocator/alloc.hpp"

namespace ecc::ds {

template <typename T, size_t N = 8, bool = std::is_trivially_destructible_v<T>>
class ArenaVecBase {
protected:
    using AllocTraits = std::allocator_traits<alloc::ArenaAllocator<T>>;

    T *data     = nullptr;
    size_t size = 0;
    size_t cap  = 0;
    ArenaAllocator<T> alloc;

    ArenaVecBase() = default;
    ~ArenaVecBase() { std::destroy_n(data, size); }
};

template <typename T, size_t N>
class ArenaVecBase<T, N, true> {
protected:
    using AllocTraits = std::allocator_traits<alloc::ArenaAllocator<T>>;

    T *data    = nullptr;
    size_t len = 0;
    size_t cap = 0;
    ArenaAllocator<T> alloc;
};

/**
A vector that preallocates to the arena
*/
template <typename T, size_t N = 8>
class ArenaVec : ArenaVecBase<T, N> {
public:
};

} // namespace ecc::ds

#endif