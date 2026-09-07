#pragma once

#ifndef ECC_UTIL_STRING_H
#define ECC_UTIL_STRING_H

#include <string>
#include <string_view>

namespace ecc::util {

/*
 * STRING UTILITIES
 */

/**
Converts non-alphanumeric characters in a string into their escape-sequence
form (e.g. a newline byte becomes the two characters '\\' and 'n'), mirroring
the escape handling performed by `decode_string_literal()` in the lexer, but
in reverse. Backslashes, quote characters, and non-printable bytes are
escaped (falling back to `\xHH` when there's no named escape sequence);
printable punctuation and spaces are left untouched.
*/
std::string encode_string_literal(std::string_view raw);

}

#endif