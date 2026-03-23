#include "codegen/lir/symbols.hpp"
#include "semantics/symbols.hpp"

using namespace sema::sym;
using namespace codegen::lir;

LIRVar *LIRSymbolMap::insert(PhysicalSymbol *sym, Box<LIRVar> var) {
    LIRVar *ret = var.get();
    map[sym] = std::move(var);

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

LIRVar *LIRSymbolMap::lookup(PhysicalSymbol *sym) {
    auto it = map.find(sym);

    if (it != map.end()) {
        return it->second.get();
    } else {
        return nullptr;
    }
}