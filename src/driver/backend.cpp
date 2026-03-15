#include "driver/backend.hpp"
#include "driver/driver.hpp"
#include "semantics/semantics.hpp"
#include "util.hpp"

using namespace ecc::driver;
using namespace ecc::sema;
using namespace ecc::frontend;

void Backend::run(TranslationUnit& unit) {
    dbprint("running backend for file ", *unit.filename);
    SemanticChecker semantic_checker(*symbols, *types);

    semantic_checker.check_semantics(*unit.ast_root);

    dbprint(symbols);
    dbprint(types);
}