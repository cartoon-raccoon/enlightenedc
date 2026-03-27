#include <memory>

#include "driver/driver.hpp"
#include "codegen/lir/lir.hpp"
#include "util.hpp"

using namespace ecc::ast;
using namespace ecc::driver;
using namespace ecc::sema::mir;
using namespace ecc::codegen;
using namespace ecc::codegen::lir;

TranslationUnit::TranslationUnit(std::string *filename, LLVMCore& llvmcore) 
: filename(filename), llvm(), ast_root(), prog_mir(), prog_lir()
{
    llvm = std::make_unique<LLVMUnit>(*filename, llvmcore);
    ast_root = std::make_unique<Program>(filename);
    prog_mir = std::make_unique<ProgramMIR>();
    prog_lir = std::make_unique<ProgramLIR>();
}

Driver::Driver(TranslationUnit& unit) : unit(unit), frontend(), backend() {
    frontend = std::make_unique<frontend::Frontend>();
    backend = std::make_unique<driver::Backend>(*unit.llvm);
}

void Driver::run() {
    frontend->run(unit);
    backend->run(unit);
}