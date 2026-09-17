#pragma once

#ifndef ECC_WARNINGFLAGS_H
#define ECC_WARNINGFLAGS_H

#include <array>

#include "util/aliases.hpp"
#include "util/string.hpp"

namespace ecc {

enum class WarningFlag : uint8_t {
    OVERSIZED_JUMP_TABLE, // an oversized jump table.
    IMPLICIT_INCOMPAT_PTR_CAST, // implicit cast of two pointers with incompatible bases.
    IMPLICIT_NARROWING, // implicit narrowing without an explicit cast.
    REINTERPRET_FLOAT, // a reinterpret expression was used on a float type.
    UNARY_POS_NEG_ON_UNSIGNED, // an unary pos/neg operation was used on an unsigned type
    ATTRIBUTED_CONSTEXPR, // attributes were found on a constexpr expression.
    CONSTEXPR_FLT_TO_INT, // constexpr declared as integer, defined as float.
    NON_INT_UNION_TYPEREP, // type-repped union representative is not an integer.
    QUALIFIED_TYPE_RVALUE, // rvalues are stripped of all type qualifications.
};

struct WarningFlagData {
    WarningFlag flag;
    StringRef name;
    StringRef warning_text;
    bool default_enabled;

    constexpr WarningFlagData(WarningFlag flag, StringRef name, StringRef text, bool default_enabled)
        : flag(flag), name(name), warning_text(text), default_enabled(default_enabled) {}
};

inline constexpr
std::array WARNING_FLAGS = std::to_array<WarningFlagData>({
    {
        WarningFlag::OVERSIZED_JUMP_TABLE,
        "oversized-jump-table",
        "this case range might produce an oversized jump table; consider using an if statement",
        true,
    },
    {
        WarningFlag::IMPLICIT_INCOMPAT_PTR_CAST,
        "implicit-incompat-ptr-cast",
        "comparison of distinct pointer types lacks a cast",
        true,
    },
    {
        WarningFlag::IMPLICIT_NARROWING,
        "implicit-narrowing",
        "integer is narrowed to a smaller representation, which might cause issues",
        false,
    },
    {
        WarningFlag::REINTERPRET_FLOAT,
        "reinterpret-float",
        "reinterpreting floats as a bytearray is risky",
        true,
    },
    {
        WarningFlag::UNARY_POS_NEG_ON_UNSIGNED,
        "unary-pos-neg-on-unsigned",
        "unary +/- on an unsigned type might cause unintended behaviour",
        true,
    },
    {
        WarningFlag::ATTRIBUTED_CONSTEXPR,
        "attributed-constexpr",
        "attributes on constexpr declarations are ignored",
        false,
    },
    {
        WarningFlag::CONSTEXPR_FLT_TO_INT,
        "constexpr-flt-to-int",
        "defining an integer constexpr as a float might cause truncation",
        true,
    },
    {
        WarningFlag::NON_INT_UNION_TYPEREP,
        "non-int-union-typerep",
        "using a non-integer as a union's type representative might cause undefined behaviour",
        true,
    },
    {
        WarningFlag::QUALIFIED_TYPE_RVALUE,
        "qualified-type-rvalue",
        "type qualifications are ignored on r-values",
        true,
    },
});

class WarningFlags {
    util::HashSet<WarningFlag> flags;

public:
    void set(WarningFlag flag) { flags.insert(flag); }

    bool is_set(WarningFlag flag) { return flags.contains(flag); }
};

}

#endif
