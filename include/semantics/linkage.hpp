#pragma once

#ifndef ECC_SEMA_LINKAGE_H
#define ECC_SEMA_LINKAGE_H

namespace ecc::sema {

// The linkage of the symbol.
enum class Linkage : uint8_t {
    // The symbol has no linkage.
    NONE,
    // The symbol has internal linkage.
    INTERNAL,
    // The symbol has external linkage.
    EXTERNAL,
};

enum class LangLinkage : uint8_t {
    NONE, // The symbol has no language linkage.
    C, // The symbol has "C" language linkage.
};

template <typename Link>
concept IsLinkage = requires {
    Link::NONE;
};

/**
Check if `mine` Linkage is compatible with `other` linkage.

The two linkages are compatible if `other` is `Link::NONE` or they match.
*/
template <typename Link>
    requires IsLinkage<Link>
bool linkages_are_compatible(Link mine, Link other) {
    return other == Link::NONE || mine == other;
}

} // end namespace ecc::sema

#endif