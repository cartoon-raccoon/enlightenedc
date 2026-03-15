#ifndef ECC_LEXER_H
#define ECC_LEXER_H

#if ! defined(yyFlexLexerOnce)
// FlexLexer.h is the definition for the Flex C++ scanner.
#include <FlexLexer.h>
#endif

#include <unordered_set>

// note: clangd will complain this header is missing if the build directory is clean.
#include "parser.hpp"
#include "util.hpp"


namespace ecc::frontend {

using namespace ecc::parser;
using namespace ecc::util;

class FilenamePool {
public:
    const std::string *intern(const char* s) {
        // insert returns a pair: {iterator, bool_inserted}
        auto result = pool.insert(std::string(s));
        // Return the address of the string inside the set
        return &(*result.first);
    }
private:
    std::unordered_set<std::string> pool;
};

class Lexer : public yyFlexLexer {
public:

    // Use the standard yyFlexLexer constructor.
    Lexer(std::istream *in, std::string *filename) : yyFlexLexer(in), filenames() {
        const std::string *main_file = filenames.intern(filename->c_str());
        loc.begin.filename = main_file;
        loc.end.filename = main_file;
    }

    Location loc;

    FilenamePool filenames;

    // Override the yyFlexLexer constructor.
    Parser::symbol_type get_next_token();

    void handle_linemarker(const char *yytext) {
        int line_num;
        char filename_buf[1024];
        int flag = 0;
        int chars_read;

        // Parse the mandatory parts
        if (sscanf(yytext, "# %d \"%[^\"]\"%n", &line_num, filename_buf, &chars_read) >= 2) {
            
            // 1. Intern the string to get a stable pointer
            const std::string* new_file_ptr = filenames.intern(filename_buf);

            // 2. Look for the first flag (if any)
            // We only really care about the first flag for filename tracking
            sscanf(yytext + chars_read, "%d", &flag);

            // 3. Update your tracking location
            // Both start and end of the NEXT token will be in this file
            loc.begin.filename = new_file_ptr;
            loc.end.filename   = new_file_ptr;
            
            // Update line number (subtract 1 because the NEXT line is line_num)
            loc.begin.line = line_num; 
            loc.end.line   = line_num;
        }
    }
};

}

#endif