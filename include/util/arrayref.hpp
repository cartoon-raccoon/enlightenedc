#pragma once

#ifndef ECC_ARRAYREF_H
#define ECC_ARRAYREF_H

#include <algorithm>
#include <concepts>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iterator>

#include "aliases.hpp"

namespace ecc::util {

template <typename C, typename T>
concept ArrayRefConvertible = requires(const C& c) {
    requires std::convertible_to<decltype(c.data()) *, const T *const *>;
    { c.size() } -> std::integral;
};


template <typename T>
class [[nodiscard]] ArrayRef {
public:
    using value_type = T;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using reference = value_type &;
    using const_reference = const value_type &;
    using iterator = const_pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

private:
    const T *elems = nullptr;
    size_type len = 0;

public:
    ArrayRef() = default;

    ArrayRef(const T& elem) : elems(&elem), len(1) {}

    template <ArrayRefConvertible<T> C>
    constexpr ArrayRef(const C& c) : elems(c.data()), len(c.size()) {}

    template <size_t N>
    constexpr ArrayRef(const T(&arr)[N]) : elems(arr), len(N) {}

    // Disable gcc's warning in this constructor as it generates an enormous amount
    // of messages. Anyone using ArrayRef should already be aware of the fact that
    // it does not do lifetime extension.
// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Winit-list-lifetime"
    constexpr ArrayRef(std::initializer_list<T> vec)
        : elems(vec.begin() == vec.end() ? (T *)nullptr : vec.begin()),
            len(vec.size()) {}
// #pragma GCC diagnostic pop

    bool empty() { return len == 0; }

    const T *data() { return elems; }

    size_type size() { return len; }

    const T& front() const {
        ECC_ASSERT_N(!empty());
        return elems[0];
    }

    const T& back() const {
        ECC_ASSERT_N(!empty());
        return elems[len - 1];
    }

    iterator begin() { return elems; }
    iterator end() { return elems + len; }

    reverse_iterator rbegin() { return reverse_iterator(end()); }
    reverse_iterator rend() { return reverse_iterator(begin()); }

    const_iterator cbegin() { return elems; }
    const_iterator cend() { return elems + len; }

    const_iterator begin() const { return elems; }
    const_iterator end() const { return elems+len; }

    bool operator==(const ArrayRef other) { return equals(other); }

    bool equals(const ArrayRef other) {
        return std::ranges::equal(*this, other);
    }

    ArrayRef<T> slice(size_type from, size_type to) {
        ECC_ASSERT(from < to && to < len, "invalid specifiers for ArrayRef::slice");

        return ArrayRef(data() + from, to - from);
    }

    ArrayRef<T> slice_n(size_type from, size_type n) {
        ECC_ASSERT(from + n <= size(), "invalid specifiers for ArrayRef::slice_n");
        return ArrayRef(data() + from, n);
    }

    Vec<T> vector() {
        return Vec(data(), size());
    }

    // todo: remaining llvm::ArrayRef API
};

}

#endif