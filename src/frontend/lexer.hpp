#ifndef ECC_LEXER_H
#define ECC_LEXER_H

#if ! defined(yyFlexLexerOnce)
// FlexLexer.h is the definition for the Flex C++ scanner.
#include <FlexLexer.h>
#endif

// note: clangd will complain these two headers are missing if the build directory is clean.
#include "parser.hpp"
#include "location.hh"

namespace ecc::frontend {

class Lexer : public yyFlexLexer {
public:

    Lexer(std::istream *in) : yyFlexLexer(in) {
        
    }
};

}

#endif