#include "ast/printer.hpp"
#include "driver/driver.hpp"
#include "frontend/frontend.hpp"
#include "frontend/lexer.hpp"
#include "parser.hpp"
#include "preproc/preproc.hpp"
#include "error.hpp"
#include "util.hpp"

#include <sstream>

using namespace ecc::frontend;

void Frontend::parse(TranslationUnit& unit) {
    dbprint("parsing file ", *unit.filename);
    try {
        ecc::preproc::PreProcessor preproc;
        std::string preprocessed = preproc.run(unit.filename);

        std::cout << preprocessed << "\n\n\n";
        std::istringstream input(preprocessed);

        ecc::frontend::Lexer lexer(&input);
        ecc::parser::Parser parser(lexer, *unit.ast_root);

        // temp for testing
        //parser.set_debug_level(1);

        int result = parser.parse();

        if (result == 0) {
            dbprint("printing AST for file ", *unit.filename);
            ast::ASTPrinter printer;
            unit.ast_root->accept(printer);
        }

        if (result != 0) {
            throw EccError("Parsing exited with error");
        }

    } catch (const std::exception& e) {
        std::cerr << "Preprocessor error: " << e.what() << "\n";
        throw EccError("preprocessor exited with error");
    }
}
