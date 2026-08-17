#include "lowering/lir/symbols.hpp"

#include "semantics/symbols.hpp"

using namespace sema::sym;
using namespace lower::lir;

LIRVarSym *LIRFuncSym::insert(VarSymbol *sym, Box<LIRVarSym> var) {
    LIRVarSym *ret = var.get();
    map[sym]       = std::move(var);

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

    return it != map.end() ? it->second.get() : nullptr;
}

LIRFuncSym *LIRSymbolMap::add_function(FuncSymbol *funcsym, Box<LIRFuncSym> func) {
    auto it = funcs.find(funcsym);

    if (it != funcs.end())
        return it->second.get(); // reuse, discard the fresh one

    LIRFuncSym *ret = func.get();
    funcs[funcsym]  = std::move(func);
    return ret;
}

LIRVarSym *LIRSymbolMap::insert_global(VarSymbol *sym, Box<LIRVarSym> var) {
    LIRVarSym *ret = var.get();
    globals[sym]   = std::move(var);

    return ret;
}

LIRFuncSym *LIRSymbolMap::lookup_func(FuncSymbol *sym) {
    if (!funcs.contains(sym)) return nullptr;

    return funcs[sym].get();
}

LIRSym *LIRSymbolMap::lookup(sema::sym::PhysicalSymbol *sym) {
    if (sym->is_var()) {
        // if sym is varsym, check globals and then functions

        auto *varsym = sym->as_varsym();

        assert(varsym);

        // first search globals
        auto it = globals.find(varsym);
        if (it != globals.end()) {
            return it->second.get();
        }

        // if not found, search in functions
        for (auto& [_, lirfunc] : funcs) {
            auto *lirsym = lirfunc->lookup(varsym);
            if (lirsym) {
                return lirsym;
            }
        }
    } else if (sym->is_func()) {
        // if sym is funcsym, just search functions

        auto it = funcs.find(sym->as_funcsym());
        if (it != funcs.end()) {
            return it->second.get();
        }
    }

    return nullptr;
}