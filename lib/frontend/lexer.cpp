#include "frontend/lexer.hpp"

#include "driver/filenames.hpp"

using namespace ecc::frontend;

constexpr size_t FILENAME_BUF_SIZE = 2048;

LexerHack::LexerHack() {
    // push the global scope.
    push_scope();
}

bool LexerHack::contains_type(StringRef ident) {
    for (auto sc = stack.rbegin(); sc != stack.rend(); sc++) {
        if (sc->typedefs.contains(ident)) return true;
    }

    return false;
}

// Use the standard yyFlexLexer constructor.
Lexer::Lexer(
    std::istream *in, std::string *filename, LexerHack& typedefs,
    driver::FilenamePool& filenames)
    : yyFlexLexer(in), typedefs(typedefs), filenames(filenames) {
    const std::string *main_file = this->filenames.get().intern(filename->c_str());
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
        const std::string *new_file_ptr = filenames.get().intern(filename_buf);

        // 2. Look for the first flag (if any)
        // We only really care about the first flag for filename tracking
        sscanf(yytext + chars_read, "%d", &flag);

        // 3. Update your tracking location
        // Both start and end of the NEXT token will be in this file
        loc.end = Point(new_file_ptr, 1, line_num);
        loc.step();
    }
}