#ifndef ECC_BACKEND_H
#define ECC_BACKEND_H

#include <memory>

#include "semantics/types.hpp"
#include "semantics/symbols.hpp"
#include "codegen/llvm.hpp"

#include "util.hpp"

namespace ecc::driver {

struct TranslationUnit;

class Backend {
public:
    Box<sema::sym::SymbolTable> symbols;
    Box<sema::types::TypeContext> types;

    Backend(codegen::LLVMUnit& llvm)
    : 
    symbols(std::make_unique<sema::sym::SymbolTable>()), 
    types(std::make_unique<sema::types::TypeContext>(llvm)) {}

    void run(TranslationUnit& unit);
};

}

#endif