#pragma once

#ifndef ECC_UTIL_H
#define ECC_UTIL_H

#include <compare>
#include <concepts>
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <source_location>
#include <sstream>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

/*
* CONSTANTS
*/

/**
The Boost Golden Ratio used when hashing.
*/
constexpr std::size_t BOOST_GOLDEN_RATIO = 0x9e3779b9;

/**
The left-bitshift value used when hashing.
*/
constexpr std::size_t HASH_SHL           = 6;

/**
The right-bitshift value used when hashing.
*/
constexpr std::size_t HASH_SHR           = 2;

/*
* DEBUG PRINTING AND TODOS
*/

#ifndef NDEBUG
#include <iostream>

template <typename... Args>
void dbprint(Args&&...args) {
    (std::cerr << ... << std::forward<Args>(args)) << "\n";
}
#else
template <typename T, typename... Args>
void dbprint(T msg, Args&&...args) {
}
#endif

#define DO_ACCEPT(tyname, vistype) /*NOLINT*/            \
    void tyname::accept(vistype& visitor) /* NOLINT */ { \
        visitor.visit(*this);                            \
    }

#define VISIT_NO_IMPL(_node)           /* NOLINT */                                             \
    void visit(_node& node) override { /*NOLINT */                                              \
        throw std::runtime_error("visit() was not implemented for the current visitable node"); \
    }

#define todo() throw Todo(std::source_location::current()) // NOLINT

namespace ecc::util {

/**
An exception class to indicate that a region of code is currently unimplemented.
*/
class Todo : std::exception {
public:
    std::string location;

    Todo(std::source_location at) {
        std::stringstream ss;
        ss << at.file_name() << " - ";
        ss << at.function_name();
        ss << " (" << at.line() << ":" << at.column() << ")";
        location = ss.str();
    }

    const char *what() const noexcept override { return location.c_str(); }
};

/*
* TYPE ALIASES
*/

/**
A convenient type alias for `std::unique_ptr`.
*/
template <typename T>
using Box = std::unique_ptr<T>;

template <typename T, typename... Args>
auto make_box(Args&&...args) -> decltype(std::make_unique<T>(std::forward<Args>(args)...)) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

/**
A convenient type alias for `std::vector`.
*/
template <typename T>
using Vec = std::vector<T>;

/**
A convenient type alias for `std::unordered_map`.
*/
template <typename... Args>
using HashMap = std::unordered_map<Args...>;

/**
A convenient type alias for `std::unordered_set`.
*/
template <typename... Args>
using HashSet = std::unordered_set<Args...>;

/**
A convenient type alias for `std::span`.
*/
template <typename T>
using Span = std::span<T>;

/**
A convenient type alias for `std::optional`.
*/
template <typename T>
using Optional = std::optional<T>;

/**
A convenient type alias for `std::reference_wrapper`.
*/
template <typename T>
using Ref = std::reference_wrapper<T>;

/**
A convenient type alias for `std::pair`.
*/
template <typename T1, typename T2>
using Pair = std::pair<T1, T2>;

// Overloaded template class for Rust-style pattern matching on variants.
template <class... Ts>
struct match : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
match(Ts...) -> match<Ts...>;

template <typename... Types>
struct VarHash {
    // Helper to combine an individual seed with a new value
    void hash_combine(std::size_t& seed, const auto& val) const {
        std::hash<std::decay_t<decltype(val)>> hasher;
        // The Boost "Golden Ratio" formula
        seed ^= hasher(val) + BOOST_GOLDEN_RATIO + (seed << HASH_SHL) + (seed >> HASH_SHR);
    }

    std::size_t operator()(const Types&...args) const {
        std::size_t seed = 0;
        // C++17 Fold Expression: applies hash_combine to every argument in args
        (hash_combine(seed, args), ...);
        return seed;
    }
};

/*
* CONCEPTS
*/

// Helper to check if T is in the list of Types...
template <typename T, typename Variant>
struct is_variant_member;

template <typename T, typename... Types>
struct is_variant_member<T, std::variant<Types...>>
    : std::bool_constant<(std::is_same_v<T, Types> || ...)> {};

// Concept to check if a type T is a member of a std::variant,
template <typename T, typename Variant>
concept VariantMember = is_variant_member<T, Variant>::value;

/*
* UTILITY CLASSES
*/

/**
A counter that keeps increasing.
*/
template <typename I>
    requires std::is_integral_v<I>
class MonotonicCtr {
    I val;

public:
    MonotonicCtr<I>() : val(0) {}

    MonotonicCtr(I val) : val(val) {}

    MonotonicCtr(const MonotonicCtr<I>& c) : val(c.val) {}

    MonotonicCtr(MonotonicCtr<I>&& c) noexcept : val(c.val) {
        c.val = 0;
        // reset the moved-from counter to 0, since it's monotonic and should never decrease.
    }

    I value() const { return val; }

    I inc() { return val++; }

    I add(I n) { return val += n; }

    I operator*() { return val; }

    I operator++() { return ++val; }

    I operator++(int) { return val++; }

    I operator+(I n) const { return val + n; }

    I operator-(I n) const { return val - n; }

    I operator+=(I n) { return add(n); }

    std::strong_ordering operator<=>(const MonotonicCtr<I>& other) { return val <=> other.val; }

    std::strong_ordering operator<=>(const I& other) { return val <=> other; }
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
concept IterInputCapable =
    IterDeref<D> && IterPreInc<D> && std::equality_comparable<D>;

template <typename D>
concept IterForwardCapable =
    IterInputCapable<D> &&
    std::semiregular<D>; // default-constructible + copyable: the closest syntactic
                          // proxy available for "multipass", which isn't otherwise checkable

template <typename D>
concept IterBidirectionalCapable =
    IterForwardCapable<D> && IterPreDec<D>;

template <typename D>
concept IterRandomAccessCapable =
    IterBidirectionalCapable<D> && IterAdvance<D> && IterBacktrack<D> && IterDistance<D>;

// Contiguity is a semantic guarantee (elements are adjacent in memory), not a
// syntactic one -- there's no expression to probe for it, so it can't be
// inferred like the tiers above. A derived type opts in explicitly by
// defining `static constexpr bool is_contiguous_iterator = true;`.
template <typename D>
concept IterContiguousCapable =
    IterRandomAccessCapable<D> && requires {
    requires D::is_contiguous_iterator;
};

/**
A CRTP base for iterators, based of LLVM's iterator_facade_base.
*/
template <typename DerivedT, typename T, 
    typename DifferenceT = std::ptrdiff_t, 
    typename PointerT = T *, typename ReferenceT = T&>
    
class IteratorBase {
public:
    using value_type = T;
    using difference_type = DifferenceT;
    using pointer = PointerT;
    using reference = ReferenceT;

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
        PointerT operator->() const { return &R; }
    };

public:
    template <typename D = DerivedT>
        requires (IterRandomAccessCapable<D>)
    DerivedT operator+(DifferenceT n) const {
        static_assert(
            std::is_base_of_v<IteratorBase, DerivedT>,
            "you must pass the derived class into this class!");

        DerivedT tmp = *static_cast<const DerivedT *>(this);
        tmp += n;
        return tmp;
    }

    template <typename D = DerivedT>
        requires (IterRandomAccessCapable<D>)
    friend DerivedT operator+(DifferenceT n, const DerivedT &i) {
        static_assert(
            std::is_base_of_v<IteratorBase, DerivedT>,
            "you must pass the derived class into this class!");

        return i + n;
    }

    template <typename D = DerivedT>
        requires (IterRandomAccessCapable<D>)
    DerivedT operator-(DifferenceT n) const {
        static_assert(
            std::is_base_of_v<IteratorBase, DerivedT>,
            "you must pass the derived class into this class!");
        
        DerivedT tmp = *static_cast<const DerivedT *>(this);
        tmp -= n;
        return tmp;
    }

    template <typename D = DerivedT>
        requires (IterInputCapable<D>)
    DerivedT& operator++() {
        static_assert(
            std::is_base_of_v<IteratorBase, DerivedT>,
            "you must pass the derived class into this class!");

        return static_cast<DerivedT *>(this)->operator+=(1);
    }

    template <typename D = DerivedT>
        requires (IterInputCapable<D>)
    DerivedT operator++(int) {
        DerivedT tmp = *static_cast<DerivedT *>(this);
        ++*static_cast<DerivedT *>(this);
        return tmp;
    }
};

/*
* NOCOPY, NOMOVE
*/

class NoCopy { // NOLINT(cppcoreguidelines-special-member-functions)
public:
    NoCopy(NoCopy const&)            = delete;
    NoCopy& operator=(NoCopy const&) = delete;

    NoCopy(NoCopy&&)            = default;
    NoCopy& operator=(NoCopy&&) = default;
    NoCopy()                    = default;
};

class NoMove { // NOLINT(cppcoreguidelines-special-member-functions)
public:
    NoMove(NoMove&&)            = delete;
    NoMove& operator=(NoMove&&) = delete;

    NoMove() = default;
};

/*
* MANUAL RTTI FUNCTIONALITY
*/

template <typename To, typename From>
concept HasClassof = requires(const From *f) {
    { To::classof(f) } -> std::convertible_to<bool>;
};

template <typename To, typename From>
bool isa(const From *val) {
    static_assert(std::is_base_of_v<From, To>);
    if constexpr (std::is_same_v<To, From>) {
        return true;
    } else {
        static_assert(HasClassof<To, From>);
        return To::classof(val);
    }
}

template <typename To, typename From>
bool isa(const Box<From>& val) {
    static_assert(std::is_base_of_v<From, To>);
    if constexpr (std::is_same_v<To, From>) {
        return true;
    } else {
        static_assert(HasClassof<To, From>);
        return To::classof(val.get());
    }
}

template <typename To, typename From>
To *cast(From *val) {
    assert(val && isa<To>(val) && "dyncast<>: incompatible types of To and From");
    return static_cast<To *>(val);
}

template <typename To, typename From>
const To *cast(const From *val) {
    assert(val && isa<To>(val) && "dyncast<>: incompatible types of To and From");
    return static_cast<const To *>(val);
}

template <typename To, typename From>
To *cast(const Box<From>& val) {
    return cast<To>(val.get());
}

template <typename To, typename From>
To *dyncast(From *val) {
    return val && isa<To>(val) ? static_cast<To *>(val) : nullptr;
}

template <typename To, typename From>
const To *dyncast(const From *val) {
    return val && isa<To>(val) ? static_cast<const To *>(val) : nullptr;
}

template <typename To, typename From>
To *dyncast(Box<From>& val) {
    return dyncast<To>(val.get());
}

} // namespace ecc::util

#endif