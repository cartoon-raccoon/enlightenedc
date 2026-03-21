#include "driver/backend.hpp"
#include "driver/driver.hpp"
#include "semantics/semantics.hpp"
#include "semantics/mir/printer.hpp"
#include "semantics/printer.hpp"
#include "util.hpp"

using namespace ecc::driver;
using namespace ecc::sema;
using namespace ecc::frontend;
using namespace mir;

void Backend::run(TranslationUnit& unit) {
    dbprint("running backend for file ", *unit.filename);
    SemanticChecker semantic_checker(*symbols, *types);

    semantic_checker.check_semantics(*unit.ast_root, *unit.prog_mir);
    dbprint("semantic checking complete");

    dbprint(*types);
    dbprint(*symbols);
    dbprint("--------- MIR ---------\n");
    MIRPrinter printer;
    unit.prog_mir->accept(printer);
}