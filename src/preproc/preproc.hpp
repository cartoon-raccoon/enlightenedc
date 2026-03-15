#ifndef ECC_PREPROC_H
#define ECC_PREPROC_H

#include <iostream>
#include <ext/stdio_filebuf.h> // For GCC/Clang
#include <cstdio>
#include <FlexLexer.h>

namespace ecc::preproc {

/*

The EnlightenedC Preprocessor.

Currently this wraps the system `cpp` command and returns the fully preprocessed
source as a string.
*/
class PreProcessor : public std::istream {
    FILE *pipe;
    __gnu_cxx::stdio_filebuf<char> buffer;
public:
    // Throws std::runtime_error on failure
    PreProcessor(const std::string *filename);

    ~PreProcessor();
};

} // namespace ecc::preproc

#endif