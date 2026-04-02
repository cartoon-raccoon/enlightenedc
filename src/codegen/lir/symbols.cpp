#include "codegen/lir/symbols.hpp"
#include "semantics/symbols.hpp"

using namespace sema::sym;
using namespace codegen::lir;

LIRVarSym *LIRFuncSym::insert(VarSymbol *sym, Box<LIRVarSym> var) {
    LIRVarSym *ret = var.get();
    map[sym] = std::move(var);

    return ret;
}

LIRVarSym *LIRFuncSym::lookup(std::string& mangled_name) {
    for (auto& [sym, var] : map) {
        if (var->mangled_name == mangled_name) {
            return var.get();
        }
    }

    return nullptr;
}

LIRVarSym *LIRFuncSym::lookup(VarSymbol *sym) {
    auto it = map.find(sym);

    if (it != map.end()) {
        return it->second.get();
    } else {
        return nullptr;
    }
}

LIRFuncSym *LIRSymbolMap::add_function(sema::sym::FuncSymbol *funcsym, Box<LIRFuncSym> func) {
    LIRFuncSym *ret = func.get();
    funcs[funcsym] = std::move(func);

    return ret;
}

LIRVarSym *LIRSymbolMap::insert_global(sema::sym::VarSymbol *sym, Box<LIRVarSym> var) {
    LIRVarSym *ret = var.get();
    globals[sym] = std::move(var);

    return ret;
}

LIRSym *LIRSymbolMap::lookup(sema::sym::PhysicalSymbol *sym) {
    todo();
}