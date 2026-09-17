#pragma once

#ifndef ECC_WARNINGFLAGS_H
#define ECC_WARNINGFLAGS_H

#include <array>

#include "util/aliases.hpp"
#include "util/string.hpp"

namespace ecc {

enum class WarningFlag : uint8_t {
    OVERSIZED_JUMP_TABLE, // an oversized jump table.
    NO_EXPLICIT_PTRPTR_CAST, // implicit cast of two pointers with incompatible bases.
    IMPLICIT_NARROWING, // implicit narrowing without an explicit cast.
    REINTERPRET_FLOAT, // reinterpreting floats as a bytearray (risky)
    ATTRIBUTED_CONSTEXPR, // attributes on constexprs are ignored
    CONSTEXPR_FLT_TO_INT, // constexpr declared as integer, defined as float
    NON_INT_UNION_TYPEREP, // type-repped union's representative is not integer
    QUALIFIED_TYPE_RVALUE, //
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
    }
});

class WarningFlags {
    util::HashSet<WarningFlag> flags;

public:
    void set(WarningFlag flag) { flags.insert(flag); }

    bool is_set(WarningFlag flag) { return flags.contains(flag); }
};

}

#endif