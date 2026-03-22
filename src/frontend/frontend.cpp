#include <set>

#include "ast/printer.hpp"
#include "driver/driver.hpp"
#include "frontend/frontend.hpp"
#include "frontend/lexer.hpp"
#include "parser.hpp"
#include "preproc/preproc.hpp"
#include "error.hpp"
#include "util.hpp"

using namespace ecc::frontend;

void Frontend::parse(driver::TranslationUnit& unit) {
    dbprint("parsing file ", *unit.filename);
    try {
        ecc::preproc::PreProcessor preproc(unit.filename);

        // bodge for lexer hack
        std::set<std::string> typedefs {};

        ecc::frontend::Lexer lexer(&preproc, unit.filename, typedefs);
        ecc::parser::Parser parser(lexer, *unit.ast_root, typedefs);

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
