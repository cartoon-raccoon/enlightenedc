#include "frontend/frontend.hpp"
#include "ast/printer.hpp"
#include "frontend/lexer.hpp"
#include "parser.hpp"
#include "preproc/preproc.hpp"
#include <sstream>

using namespace ecc::frontend;

int Frontend::parse(const std::string& filename) {
    file = filename;

    try {
        ecc::preproc::PreProcessor preproc;
        std::string preprocessed = preproc.run(filename);

        std::cout << preprocessed << "\n\n\n";
        std::istringstream input(preprocessed);


        ecc::ast::Program ast_root;

        ecc::frontend::Lexer lexer(&input);
        ecc::parser::Parser parser(lexer, ast_root);

        // temp for testing
        parser.set_debug_level(1);

        int result = parser.parse();

        if (result == 0) {
            ecc::ast::ASTPrinter printer;
            ast_root.accept(printer);
        }

        return result;

    } catch (const std::exception& e) {
        std::cerr << "Preprocessor error: " << e.what() << "\n";
        return 1;
    }
}
