#pragma once

#ifndef ECC_LEXER_H
#define ECC_LEXER_H

#include "driver/filenames.hpp"

#if !defined(yyFlexLexerOnce)
// FlexLexer.h is the definition for the Flex C++ scanner.
#include <FlexLexer.h>
#endif

#include "parser.hpp"
#include "prelude.hpp"
#include "util/string.hpp"

namespace ecc::frontend {

/**
A class implementing the bodgiest of bodges.

Look upon this and despair, the personification of jank, the bodge to rule all bodges;
binding the parser and lexer thus in a mutually dependent cycle of non-context-free glory,
forever unwillingly bound to serve one another, an ouroboros of tokens.

IN TECHNICAL SPEAK: See [this](https://en.wikipedia.org/wiki/Lexer_hack) for an explanation.
*/
class LexerHack {
public:
    struct Scope {
        StringHashSet typedefs;
        StringHashSet identifiers;
    };

private:
    Vec<Scope> stack;

public:
    /**
    Push a new scope to the stack.
    */
    void push_scope() {
        stack.emplace_back();
    }
    
    /**
    Pop the current scope off the stack.
    */
    void pop_scope() {
        stack.pop_back();
    }

    /**
    */
    bool contains() {
        // todo
        return false;
    }
};

class Lexer : public yyFlexLexer {
    Location loc;

    Ref<StringHashSet> typedefs;
    Ref<driver::FilenamePool> filenames;

public:
    // Use the standard yyFlexLexer constructor.
    Lexer(
        std::istream *in, std::string *filename, StringHashSet& typedefs,
        driver::FilenamePool& filenames);

    // Override the yyFlexLexer constructor.
    Parser::symbol_type get_next_token();

    void handle_linemarker(const char *yytext);
};

} // namespace ecc::frontend

#endif