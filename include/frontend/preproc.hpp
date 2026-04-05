#ifndef ECC_PREPROC_H
#define ECC_PREPROC_H

#include <iostream>
#include <ext/stdio_filebuf.h> // For GCC/Clang
#include <cstdio>
#include <FlexLexer.h>

#include "error.hpp"

namespace ecc::frontend {

class PreprocError : public EccError {
public:
    PreprocError() : EccError("preprocessor exited with error") {}
};

/*

The EnlightenedC Preprocessor.

Currently this wraps the system `cpp` command and returns the fully preprocessed
source as a string.
*/
class Preprocessor : public std::istream {
    FILE *pipe;
    __gnu_cxx::stdio_filebuf<char> buffer;
public:
    // Throws std::runtime_error on failure
    Preprocessor(const std::string *filename);

    // Calls pclose on the PreProcessor's internal file descriptor.
    // Throws error if `cpp` terminated unusually for whatever reason.
    void close();

    ~Preprocessor() override;
};

} // namespace ecc::frontend

#endif