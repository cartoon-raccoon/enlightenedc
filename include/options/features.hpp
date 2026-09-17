#pragma once

#ifndef ECC_FEATUREFLAGS_H
#define ECC_FEATUREFLAGS_H

#include <array>

#include "util/aliases.hpp"
#include "util/string.hpp"

namespace ecc {

enum class FeatureFlag : uint8_t {
    HOLYC_VARARGS, // Enable HolyC-style variable arguments even if in EnlightenedC mode.
};

struct FeatureFlagData {
    FeatureFlag flag;
    StringRef name;
    bool default_enabled;

    constexpr FeatureFlagData(FeatureFlag flag, StringRef name, bool default_enabled)
        : flag(flag), name(name), default_enabled(default_enabled) {}
};

inline constexpr
std::array FEATURE_FLAGS = std::to_array<FeatureFlagData>({
    {FeatureFlag::HOLYC_VARARGS, "holyc-varargs", false},
});

class FeatureFlags {
    util::HashSet<FeatureFlag> flags;

public:
    void set(FeatureFlag flag) { flags.insert(flag); }

    bool is_set(FeatureFlag flag) { return flags.contains(flag); }
};

}

#endif