#include "frontend/lexer.hpp"

using namespace ecc::frontend;

constexpr size_t FILENAME_BUF_SIZE = 2048;

// Use the standard yyFlexLexer constructor.
Lexer::Lexer(std::istream *in, std::string *filename, std::set<std::string>& typedefs)
    : yyFlexLexer(in), typedefs(typedefs) {
    const std::string *main_file = filenames.intern(filename->c_str());
    loc.begin.filename           = main_file;
    loc.end.filename             = main_file;
}

void Lexer::handle_linemarker(const char *yytext) {
    int line_num;
    char filename_buf[FILENAME_BUF_SIZE];
    int flag = 0;
    int chars_read;

    // Parse the mandatory parts
    if (sscanf(yytext, "# %d \"%[^\"]\"%n", &line_num, filename_buf, &chars_read) >= 2) {

        // 1. Intern the string to get a stable pointer
        const std::string *new_file_ptr = filenames.intern(filename_buf);

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