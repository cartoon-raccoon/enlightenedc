#pragma once

#ifndef ECC_ALLOC_H
#define ECC_ALLOC_H

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

#include "allocator/alignment.hpp"
#include "allocator/chunk.hpp"
#include "util.hpp"

namespace ecc::alloc {

using namespace ecc::util;

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
    size_t num_slabs = 0;
    size_t num_custom_slabs = 0;
    size_t total_used_bytes = 0;
    size_t total_allocated_bytes = 0;
    size_t current_slab_size = 0;

    void reset() {
        num_slabs = num_custom_slabs 
                  = total_used_bytes
                  = total_allocated_bytes
                  = 0;
    }
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
pool of memory.
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
    Vec<Slab> slabs;

    /**
    Custom slabs for oversized objects.
    */
    Vec<Slab> custom_slabs;

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

    size_t num_slabs() { return slabs.size(); }

    template <class T, typename... Args>
    T *create(Args&&...args) {
        void *mem = alloc(sizeof(T), alignof(T));
        T *obj    = ::new (mem) T(std::forward<Args>(args)...);

        if constexpr (!std::is_trivially_destructible_v<T>) {
            // if T has a non-trivial destructor, link it into the cleanup chain
            auto *cleanuprec    = static_cast<Cleanup *>(alloc(sizeof(Cleanup), alignof(Cleanup)));
            cleanuprec->obj     = obj;
            cleanuprec->destroy = +[](void *p) { static_cast<T *>(p)->~T(); };
            cleanuprec->next    = cleanup_head;
            cleanup_head        = cleanuprec;
        }

        return obj;
    }

    void *alloc(size_t size, size_t align) {
        if (size > SizeThreshold) {
            return alloc_custom(size);
        }

        size_t align_to_use = std::max(align, MinAlign);
        if (cur != nullptr) {
            auto raw        = align_addr(cur, align_to_use);
            uint8_t *result = reinterpret_cast<uint8_t *>(raw);
            if (result + size <= end) {

#ifndef NDEBUG
                stats.total_used_bytes += size;
#endif

                // bump the pointer
                cur = result + size;
                return result;
            }
        }
        return grow(size, align);
    }

    /**
    Allocate a custom sized slab.
    */
    void *alloc_custom(size_t size) {
        assert(size > SizeThreshold && "custom slab for size under threshold");

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
    void print_stats() {
        std::cerr << stats;
    }
#endif

private:
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

inline void *alloc(size_t size, size_t align) {
    return detail::instance().alloc(size, align);
}

inline void reset() {
    detail::instance().reset();
}

#ifndef NDEBUG
inline void print_allocator_stats() {
    detail::instance().print_stats();
}
#endif

template <typename T, typename... Args>
Chunk<T> make_chunk(Args&&...args) {
    T *obj = detail::instance().create<T>(std::forward<Args>(args)...);
    return Chunk<T>(obj);
}

} // namespace ecc::alloc

#endif