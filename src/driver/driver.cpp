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
    : symbols(std::make_unique<SymbolTable>()), mir(std::make_unique<ProgramMIR>()) {
}

TranslationUnitLIR::TranslationUnitLIR()
    : symbols(std::make_unique<LIRSymbolMap>()), lir(std::make_unique<ProgramLIR>()) {
}

TranslationUnit::TranslationUnit(std::string *filename, LLVMCore& llvmcore)
    : filename(filename), llvm(), types(), ast_root(), prog_mir(), prog_lir() {
    llvm     = std::make_unique<LLVMUnit>(*filename, llvmcore);
    types    = std::make_unique<TypeContext>(*llvm);
    ast_root = std::make_unique<Program>(filename);
    prog_mir = std::make_unique<TranslationUnitMIR>();
    prog_lir = std::make_unique<TranslationUnitLIR>();
}

Driver::Driver(TranslationUnit& unit) : unit(unit), frontend(), backend() {
    frontend = std::make_unique<frontend::Frontend>();
    backend  = std::make_unique<driver::Backend>();
}

void Driver::run() {
    frontend->run(unit);
    backend->run(unit);
}