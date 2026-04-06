#pragma once

#ifndef ECC_LEXER_H
#define ECC_LEXER_H

#if !defined(yyFlexLexerOnce)
// FlexLexer.h is the definition for the Flex C++ scanner.
#include <FlexLexer.h>
#endif

#include <set>

#include "frontend/filenames.hpp"
#include "parser.hpp"
#include "util.hpp"

namespace ecc::frontend {

extern frontend::FilenamePool filenames;

class Lexer : public yyFlexLexer {
public:
    // Use the standard yyFlexLexer constructor.
    Lexer(std::istream *in, std::string *filename, std::set<std::string>& typedefs);

    Location loc;

    std::set<std::string>& typedefs;

    // Override the yyFlexLexer constructor.
    Parser::symbol_type get_next_token();

    void handle_linemarker(const char *yytext);
};

} // namespace ecc::frontend

#endif