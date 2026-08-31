#include "driver/driver.hpp"

#include <memory>

#include "codegen/codegen.hpp"
#include "lowering/lir/lir.hpp"
#include "lowering/lir/symbols.hpp"
#include "semantics/symbols.hpp"
#include "semantics/types.hpp"
#include "util.hpp"

using namespace ecc::ast;
using namespace ecc::driver;
using namespace ecc::sema::mir;
using namespace sema::types;
using namespace sema::sym;
using namespace ecc::codegen;
using namespace lower::lir;
using namespace lower::cfg;

TranslationUnitMIR::TranslationUnitMIR()
    : symbols(make_box<SymbolTable>()), mir(make_box<ProgramMIR>()) {
}

TranslationUnitLIR::TranslationUnitLIR()
    : symbols(make_box<LIRSymbolMap>()), lir(make_box<ProgramLIR>()), cfg(make_box<ProgramCFG>()) {
}

TranslationUnit::TranslationUnit(std::string *filename, CodeGenCore& cgcore) : filename(filename) {
    cgu      = cgcore.make_unit(*filename);
    types    = make_box<TypeContext>(*cgu);
    ast_root = make_box<Program>(filename);
    prog_mir = make_box<TranslationUnitMIR>();
    prog_lir = make_box<TranslationUnitLIR>();
}

Driver::Driver(TranslationUnit& unit) : unit(unit) {
    frontend = make_box<frontend::Frontend>();
    backend  = make_box<driver::Backend>();
}

void Driver::run(Ecc& ecc) {
    frontend->run(ecc, unit);
    backend->run(ecc, unit);
}