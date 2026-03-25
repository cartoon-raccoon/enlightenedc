#ifndef ECC_DRIVER_H
#define ECC_DRIVER_H

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

    TranslationUnit(std::string *filename);
};

// A class for driving the compilation process for a single translation unit.
class Driver {
public:

    TranslationUnit& unit;
    Box<frontend::Frontend> frontend;
    Box<driver::Backend> backend;

    Driver(TranslationUnit& unit);

    void run();
};

}

#endif