#include "driver/driver.hpp"
#include "util.hpp"

using namespace ecc::driver;

TranslationUnit::TranslationUnit(std::string *filename) 
: filename(filename), ast_root(), prog_mir(), llvm() {
    ast_root = std::make_unique<ast::Program>(filename);
    prog_mir = std::make_unique<sema::mir::ProgramMIR>();
    llvm = std::make_unique<codegen::LLVM>(*filename);
}

Driver::Driver(TranslationUnit& unit) : unit(unit), frontend(), backend() {
    frontend = std::make_unique<frontend::Frontend>();
    backend = std::make_unique<driver::Backend>(*unit.llvm);
}

void Driver::run() {
    frontend->parse(unit);
    backend->run(unit);
}