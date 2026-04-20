#include "frontend/frontend.hpp"

#include <set>

#include "ast/printer.hpp"
#include "driver/driver.hpp"
#include "frontend/lexer.hpp"
#include "frontend/preproc.hpp"
#include "parser.hpp"
#include "util.hpp"

using namespace ecc::frontend;

void Frontend::run(Ecc& ecc, driver::TranslationUnit& unit) {
    dbprint("parsing file ", *unit.filename);
    Preprocessor preproc(unit.filename);

    // bodge for lexer hack
    std::set<std::string> typedefs{};

    Lexer lexer(&preproc, unit.filename, typedefs, ecc.filenames);
    Parser parser(lexer, *unit.ast_root, typedefs);

    try {

        // temp for testing
        // parser.set_debug_level(1);
        parser.parse();
    } catch (ParseError& e) {
        // the preprocessor exiting before processing the entire file
        // will also result in a parse error, so if we catch a parse error,
        // we try to close the preprocessor and see if it throws something.
        try {
            preproc.close();
        } catch (PreprocError& pperr) {
            throw pperr;
        }
        throw e;
    }
    preproc.close();

    // todo: hide this behind an if guard
    dbprint("printing AST for file ", *unit.filename);
    ast::ASTPrinter printer;
    unit.ast_root->accept(printer);
}
