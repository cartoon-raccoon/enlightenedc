#pragma once

#ifndef ECC_CHUNK_H
#define ECC_CHUNK_H

#include <cstddef>
#include <concepts>

namespace ecc::alloc {

/**
A chunk of memory on a slab, owned by a bump-pointer allocator.

`Chunk<T>` expresses single ownership of an arena-allocated T.
It is move-only; copying is forbidden so there is exactly one owner.
Destroying a Chunk does not run `~T()` or free memory: the backing
BumpAllocator owns the storage and runs `~T()` for all live objects
at `reset()`. Chunk is a compile-time discipline, not a runtime resource guard.
*/
template <typename T>
class Chunk {

    T *ptr = nullptr;

public:
    Chunk() noexcept = default;

    Chunk(std::nullptr_t) noexcept {}

    explicit Chunk(T *ptr) : ptr(ptr) {}

    Chunk(const Chunk&) = delete;

    Chunk(Chunk&& chunk) noexcept {
        ptr       = chunk.ptr;
        chunk.ptr = nullptr;
    }

    template <typename U>
        requires std::convertible_to<U *, T *>
    Chunk(Chunk<U>&& other) noexcept : ptr(other.release()) {}

    Chunk& operator=(const Chunk&) = delete;

    Chunk& operator=(Chunk&& chunk) noexcept {
        ptr       = chunk.ptr;
        if (this != &chunk) {
            chunk.ptr = nullptr;
        }

        return *this;
    }

    template <typename U>
        requires std::convertible_to<U *, T *>
    Chunk& operator=(Chunk<U>&& other) noexcept {
        ptr = other.release();
        return *this;
    }

    ~Chunk() noexcept = default;

    T *get() { return ptr; }

    const T *get() const { return ptr; }

    T *release() {
        T *tmp = ptr;
        ptr = nullptr;

        return tmp;
    }

    void swap(Chunk& other) noexcept {
        T *tmp    = ptr;
        ptr       = other.ptr;
        other.ptr = tmp;
    }

    T& operator*() const { return *ptr; }

    T *operator->() const { return ptr; }

    bool operator==(const Chunk& other) const { return ptr == other.ptr; }

    bool operator==(std::nullptr_t) const { return ptr == nullptr; }

    explicit operator bool() const { return ptr != nullptr; }
};

}

#endif