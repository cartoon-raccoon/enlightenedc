#ifndef ECC_LEXER_H
#define ECC_LEXER_H

#if ! defined(yyFlexLexerOnce)
// FlexLexer.h is the definition for the Flex C++ scanner.
#include <FlexLexer.h>
#endif

// note: clangd will complain this header is missing if the build directory is clean.
#include "parser.hpp"
#include "util.hpp"


namespace ecc::frontend {

using namespace ecc::parser;
using namespace ecc::util;

class Lexer : public yyFlexLexer {
public:

    // Use the standard yyFlexLexer constructor.
    Lexer(std::istream *in) : yyFlexLexer(in) {}

    Location loc;

    // Override the yyFlexLexer constructor.
    Parser::symbol_type get_next_token();
};

}

#endif