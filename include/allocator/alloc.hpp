#pragma once

#ifndef ECC_ALLOC_H
#define ECC_ALLOC_H

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <new>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "allocator/alignment.hpp"
#include "allocator/chunk.hpp"
#include "util/assert.hpp"
#include "util/math.hpp"

namespace ecc::alloc {

using namespace ecc::util;

/**
An allocator generic over some type `T` that allocates copies of `T`.
*/
template <typename A, typename T>
concept GenericAllocator = requires(A& a, size_t n, T *ptr) {
    { a.allocate(n) } -> std::convertible_to<T *>;
    { a.deallocate(ptr, n) } noexcept;
};


/**
An allocator that allocates bytes.

The `allocate` method on a `ByteAllocator` takes an alignment argument. Note that
this concept does not place constraints on the alignment accepted by the allocator,
and each allocator is free to place its own limitations on the accepted alignment.

Note that `ByteAllocators` are not STL-compatible.
*/
template <typename A>
concept ByteAllocator = requires(A& a, size_t n, size_t align, void *ptr) {
    { a.allocate(n, align) } -> std::same_as<void *>;
    { a.deallocate(ptr, n, align) } noexcept;
};

class Slab {
    uint8_t *ptr = nullptr;
    size_t size  = 0;

public:
    Slab() = default;

    Slab(size_t size);

    Slab(const Slab&) = delete;

    Slab(Slab&& slab) noexcept : ptr(slab.ptr), size(slab.size) {
        slab.ptr  = nullptr;
        slab.size = 0;
    }

    ~Slab() noexcept;

    uint8_t *start() const { return ptr; }

    uint8_t *end() const { return ptr + size; }

    void initialize(size_t size);

    void clear();

    bool is_clear() { return ptr == nullptr; }
};

#ifndef NDEBUG

struct AllocatorStats {
    size_t num_slabs             = 0;
    size_t num_custom_slabs      = 0;
    size_t total_used_bytes      = 0;
    size_t total_allocated_bytes = 0;
    size_t current_slab_size     = 0;

    void reset() { num_slabs = num_custom_slabs = total_used_bytes = total_allocated_bytes = 0; }
};

template <typename T>
std::basic_ostream<T>& operator<<(std::basic_ostream<T>& ostr, const AllocatorStats& stats) {
    ostr << "===== Bump-Pointer Allocator Stats =====\n";
    ostr << "  # of Slabs:         " << stats.num_slabs << "\n";
    ostr << "  # of Custom Slabs:  " << stats.num_custom_slabs << "\n";
    ostr << "  Total Used Bytes:   " << stats.total_used_bytes << "\n";
    ostr << "  Total Alloc'd Byes: " << stats.total_allocated_bytes << "\n";
    ostr << "  Current Slab Size:  " << stats.current_slab_size << "\n";

    return ostr;
}

#endif

struct Cleanup {
    void *obj;
    void (*destroy)(void *);
    Cleanup *next;
};

/**
A bump-pointer allocator that allocates from a monotonically-increasing
pool of memory. Satisfies ByteAllocator.
*/
template <
    size_t SlabSize = 4096, size_t GrowthDelay = 8, size_t SizeThreshold = SlabSize,
    size_t MinAlign = 8>
class BumpAllocator {
    static constexpr size_t SLAB_SATURATE = 30;

    static_assert(
        SizeThreshold <= SlabSize, "The SizeThreshold must be at most the SlabSize to ensure "
                                   "that objects larger than a slab go into their own memory "
                                   "allocation.");
    static_assert(
        GrowthDelay > 0, "GrowthDelay must be at least 1 which already increases the"
                         "slab size after each allocated slab.");
    static_assert(
        MinAlign > 0 && (MinAlign & (MinAlign - 1)) == 0, "MinAlign must be a power of two");
    static_assert(
        MinAlign <= alignof(std::max_align_t),
        "MinAlign must not exceed the alignment of fresh slabs");

    /**
    A pointer to the start of the free space of the current slab.
    */
    uint8_t *cur = nullptr;

    /**
    One past the end of the current slab.
    */
    uint8_t *end = nullptr;

    /**
    Slabs storing the data.
    */
    std::vector<Slab> slabs;

    /**
    Custom slabs for oversized objects.
    */
    std::vector<Slab> custom_slabs;

    Cleanup *cleanup_head = nullptr;

#ifndef NDEBUG
    AllocatorStats stats;
#endif

public:
    BumpAllocator() = default;

    ~BumpAllocator() {
        for (Cleanup *c = cleanup_head; c != nullptr; c = c->next) {
            c->destroy(c->obj);
        }
        cleanup_head = nullptr;

        slabs.clear();
        custom_slabs.clear();

        cur = end = nullptr;
    }

    [[nodiscard]] size_t num_slabs() { return slabs.size(); }

    template <class T, typename... Args>
    [[nodiscard]] T *create(Args&&...args) {
        void *mem = allocate(sizeof(T), alignof(T));
        T *obj    = ::new (mem) T(std::forward<Args>(args)...);

        if constexpr (!std::is_trivially_destructible_v<T>) {
            // if T has a non-trivial destructor, link it into the cleanup chain
            auto *cleanuprec    = static_cast<Cleanup *>(allocate(sizeof(Cleanup), alignof(Cleanup)));
            cleanuprec->obj     = obj;
            cleanuprec->destroy = +[](void *p) { static_cast<T *>(p)->~T(); };
            cleanuprec->next    = cleanup_head;
            cleanup_head        = cleanuprec;
        }

        return obj;
    }

    void register_cleanup(void *obj, void (*destroy)(void *)) {
        auto *cleanuprec    = static_cast<Cleanup *>(allocate(sizeof(Cleanup), alignof(Cleanup)));
        cleanuprec->obj     = obj;
        cleanuprec->destroy = destroy;
        cleanuprec->next    = cleanup_head;
        cleanup_head        = cleanuprec;
    }

    /**
    Allocates `bytes` in the arena, aligned to *at least* `align`.
    */
    [[nodiscard]]
    void *allocate(size_t n, size_t align) {
        ECC_ASSERT(is_power_of_2(align), "alignment is not power of 2");
        ECC_ASSERT(align <= alignof(std::max_align_t), "alignment exceeds max alignment");

        if (n > SizeThreshold) {
            return allocate_custom(n);
        }

        size_t align_to_use = std::max(align, MinAlign);
        if (cur != nullptr) {
            auto raw        = align_addr(cur, align_to_use);
            uint8_t *result = reinterpret_cast<uint8_t *>(raw);
            if (result + n <= end) {

#ifndef NDEBUG
                stats.total_used_bytes += n;
#endif

                // bump the pointer
                cur = result + n;
                return result;
            }
        }
        return grow(n, align);
    }

    /**
    On a bump-pointer allocator, deallocation is a no-op.
    */
    void deallocate(
        [[maybe_unused]] void *ptr, 
        [[maybe_unused]] size_t n, [[maybe_unused]] size_t align) noexcept {}

    /**
    Deallocate all but the first slab, and clear all custom-sized slabs.
    */
    void reset() {
        // walk the cleanup list,
        for (Cleanup *c = cleanup_head; c != nullptr; c = c->next) {
            c->destroy(c->obj);
        }
        cleanup_head = nullptr;

        custom_slabs.clear();

        for (size_t idx = 1; idx < slabs.size(); idx++) {
            slabs[idx].clear();
        }

        if (!slabs.empty()) {
            slabs.resize(1);
            slabs[0].initialize(compute_slab_size());
            cur = slabs[0].start();
            end = slabs[0].end();
        } else {
            cur = end = nullptr;
        }
#ifndef NDEBUG
        stats.reset();
#endif
    }

#ifndef NDEBUG
    void print_stats() { std::cerr << stats; }
#endif

private:
    /**
    Allocate a custom sized slab.
    */
    [[nodiscard]]
    void *allocate_custom(size_t size) {
        ECC_ASSERT(size > SizeThreshold, "custom slab for size under threshold");

        Slab slab(size);

#ifndef NDEBUG
        stats.num_custom_slabs += 1;
        stats.total_used_bytes += size;
        stats.total_allocated_bytes += size;
#endif

        void *ret = slab.start();

        custom_slabs.push_back(std::move(slab));

        return ret;
    }

    /**
    Allocate a new slab.
    */
    void *grow(size_t size, size_t align) {
        size_t slab_size = compute_slab_size();

        Slab slab(slab_size);

#ifndef NDEBUG
        stats.num_slabs += 1;
        stats.total_allocated_bytes += slab_size;
        stats.current_slab_size = slab_size;
#endif

        cur = slab.start();
        end = slab.end();

        slabs.push_back(std::move(slab));

        size_t align_to_use = std::max(align, MinAlign);
        auto raw            = align_addr(cur, align_to_use);
        uint8_t *result     = reinterpret_cast<uint8_t *>(raw);

#ifndef NDEBUG
        stats.total_used_bytes += size;
#endif

        cur = result + size;

        return result;
    }

    size_t compute_slab_size() {
        // Scale the actual allocated slab size based on the number of slabs
        // allocated. Every GrowthDelay slabs allocated, we double
        // the allocated size to reduce allocation frequency, but saturate at
        // multiplying the slab size by 2^30.
        return SlabSize * ((size_t)1) << std::min<size_t>(SLAB_SATURATE, num_slabs() / GrowthDelay);
    }
};

namespace detail {
inline BumpAllocator<>& instance() {
    static BumpAllocator<> a;
    return a;
}
} // namespace detail

[[nodiscard]] inline void *alloc(size_t size, size_t align) {
    return detail::instance().allocate(size, align);
}

/**
Create a new instance of `T`, owned by the arena. Returns a pointer to the created instance.
*/
template <typename T, typename... Args>
[[nodiscard]] inline T *create(Args&&...args) {
    return detail::instance().create<T>(std::forward<Args>(args)...);
}

/**
Resets the global arena.

SAFETY: after this is run, any existing pointers and chunks into the arena will be left dangling.
It is extremely important to ensure that any object holding a reference into the arena does not
outlive the arena itself, and if it does, it must not dereference that reference.
*/
inline void reset() {
    detail::instance().reset();
}

/**
Registers a cleanup function for `obj`. Usually this will be a function that calls `obj`'s dtor.

Note: You should not need to call this yourself. If you find yourself reaching for this, step
back and strongly consider if you really need to use it.
*/
inline void register_cleanup(void *obj, void (*destroy)(void *)) {
    detail::instance().register_cleanup(obj, destroy);
}

/**
Prints global allocator stats. Is a no-op if NDEBUG is defined.
*/
inline void print_allocator_stats() {
#ifndef NDEBUG
    detail::instance().print_stats();
#endif
}

/**
Create a `Chunk<T>` using the global allocator.
*/
template <typename T, typename... Args>
[[nodiscard]] Chunk<T> make_chunk(Args&&...args) {
    T *obj = detail::instance().create<T>(std::forward<Args>(args)...);
    return Chunk<T>(obj);
}

/**
Create a `Chunk<T>` from a `unique_ptr<T>`.

Note: THIS IS AN EXPENSIVE OPERATION. Constructing a Chunk from a Box involves allocating memory
in the arena, and then move constructing the object into that memory. This might also involve
a new slab allocation, if the arena needs to grow.
*/
template <typename T>
[[nodiscard]] Chunk<T> make_chunk(std::unique_ptr<T> box) {
    return make_chunk<T>(std::move(*box));
}

/**
Create a `Chunk<T>`, where the memory is owned by the provided `allocator`.
*/
template <typename T, typename... Args>
[[nodiscard]] Chunk<T> make_chunk(BumpAllocator<>& allocator, Args&&...args) {
    T *obj = allocator.create<T>(std::forward<Args>(args)...);
    return Chunk<T>(obj);
}


/**
A generic allocator that allocates on the arena, instead of directly on the heap.
Satisfies `GenericAllocator`.

Note that `deallocate` is a no-op; this allocator allocates on the arena, whose
memory is only reclaimed after the entire arena is reset. When the arena is reset,
every allocation is thus invalidated, and any pointers into it will be left dangling.
*/
template <typename T>
class ArenaAllocator {
public:
    using value_type      = T;
    using size_type       = size_t;
    using difference_type = std::ptrdiff_t;

    using is_always_equal = std::true_type;

    static_assert(
        alignof(T) <= alignof(std::max_align_t),
        "alignment of T must be less than that of std::max_align_t");
    static_assert(!std::is_const_v<T>, "cannot allocate a const type");

    ArenaAllocator() noexcept = default;

    template <class U>
    ArenaAllocator(const ArenaAllocator<U>&) noexcept {}

    /**
    Allocate space on the arena for `n` *copies* of T.
    */
    [[nodiscard]]
    T *allocate(size_t n) {
        if (n > std::numeric_limits<size_t>::max() / sizeof(T)) {
            throw std::bad_array_new_length();
        }

        if (n == 0)
            return nullptr;

        return static_cast<T *>(detail::instance().allocate(n * sizeof(T), alignof(T)));
    }

    void deallocate(T *, [[maybe_unused]] size_t n) noexcept {}

    friend bool operator==(const ArenaAllocator&, const ArenaAllocator&) noexcept { return true; }
};

/**
A generic allocator that allocates on the heap. Satisfies the concept `GenericAllocator`.
*/
template <typename T>
class Allocator {
public:
    using value_type      = T;
    using size_type       = size_t;
    using difference_type = std::ptrdiff_t;

    using is_always_equal = std::true_type;

    static_assert(
        alignof(T) <= alignof(std::max_align_t),
        "alignment of T must be less than that of std::max_align_t");
    static_assert(!std::is_const_v<T>, "cannot allocate a const type");

    Allocator() noexcept = default;

    template <class U>
    Allocator(const Allocator<U>&) noexcept {}

    /**
    Allocate space on the heap for `n` *copies* of T.
    */
    [[nodiscard]]
    T *allocate(size_t n) {
        if (n > std::numeric_limits<size_t>::max() / sizeof(T)) {
            throw std::bad_array_new_length();
        }

        if (n == 0)
            return nullptr;

        return static_cast<T *>(::operator new(n * sizeof(T), std::align_val_t(alignof(T))));
    }

    void deallocate(T *ptr, size_t n) noexcept {
        ::operator delete(ptr, n * sizeof(T), std::align_val_t(alignof(T)));
    }

    friend bool operator==(const Allocator&, const Allocator&) noexcept { return true; }
};

/**
An allocator that calls malloc. It allocates bytes, not copies of an object.
Satisfies `ByteAllocator`.
*/
class MallocAllocator {
public:
    /**
    Allocate space on the heap for `n` *bytes*, on alignment `align`.
    */
    [[nodiscard]]
    void *allocate(size_t n, size_t align) {
        ECC_ASSERT(is_power_of_2(align), "alignment is not power of 2");

        if (n == 0)
            return nullptr;

        return ::operator new(n, std::align_val_t(align));
    }

    void deallocate(void *ptr, size_t n, size_t align) noexcept {
        ::operator delete(ptr, n, std::align_val_t(align));
    }
};

static_assert(ByteAllocator<BumpAllocator<>>);
static_assert(ByteAllocator<MallocAllocator>);
static_assert(GenericAllocator<ArenaAllocator<int>, int>);
static_assert(GenericAllocator<Allocator<int>, int>);

} // namespace ecc::alloc

#endif