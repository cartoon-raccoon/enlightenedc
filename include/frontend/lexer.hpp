#pragma once

#ifndef ECC_LEXER_H
#define ECC_LEXER_H

#include "driver/filenames.hpp"

#if !defined(yyFlexLexerOnce)
// FlexLexer.h is the definition for the Flex C++ scanner.
#include <FlexLexer.h>
#endif

#include <set>

#include "parser.hpp"
#include "util.hpp"

namespace ecc::frontend {

class Lexer : public yyFlexLexer {
    Location loc;

    Ref<std::set<std::string>> typedefs;
    Ref<driver::FilenamePool> filenames;

public:
    // Use the standard yyFlexLexer constructor.
    Lexer(
        std::istream *in, std::string *filename, std::set<std::string>& typedefs,
        driver::FilenamePool& filenames);

    // Override the yyFlexLexer constructor.
    Parser::symbol_type get_next_token();

    void handle_linemarker(const char *yytext);
};

} // namespace ecc::frontend

#endif