#pragma once

#ifndef ECC_BUILTINS_H
#define ECC_BUILTINS_H

#include "util/string.hpp"

/*
This file defines the builtin symbols as defined by the EnlightenedC spec.
*/

namespace ecc {

constexpr StringRef EC_IMPLICIT_MAIN = "__ec_implicit_main";

constexpr StringRef EC_ENTRY = "__ec_entry";

constexpr StringRef EC_PRINT = "__ec_print";

constexpr StringRef EC_BUILTIN_VA_START = "__ec_builtin_va_start";

constexpr StringRef EC_BUILTIN_VA_END = "__ec_builtin_va_end";

}

#endif