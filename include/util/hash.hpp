#pragma once

#ifndef ECC_UTIL_HASH_H
#define ECC_UTIL_HASH_H

#include <boost/container_hash/hash.hpp>

#include "util/aliases.hpp"

namespace ecc::util {

/**
The Boost Golden Ratio used when hashing.
*/
constexpr std::size_t BOOST_GOLDEN_RATIO = 0x9e3779b9;

/**
The left-bitshift value used when hashing.
*/
constexpr std::size_t HASH_SHL = 6;

/**
The right-bitshift value used when hashing.
*/
constexpr std::size_t HASH_SHR = 2;

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

template <typename T1, typename T2>
struct PairHash {
    size_t operator()(const Pair<T1, T2>& pair) const {
        auto varhash = VarHash<T1, T2>();
        return varhash(pair.first, pair.second);
    }
};

/**
Helper type for hashing a contiguous sequence of pointers.
*/
template <typename T>
struct SeqHash {
    using is_transparent = void;

    std::size_t operator()(Span<T * const> s) const {
        return boost::hash_range(s.begin(), s.end());
    }

    std::size_t operator()(const Vec<T *>& v) const {
        return (*this)(Span<T *const>{v});
    }
};

/**
Helper type for checking the equality of a contiguous sequence of pointers.
*/
template <typename T>
struct SeqEq {
    using is_transparent = void;

    bool operator()(Span<T *const> a, Span<T *const> b) const {
        return std::ranges::equal(a, b);
    }

    bool operator()(const Vec<T *>& a, Span<T *const> b) const {
        return std::ranges::equal(a, b);
    }

    bool operator()(const Vec<T *>& a, const Vec<T *>& b) const {
        return a == b;
    }
};

}

#endif