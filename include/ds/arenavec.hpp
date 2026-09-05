#pragma once

#ifndef ECC_ARENAVEC_H
#define ECC_ARENAVEC_H

#include <algorithm>
#include <cassert>
#include <compare>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "allocator/alloc.hpp"

namespace ecc::ds {

template <typename T, size_t N = 8, bool = std::is_trivially_destructible_v<T>>
class ArenaVecBase {
protected:
    using AllocTraits = std::allocator_traits<alloc::ArenaAllocator<T>>;

    T *ptr     = nullptr;
    size_t len = 0;
    size_t cap  = 0;
    ArenaAllocator<T> alloc;

    ArenaVecBase() = default;
    ~ArenaVecBase() { std::destroy_n(ptr, len); }
};

template <typename T, size_t N>
class ArenaVecBase<T, N, true> {
protected:
    using AllocTraits = std::allocator_traits<alloc::ArenaAllocator<T>>;

    T *ptr    = nullptr;
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

    using difference_type   = std::ptrdiff_t;
    using value_type        = std::remove_const_t<T>;
    using pointer            = T *;
    using reference          = T&;
    using iterator_category  = std::bidirectional_iterator_tag;

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
    using ConstIterator = ArenaVecIter<const T>;
    using ReverseIter = std::reverse_iterator<Iterator>;

    ArenaVec() : ArenaVecBase<T, N>() {}

    explicit ArenaVec(size_t size) : ArenaVecBase<T, N>() {
        reserve(size);
    }

    ArenaVec(const ArenaVec& vec) : ArenaVecBase<T, N>() {
        this->ptr = allocate(vec.cap);
        std::uninitialized_copy_n(vec.ptr, vec.len, this->ptr);
        this->len = vec.len;
        this->cap = vec.cap;
    }

    ArenaVec(ArenaVec&& vec) noexcept : ArenaVecBase<T, N>() {
        this->ptr = vec.ptr;
        this->len = vec.len;
        this->cap = vec.cap;
        // allocator is stateless, so no need to copy it
        // (there is no state to copy)

        vec.ptr = nullptr;
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
            std::destroy_n(this->ptr, this->len);
        }

        if (vec.len > this->cap) {
            // if we can hold fewer objects than the incoming has,
            // reallocate for enough space
            this->ptr = allocate(vec.len);
            this->cap = vec.len;
        }

        // nothing to deallocate - just leave our old data dangling

        std::uninitialized_copy_n(vec.ptr, vec.len, this->ptr);
        this->len = vec.len;

        return *this;
    }

    ArenaVec& operator=(ArenaVec&& vec) noexcept {
        std::destroy_n(this->ptr, this->len);
        this->ptr = vec.ptr;
        this->len = vec.len;
        this->cap = vec.cap;

        vec.ptr = nullptr;
        vec.len = 0;
        vec.cap = 0;

        return *this;
    }

    void assign(size_t n, const T& value) {
        T value_copy = value; // guard: value may alias an element we're about to destroy

        if constexpr (!std::is_trivially_destructible_v<T>) {
            std::destroy_n(this->ptr, this->len);
        }

        if (n > this->cap) {
            this->ptr = allocate(n);
            this->cap = n;
        }

        std::uninitialized_fill_n(this->ptr, n, value_copy);
        this->len = n;
    }

    // todo: assign overloads

    ArenaAllocator<T>& get_allocator() {
        return this->alloc;
    }

    T& at(size_t idx) {
        if (idx >= this->len) {
            throw std::out_of_range("out of range");
        }
        return *(this->ptr + idx);
    }

    T& operator[](size_t idx) {
        return *(this->ptr + idx);
    }

    T& front() { return *(this->ptr); }

    T& back() { return *(this->ptr + (this->len - 1)); }

    const T* data() { return this->ptr; }

    Iterator begin() { return Iterator(this->ptr, 0, this->len); }

    ConstIterator begin() const { return ConstIterator(this->ptr, 0, this->len); }

    ConstIterator cbegin() { return ConstIterator(this->ptr, 0, this->len); }

    Iterator end() { return Iterator(this->ptr, this->len, this->len); }

    ConstIterator end() const { return ConstIterator(this->ptr, this->len, this->len); }

    ConstIterator cend() { return ConstIterator(this->ptr, 0, this->len); }\

    ReverseIter rbegin() { return std::reverse_iterator(end()); }

    ReverseIter rend() { return std::reverse_iterator(begin()); }

    bool empty() const { return this->len == 0; }

    size_t size() const { return this->len; }

    size_t max_size() const { return (std::numeric_limits<size_t>::max() / sizeof(T)); }

    void reserve(size_t size = N) {
        if (this->ptr != nullptr) return;

        if (size <= N) {
            this->ptr = allocate(N);
            this->cap = N;
        } else {
            this->ptr = allocate(size);
            this->cap = size;
        }
    }

    size_t capacity() const { return this->cap; }

    static constexpr size_t initial_cap() { return N; }

    void clear() {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            std::destroy_n(this->ptr, this->len);
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
        T *point = this->ptr + this->len;
        T *obj = ::new (point) T(std::forward<Args>(args) ...);
        ++this->len;

        return *obj;
    }

    void pop_back() {
        std::destroy_at(this->ptr + (this->len - 1));
        --this->len;
    }

    void swap(ArenaVec& other) {
        T *data_temp = other.ptr;
        other.ptr = this->ptr;
        this->ptr = data_temp;

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

    // todo: rest of std::vector API, except shrink_to_fit

private:

    void grow() {
        size_t new_cap;
        if (this->cap == 0) {
            assert(this->len == 0);
            new_cap = N;
        } else {
            new_cap = this->cap * 2;
        }
        T *my_data = this->ptr;
        this->ptr = allocate(new_cap);
        this->cap = new_cap;

        std::uninitialized_copy_n(my_data, this->len, this->ptr);

        // destroy old data *after* copying
        std::destroy_n(my_data, this->len);
    }

    T *allocate(size_t n) {
        return ArenaVecBase<T, N>::AllocTraits::allocate(this->alloc, n);
    }
};

} // namespace ecc::ds

#endif