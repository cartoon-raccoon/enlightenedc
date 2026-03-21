#ifndef ECC_LIR_SYMBOLS_H
#define ECC_LIR_SYMBOLS_H

#include <unordered_map>

#include "semantics/types.hpp"
#include "semantics/symbols.hpp"
#include "util.hpp"

using namespace ecc;
using namespace util;

namespace ecc::codegen::lir {

/*
The representation of a physical variable (memory location) in the LIR.
*/
struct LIRVar {
    // The mangled name of the variable (for global uniqueness).
    std::string mangled_name;
    // The name of the variable as declared in the source code.
    std::string name;
    // The type of the variable.
    sema::types::Type *type;
    // The location of the variable as declared in the source code.
    Location loc;
    // Whether the variable is a function.
    bool is_function;
    // Whether the variable is a function parameter.
    bool is_param;
};

/*
A scope-unaware, "flat" Symbol-to-LIRVar mapping.
*/
class LIRSymbolMap {
public:
    LIRSymbolMap() : map() {}

    LIRVar *insert(sema::sym::VarSymbol *varsym, Box<LIRVar> var);

    LIRVar *lookup(std::string& mangled_name);

    LIRVar *lookup(sema::sym::VarSymbol *varsym);

private:
    std::unordered_map<sema::sym::VarSymbol *, Box<LIRVar>> map;

};

}

#endif