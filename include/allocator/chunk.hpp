#pragma once

#ifndef ECC_CHUNK_H
#define ECC_CHUNK_H

#include <concepts>
#include <cstddef>
#include <memory>
#include <type_traits>

namespace ecc::alloc {

void *alloc(std::size_t size, std::size_t align);

void register_cleanup(void *obj, void (*destroy)(void *));

/**
A chunk of memory on a slab, owned by a bump-pointer allocator.

`Chunk<T>` expresses single ownership of an arena-allocated T. It is move-only;
copying is forbidden so there is exactly one owner. Destroying a Chunk does not run
`~T()` or free memory: the backing BumpAllocator owns the storage and runs `~T()` for
all live objects at `reset()`. Chunk is a compile-time discipline, not a runtime
resource guard.
*/
template <typename T>
class Chunk {

    T *ptr = nullptr;

public:
    Chunk() noexcept = default;

    Chunk(std::nullptr_t) noexcept {}

    explicit Chunk(T *ptr) : ptr(ptr) {}

    /**
    Move a heap-owned `std::unique_ptr<U>` into an arena-owned `Chunk<T>`.

    Note: THIS IS AN EXPENSIVE OPERATION. It is *not* a pointer steal. Constructing
    a Chunk from a unique pointer involves allocating space in the arena and move-constructing
    a new `U` into it, which might additionally involve a slab allocation.
    */
    template <typename U>
        requires std::convertible_to<U *, T *>
    explicit Chunk(std::unique_ptr<U> box) {
        void *mem = alloc(sizeof(U), alignof(U));
        U *obj    = ::new (mem) U(std::move(*box));
        if constexpr (!std::is_trivially_destructible_v<U>) {
            register_cleanup(obj, +[](void *p) { static_cast<U *>(p)->~U(); });
        }
        ptr = obj;
    }

    Chunk(const Chunk&) = delete;

    Chunk(Chunk&& chunk) noexcept {
        ptr       = chunk.ptr;
        chunk.ptr = nullptr;
    }

    template <typename U>
        requires std::convertible_to<U *, T *>
    Chunk(Chunk<U>&& other) noexcept : ptr(other.release()) {}

    Chunk& operator=(const Chunk&) = delete;

    /**
    Assign from a `Box<U>`. Same cost and ownership rules as the converting constructor.
    */
    template <typename U>
        requires std::convertible_to<U *, T *>
    Chunk& operator=(std::unique_ptr<U> box) {
        ptr = Chunk(std::move(box)).release();
        return *this;
    }

    Chunk& operator=(Chunk&& chunk) noexcept {
        ptr = chunk.ptr;
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

    /**
    Get a pointer to the underlying object.
    */
    T *get() { return ptr; }

    /**
    Get a pointer to the underlying object.
    */
    const T *get() const { return ptr; }

    /**
    Releases the chunk's ownership of the object.
    */
    T *release() {
        T *tmp = ptr;
        ptr    = nullptr;

        return tmp;
    }

    /**
    Swap two `Chunk`s' underlying pointers.
    */
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

} // namespace ecc::alloc

#endif