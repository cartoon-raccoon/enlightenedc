#ifndef ECC_DRIVER_H
#define ECC_DRIVER_H

#include <memory>
#include <string>

#include "ast/ast.hpp"
#include "frontend/frontend.hpp"
#include "driver/backend.hpp"
#include "semantics/mir/mir.hpp"
#include "util.hpp"

using namespace ecc;

namespace ecc::frontend {

struct TranslationUnit {
    std::string *filename;
    Box<ast::Program> ast_root;
    Box<sema::mir::ProgramMIR> prog_mir;

    TranslationUnit(std::string *filename) 
    : filename(filename),
    ast_root(std::make_unique<ast::Program>(filename)),
    prog_mir(std::make_unique<sema::mir::ProgramMIR>())
    {}
};

// A class for driving the compilation process for a single translation unit.
class Driver {
public:
    Box<TranslationUnit> unit;

    Box<Frontend> frontend;
    Box<driver::Backend> backend;

    Driver(std::string *filename) :
    unit(std::make_unique<TranslationUnit>(filename)),
    frontend(std::make_unique<Frontend>()),
    backend(std::make_unique<driver::Backend>())
    {}

    void run();
};

}

#endif