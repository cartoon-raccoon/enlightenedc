#ifndef ECC_LEXER_H
#define ECC_LEXER_H

#if ! defined(yyFlexLexerOnce)
// FlexLexer.h is the definition for the Flex C++ scanner.
#include <FlexLexer.h>
#endif

// note: clangd will complain these two headers are missing if the build directory is clean.
#include "parser.hpp"


namespace ecc::frontend {

using namespace ecc::parser;

class Lexer : public yyFlexLexer {
public:

    // Use the standard yyFlexLexer constructor.
    Lexer(std::istream *in) : yyFlexLexer(in) {}

    // Override the yyFlexLexer constructor.
    int yylex(Parser::value_type *yylval, Parser::location_type *yylloc);
};

}

#endif