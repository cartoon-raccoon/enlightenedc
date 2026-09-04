#pragma once

#ifndef ECC_ARENAVEC_H
#define ECC_ARENAVEC_H

#include <algorithm>
#include <cassert>
#include <compare>
#include <initializer_list>
#include <memory>
#include <limits>
#include <type_traits>
#include <utility>

#include "allocator/alloc.hpp"

namespace ecc::ds {

template <typename T, size_t N = 8, bool = std::is_trivially_destructible_v<T>>
class ArenaVecBase {
protected:
    using AllocTraits = std::allocator_traits<alloc::ArenaAllocator<T>>;

    T *data     = nullptr;
    size_t len = 0;
    size_t cap  = 0;
    ArenaAllocator<T> alloc;

    ArenaVecBase() = default;
    ~ArenaVecBase() { std::destroy_n(data, len); }
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

template <typename T, size_t N>
class ArenaVec;

template <typename T>
class ArenaVecIter {
    template <typename U, size_t N>
    friend class ArenaVec;

    T *data;
    size_t idx;
    size_t len;

    ArenaVecIter(T *data, size_t idx, size_t len)
        : data(data), idx(idx), len(len) {}

public:
    ArenaVecIter(const ArenaVecIter& iter) {
        data = iter.data;
        idx = iter.idx;
        len = iter.len;
    }

    ArenaVecIter(ArenaVecIter&& iter) noexcept {
        data = iter.data;
        idx = iter.idx;
        len = iter.len;

        iter.data = nullptr;
        iter.idx = 0;
        iter.len = 0;
    }

    ArenaVecIter& operator=(const ArenaVecIter& iter) {
        if (this == &iter) {
            return *this;
        }

        this->data = iter.data;
        this->idx = iter.idx;
        this->len = iter.len;

        return *this;
    }

    ArenaVecIter& operator=(ArenaVecIter&& iter) noexcept {
        this->data = iter.data;
        this->idx = iter.idx;
        this->len = iter.len;

        return *this;
    }

    bool operator==(const ArenaVecIter& other) const {
        return idx == other.idx;
    }

    std::strong_ordering operator<=>(const ArenaVecIter& other) const {
        return idx <=> other.idx;
    }

    T& operator*() {
        return *(data + idx);
    }

    ArenaVecIter& operator++() {
        if (idx == len) {
            return *this;
        }

        ++idx;
        return *this;
    }

    ArenaVecIter operator++(int) {
        ArenaVecIter temp = *this;

        if (idx < len) {
            idx++;
        }

        return temp;
    }

    ArenaVecIter& operator--() {
        if (idx <= 0) {
            return *this;
        }

        --idx;
        return *this;
    }

    ArenaVecIter operator--(int) {
        ArenaVecIter temp = *this;

        if (idx > 0) {
            idx--;
        }

        return temp;
    }

};

/**
A vector that preallocates to the arena enough space for *at least* `N` items.

If `reserve` is called with a size greater than `N`, that size is used instead.
*/
template <typename T, size_t N = 8>
class ArenaVec : ArenaVecBase<T, N> {
public:
    using Iterator = ArenaVecIter<T>;

    ArenaVec() : ArenaVecBase<T, N>() {}

    explicit ArenaVec(size_t size) : ArenaVecBase<T, N>() {
        reserve(size);
    }

    ArenaVec(const ArenaVec& vec) : ArenaVecBase<T, N>() {
        this->data = this->alloc.allocate(vec.cap);
        std::uninitialized_copy_n(vec.data, vec.len, this->data);
        this->len = vec.len;
        this->cap = vec.cap;
    }

    ArenaVec(ArenaVec&& vec) noexcept : ArenaVecBase<T, N>() {
        this->data = vec.data;
        this->len = vec.len;
        this->cap = vec.cap;
        // allocator is stateless, so no need to copy it
        // (there is no state to copy)

        vec.data = nullptr;
        vec.len = 0;
        vec.cap = 0;
    }

    ArenaVec(std::initializer_list<T> inits) : ArenaVecBase<T, N>() {
        reserve(inits.size());

        for (auto& item : inits) {
            emplace_back(item);
        }
    }

    ArenaVec& operator=(const ArenaVec& vec) {
        if (this == &vec) {
            return *this;
        }

        // destroy old objects before this->data is potentially replaced below
        if constexpr (!std::is_trivially_destructible_v<T>) {
            std::destroy_n(this->data, this->len);
        }

        if (vec.len > this->cap) {
            // if we can hold fewer objects than the incoming has,
            // reallocate for enough space
            this->data = this->alloc.allocate(vec.len);
            this->cap = vec.len;
        }

        // nothing to deallocate - just leave our old data dangling

        std::uninitialized_copy_n(vec.data, vec.len, this->data);
        this->len = vec.len;

        return *this;
    }

    ArenaVec& operator=(ArenaVec&& vec) noexcept {
        std::destroy_n(this->data, this->len);
        this->data = vec.data;
        this->len = vec.len;
        this->cap = vec.cap;

        vec.data = nullptr;
        vec.len = 0;
        vec.cap = 0;

        return *this;
    }

    T& front() { return *(this->data); }

    T& back() { return *(this->data + (this->len - 1)); }

    Iterator begin() { return Iterator(this->data, 0, this->len); }

    Iterator end() { return Iterator(this->data, this->len, this->len); }

    bool empty() const { return this->len == 0; }

    size_t size() const { return this->len; }

    size_t max_size() const { return (std::numeric_limits<size_t>::max() / sizeof(T)); }

    void reserve(size_t size = N) {
        if (this->data != nullptr) return;

        if (size <= N) {
            this->data = this->alloc.allocate(N);
            this->cap = N;
        } else {
            this->data = this->alloc.allocate(size);
            this->cap = size;
        }
    }

    size_t capacity() const { return this->cap; }

    static constexpr size_t initial_cap() { return N; }

    void clear() {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            std::destroy_n(this->data, this->len);
        }
        this->len = 0;
    }

    void push_back(const T& item) {
        emplace_back(item);
    }

    void push_back(T&& item) {
        emplace_back(std::move(item));
    }

    template <typename ... Args>
    T& emplace_back(Args&& ... args) {
        if (this->len == this->cap) {
            grow();
        }
        T *point = this->data + this->len;
        T *obj = ::new (point) T(std::forward<Args>(args) ...);
        ++this->len;

        return *obj;
    }

    void pop_back() {
        std::destroy_at(this->data + (this->len - 1));
        --this->len;
    }

    void swap(ArenaVec& other) {
        T *data_temp = other.data;
        other.data = this->data;
        this->data = data_temp;

        if (this->len != other.len) {
            size_t len_temp = other.len;
            other.len = this->len;
            this->len = len_temp;
        }

        if (this->cap != other.cap) {
            size_t cap_temp = other.cap;
            other.cap = this->cap;
            this->cap = cap_temp;
        }
    }

private:

    void grow() {
        size_t new_cap;
        if (this->cap == 0) {
            assert(this->len == 0);
            new_cap = N;
        } else {
            new_cap = this->cap * 2;
        }
        T *my_data = this->data;
        this->data = this->alloc.allocate(new_cap);
        this->cap = new_cap;

        std::uninitialized_copy_n(my_data, this->len, this->data);

        // destroy old data *after* copying
        std::destroy_n(my_data, this->len);
    }
};

} // namespace ecc::ds

#endif