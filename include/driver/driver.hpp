#ifndef ECC_DRIVER_H
#define ECC_DRIVER_H

#include <string>

#include "ast/ast.hpp"
#include "codegen/lir/lir.hpp"
#include "codegen/lir/symbols.hpp"
#include "codegen/llvm.hpp"
#include "driver/backend.hpp"
#include "frontend/frontend.hpp"
#include "semantics/mir/mir.hpp"
#include "semantics/symbols.hpp"
#include "semantics/types.hpp"
#include "util.hpp"

using namespace ecc;
using namespace util;

namespace ecc::driver {

/**
The part of the translation unit storing the MIR intermediate representation.

The MIR encompasses two core data structures: the SymbolTable,
and the MIR tree itself.
*/
struct TranslationUnitMIR {
    TranslationUnitMIR();

    Box<sema::sym::SymbolTable> symbols;
    Box<sema::mir::ProgramMIR> mir;
};

/**
The part of the translation unit storing the LIR intermediate representation.
*/
struct TranslationUnitLIR {
    TranslationUnitLIR();

    Box<codegen::lir::LIRSymbolMap> symbols;
    Box<codegen::lir::ProgramLIR> lir;
};

/**
A single translation unit that is compiled into an object file.

The translation unit stores the per-unit LLVM state, the parsed AST,
and the IR trees, as well as the TypeContext, which is stored at this
level since both MIR and LIR depend on it.
*/
class TranslationUnit {
public:
    TranslationUnit(std::string *filename, codegen::LLVMCore& llvmcore);

    std::string *filename;
    Box<codegen::LLVMUnit> llvm;
    Box<sema::types::TypeContext> types;

    Box<ast::Program> ast_root;
    Box<TranslationUnitMIR> prog_mir;
    Box<TranslationUnitLIR> prog_lir;
};

// A class for driving the compilation process for a single translation unit.
class Driver {
public:
    TranslationUnit& unit;
    Box<frontend::Frontend> frontend;
    Box<driver::Backend> backend;

    Driver(TranslationUnit& unit);
    ~Driver() = default;

    void run();
};

} // namespace ecc::driver

#endif