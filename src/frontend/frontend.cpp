#include "frontend/frontend.hpp"
#include "ast/printer.hpp"
#include "frontend/lexer.hpp"
#include "parser.hpp"
#include <fstream>

using namespace ecc::frontend;

int Frontend::parse(const std::string &filename) {
    file = filename;

    std::ifstream input(filename);
    if (!input.is_open()) {
        std::cerr << "Failed to open file: " << filename << "\n";
        return 1;
    }

    ecc::ast::Program ast_root;

    ecc::frontend::Lexer lexer(&input);
    ecc::parser::Parser parser(lexer, ast_root);

    // // temp for testing
    // parser.set_debug_level(1);

    int result = parser.parse();

    if (result == 0) {
        ecc::ast::ASTPrinter printer;
        ast_root.accept(printer);
    }

    return result;
}
