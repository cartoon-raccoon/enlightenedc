#include "codegen/lir/symbols.hpp"

using namespace sema::sym;
using namespace codegen::lir;

LIRVar *LIRSymbolMap::insert(VarSymbol *varsym, Box<LIRVar> var) {
    LIRVar *ret = var.get();
    map[varsym] = std::move(var);

    return ret;
}

LIRVar *LIRSymbolMap::lookup(std::string& mangled_name) {
    for (auto& [sym, var] : map) {
        if (var->mangled_name == mangled_name) {
            return var.get();
        }
    }

    return nullptr;
}

LIRVar *LIRSymbolMap::lookup(VarSymbol *varsym) {
    auto it = map.find(varsym);

    if (it != map.end()) {
        return it->second.get();
    } else {
        return nullptr;
    }
}