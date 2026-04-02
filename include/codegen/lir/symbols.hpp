#pragma once

#ifndef ECC_LIR_SYMBOLS_H
#define ECC_LIR_SYMBOLS_H

#include <unordered_map>

#include "semantics/symbols.hpp"
#include "util.hpp"

using namespace ecc;
using namespace util;

namespace ecc::codegen::lir {

class LIRVarSym;
class LIRFuncSym;

class LIRSym : public NoCopy {
public:
    enum class LIRSymKind {
        FUNC,
        VAR,
    };

    LIRSym(LIRSymKind kind, std::string mangled, std::string name, Location loc)
        : kind(kind), mangled_name(std::move(mangled)), name(std::move(name)), loc(loc) {}

    virtual ~LIRSym() = default;

    LIRSymKind kind;

    std::string mangled_name;

    std::string name;

    Location loc;

    virtual LIRVarSym *as_varsym() { return nullptr; }
    virtual LIRFuncSym *as_funcsym() { return nullptr; }
};

/*
The representation of a physical variable (memory location) in the LIR.
*/
class LIRVarSym : public LIRSym {
public:
    LIRVarSym(std::string mangled, 
              std::string name, 
              Location loc, 
              sema::sym::VarSymbol *sym, 
              bool is_param)
        : LIRSym(LIRSymKind::VAR, mangled, name, loc), sym(sym), is_param(is_param) {}

    // The type of the variable.
    sema::sym::VarSymbol *sym;
    // Whether the variable is a function parameter.
    bool is_param;

    LIRVarSym *as_varsym() override { return this; }
};

class LIRFuncSym : public LIRSym {
public:
    LIRFuncSym(std::string mangled, 
               std::string name,
               Location loc, 
               sema::sym::FuncSymbol *symbol)
        : LIRSym(LIRSymKind::FUNC, mangled, name, loc), symbol(symbol) {}

    sema::sym::FuncSymbol *symbol;

    std::unordered_map<sema::sym::VarSymbol *, Box<LIRVarSym>> map;

    LIRVarSym *insert(sema::sym::VarSymbol *sym, Box<LIRVarSym> var);

    LIRVarSym *lookup(std::string& mangled_name);

    LIRVarSym *lookup(sema::sym::VarSymbol *sym);

    LIRVarSym * operator[] (sema::sym::VarSymbol *sym) {
        return lookup(sym);
    }

    LIRFuncSym *as_funcsym() override { return this; }
};

/*
A function-scoped mapping of symbols to their respective functions.
*/
class LIRSymbolMap {
public:
    LIRSymbolMap() {}

    LIRFuncSym *add_function(sema::sym::FuncSymbol *funcsym, Box<LIRFuncSym> func);

    LIRVarSym *insert_global(sema::sym::VarSymbol *sym, Box<LIRVarSym> var);

    LIRSym *lookup(sema::sym::PhysicalSymbol *sym);

private:
    std::unordered_map<sema::sym::FuncSymbol *, Box<LIRFuncSym>> funcs;
    std::unordered_map<sema::sym::VarSymbol *, Box<LIRVarSym>> globals;
};

}

#endif