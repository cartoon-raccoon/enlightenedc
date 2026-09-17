#pragma once

#ifndef ECC_BUILTINS_H
#define ECC_BUILTINS_H

#include "util/string.hpp"

/*
This file defines builtin symbols as defined by the EnlightenedC spec, as well as some compiler-specific
names hardcoded into it.
*/

namespace ecc {

/**
The name defined by the spec as the implicit main function.
*/
constexpr StringRef EC_IMPLICIT_MAIN = "__ec_implicit_main";

/**
The name defined by the spec as the entry point.
*/
constexpr StringRef EC_ENTRY = "__ec_entry";

/**
The name defined by the spec as the print function.
*/
constexpr StringRef EC_PRINT = "__ec_print";

constexpr StringRef EC_BUILTIN_VA_START = "__ec_builtin_va_start";

constexpr StringRef EC_BUILTIN_VA_END = "__ec_builtin_va_end";

/**
The name of the implicitly defined argc symbol inside a variadic function.
*/
constexpr StringRef EC_IMPLICIT_ARGC = "argc";

/**
The name of the implicitly defined argv symbol inside a variadic function.
*/
constexpr StringRef EC_IMPLICIT_ARGV = "argv";

/**
A mangling name used when lowering implicit argc. Not in the spec, hence the `ecc` prefix.
*/
constexpr StringRef ECC_MANGLED_ARGC = "__ecc_implicit_argc";

/**
A mangling name used when lowering implicit argv. Not in the spec, hence the `ecc` prefix.
*/
constexpr StringRef ECC_MANGLED_ARGV = "__ecc_implicit_argv";

}

#endif