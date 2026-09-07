#pragma once

#ifndef ECC_UTIL_ITERATOR_H
#define ECC_UTIL_ITERATOR_H

#include <cstddef>

#include "util/aliases.hpp"

namespace ecc::util {

/*
 * ITERATOR FACADE CONCEPTS
 *
 * A ladder of concepts describing how far a CRTP-derived iterator type has
 * gotten on its own, one tier at a time. Each concept below checks for
 * exactly one raw operation the derived type is expected to hand-implement --
 * never the full complement of operators a facade built on top would go on to
 * synthesize (postfix ++/--, !=, +, -, [], relational ops), since checking
 * for those here would be circular: they don't exist until the facade adds
 * them.
 */

// Single raw operations. Kept apart from the composed tiers below so each
// one only ever tests one primitive.

/**
A concept that expresses that operator*() is defined.
*/
template <typename D>
concept IterDeref = requires(const D& d) { *d; };

/**
A concept that expresses that operator++() is defined.
*/
template <typename D>
concept IterPreInc = requires(D d) {
    { ++d } -> std::same_as<D&>;
};

/**
A concept that expresses that operator--() is defined.
*/
template <typename D>
concept IterPreDec = requires(D d) {
    { --d } -> std::same_as<D&>;
};

/**
A concept that expresses that operator+=() is defined.
*/
template <typename D, typename Diff = std::ptrdiff_t>
concept IterAdvance = requires(D d, Diff n) {
    { d += n } -> std::same_as<D&>;
};

/**
A concept that expresses that operator+-() is defined.
*/
template <typename D, typename Diff = std::ptrdiff_t>
concept IterBacktrack = requires(D d, Diff n) {
    { d -= n } -> std::same_as<D&>;
};

/**
A concept that expresses that a distance between two iterators can be computed.
*/
template <typename D, typename Diff = std::ptrdiff_t>
concept IterDistance = requires(const D& a, const D& b) {
    { a - b } -> std::convertible_to<Diff>;
};

// The composed tiers, each built by conjunction with the tier below it, so
// satisfying a stronger tier implies every weaker one -- no inheritance
// hierarchy (a la the old iterator_category tags) needed to express "at
// least this strong." A facade should gate its synthesized members on
// these, never on the primitives above directly.

template <typename D>
concept IterInputCapable = IterDeref<D> && IterPreInc<D> && std::equality_comparable<D>;

template <typename D>
concept IterForwardCapable =
    IterInputCapable<D> &&
    std::semiregular<D>; // default-constructible + copyable: the closest syntactic
                         // proxy available for "multipass", which isn't otherwise checkable

template <typename D>
concept IterBidirectionalCapable = IterForwardCapable<D> && IterPreDec<D>;

template <typename D>
concept IterRandomAccessCapable =
    IterBidirectionalCapable<D> && IterAdvance<D> && IterBacktrack<D> && IterDistance<D>;

// Contiguity is a semantic guarantee (elements are adjacent in memory), not a
// syntactic one -- there's no expression to probe for it, so it can't be
// inferred like the tiers above. A derived type opts in explicitly by
// defining `static constexpr bool is_contiguous_iterator = true;`.
template <typename D>
concept IterContiguousCapable =
    IterRandomAccessCapable<D> && requires { requires D::is_contiguous_iterator; };

/**
A CRTP base for iterators, based of LLVM's iterator_facade_base.
*/
template <
    typename DerivedT, typename T, typename DifferenceT = std::ptrdiff_t, typename PointerT = T *,
    typename ReferenceT = T&>

class IteratorBase {
public:
    using value_type      = T;
    using difference_type = DifferenceT;
    using pointer         = PointerT;
    using reference       = ReferenceT;

protected:
    /// A proxy object for computing a reference via indirecting a copy of an
    /// iterator. This is used in APIs which need to produce a reference via
    /// indirection but for which the iterator object might be a temporary. The
    /// proxy preserves the iterator internally and exposes the indirected
    /// reference via a conversion operator.
    class ReferenceProxy {
        friend IteratorBase;

        DerivedT I;

        ReferenceProxy(DerivedT I) : I(std::move(I)) {}

    public:
        operator ReferenceT() const { return *I; }
    };

    /// A proxy object for computing a pointer via indirecting a copy of a
    /// reference. This is used in APIs which need to produce a pointer but for
    /// which the reference might be a temporary. The proxy preserves the
    /// reference internally and exposes the pointer via a arrow operator.
    class PointerProxy {
        friend IteratorBase;

        ReferenceT R;

        template <typename RefT>
        PointerProxy(RefT&& R) : R(std::forward<RefT>(R)) {}

    public:
        // Not const: when ReferenceT is a plain value (not a true reference),
        // R is stored by value, and a const method here would make `&R`
        // yield a `const T*`, which won't convert to a non-const PointerT.
        // The proxy is a short-lived temporary, so dropping const costs
        // nothing.
        PointerT operator->() { return &R; }
    };

public:
    template <typename D = DerivedT>
        requires(IterRandomAccessCapable<D>)
    friend DerivedT operator+(const DerivedT& i, DifferenceT n) {
        static_assert(
            std::is_base_of_v<IteratorBase, DerivedT>,
            "you must pass the derived class into this class!");

        DerivedT tmp = i;
        tmp += n;
        return tmp;
    }

    template <typename D = DerivedT>
        requires(IterRandomAccessCapable<D>)
    friend DerivedT operator+(DifferenceT n, const DerivedT& i) {
        static_assert(
            std::is_base_of_v<IteratorBase, DerivedT>,
            "you must pass the derived class into this class!");

        return i + n;
    }

    template <typename D = DerivedT>
        requires(IterRandomAccessCapable<D>)
    friend DerivedT operator-(const DerivedT& i, DifferenceT n) {
        static_assert(
            std::is_base_of_v<IteratorBase, DerivedT>,
            "you must pass the derived class into this class!");

        DerivedT tmp = i;
        tmp -= n;
        return tmp;
    }

    template <typename D = DerivedT>
        requires(IterAdvance<D>)
    DerivedT& operator++() {
        static_assert(
            std::is_base_of_v<IteratorBase, DerivedT>,
            "you must pass the derived class into this class!");

        return static_cast<DerivedT *>(this)->operator+=(1);
    }

    template <typename D = DerivedT>
        requires(IterInputCapable<D>)
    friend DerivedT operator++(DerivedT& i, int) {
        DerivedT tmp = i;
        ++i;
        return tmp;
    }

    template <typename D = DerivedT>
        requires(IterBacktrack<D>)
    DerivedT& operator--() {
        static_assert(
            std::is_base_of_v<IteratorBase, DerivedT>,
            "you must pass the derived class into this class!");

        return static_cast<DerivedT *>(this)->operator-=(1);
    }

    template <typename D = DerivedT>
        requires(IterBidirectionalCapable<D>)
    friend DerivedT operator--(DerivedT& i, int) {
        DerivedT tmp = i;
        --i;
        return tmp;
    }

    template <typename D = DerivedT>
        requires(IterDeref<D>)
    decltype(auto) operator->() const {
        static_assert(
            std::is_base_of_v<IteratorBase, DerivedT>,
            "you must pass the derived class into this class!");

        const DerivedT& self = *static_cast<const DerivedT *>(this);
        if constexpr (std::is_reference_v<ReferenceT>) {
            return &*self;
        } else {
            return PointerProxy(*self);
        }
    }

    template <typename D = DerivedT>
        requires(IterRandomAccessCapable<D>)
    ReferenceProxy operator[](DifferenceT n) const {
        static_assert(
            std::is_base_of_v<IteratorBase, DerivedT>,
            "you must pass the derived class into this class!");

        return ReferenceProxy(*static_cast<const DerivedT *>(this) + n);
    }

    template <typename D = DerivedT>
        requires(IterRandomAccessCapable<D>)
    friend auto operator<=>(const DerivedT& a, const DerivedT& b) {
        static_assert(
            std::is_base_of_v<IteratorBase, DerivedT>,
            "you must pass the derived class into this class!");

        return (a - b) <=> 0;
    }
};

template <typename T>
using NextResult = std::conditional_t<std::is_pointer_v<T>, T, Optional<T>>;

/**
A Rust-style iterator that signals the end of iteration by
returning an empty Optional, or nullptr if T is a pointer.
*/
template <typename T>
class NextIterator {
    static_assert(!std::is_reference_v<T>);

public:
    virtual ~NextIterator() = default;

    virtual NextResult<T> next() = 0;
};

}

#endif