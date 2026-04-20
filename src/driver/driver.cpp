#include "driver/driver.hpp"

#include <memory>

#include "codegen/lir/lir.hpp"
#include "codegen/lir/symbols.hpp"
#include "semantics/symbols.hpp"
#include "semantics/types.hpp"
#include "util.hpp"

using namespace ecc::ast;
using namespace ecc::driver;
using namespace ecc::sema::mir;
using namespace sema::types;
using namespace sema::sym;
using namespace ecc::codegen;
using namespace codegen::lir;

TranslationUnitMIR::TranslationUnitMIR()
    : symbols(make_box<SymbolTable>()), mir(make_box<ProgramMIR>()) {
}

TranslationUnitLIR::TranslationUnitLIR()
    : symbols(make_box<LIRSymbolMap>()), lir(make_box<ProgramLIR>()) {
}

TranslationUnit::TranslationUnit(std::string *filename, LLVMCore& llvmcore) : filename(filename) {
    llvm     = make_box<LLVMUnit>(*filename, llvmcore);
    types    = make_box<TypeContext>(*llvm);
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