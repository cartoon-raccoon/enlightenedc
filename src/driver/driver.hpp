#ifndef ECC_DRIVER_H
#define ECC_DRIVER_H

#include <memory>
#include <string>

#include "ast/ast.hpp"
#include "frontend/frontend.hpp"
#include "driver/backend.hpp"
#include "semantics/mir/mir.hpp"
#include "codegen/llvm.hpp"
#include "util.hpp"

using namespace ecc;

namespace ecc::driver {

struct TranslationUnit {
    std::string *filename;
    Box<ast::Program> ast_root;
    Box<sema::mir::ProgramMIR> prog_mir;
    Box<codegen::LLVM> llvm;

    TranslationUnit(std::string *filename) 
    : filename(filename),
    ast_root(std::make_unique<ast::Program>(filename)),
    prog_mir(std::make_unique<sema::mir::ProgramMIR>()),
    llvm(std::make_unique<codegen::LLVM>(*filename))
    {}
};

// A class for driving the compilation process for a single translation unit.
class Driver {
public:

    TranslationUnit& unit;
    Box<frontend::Frontend> frontend;
    Box<driver::Backend> backend;

    Driver(TranslationUnit& unit) 
        : unit(unit),
        frontend(std::make_unique<frontend::Frontend>()),
        backend(std::make_unique<driver::Backend>(*unit.llvm))
    {}

    void run();
};

}

#endif