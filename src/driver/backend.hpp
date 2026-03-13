#ifndef ECC_BACKEND_H
#define ECC_BACKEND_H

#include <memory>

#include "semantics/types.hpp"
#include "semantics/symbols.hpp"

namespace ecc::frontend {
    struct TranslationUnit;
}

#include "util.hpp"

namespace ecc::driver {

class Backend {
public:
    Box<sema::sym::SymbolTable> symbols;
    Box<sema::types::TypeContext> types;

    Backend()
    : 
    symbols(std::make_unique<sema::sym::SymbolTable>()), 
    types(std::make_unique<sema::types::TypeContext>()) {}

    void run(frontend::TranslationUnit& unit);
};

}

#endif