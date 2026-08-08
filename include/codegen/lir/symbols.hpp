#pragma once

#ifndef ECC_LIR_SYMBOLS_H
#define ECC_LIR_SYMBOLS_H

#include "semantics/symbols.hpp"
#include "semantics/types.hpp"
#include "util.hpp"

using namespace ecc;
using namespace util;

namespace ecc::codegen::lir {

class LIRVarSym;
class LIRFuncSym;
class FunctionLIR;

class LIRSym : public NoCopy {
public:
    enum class LIRSymKind : bool {
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

    bool is_var() const { return kind == LIRSymKind::VAR; }
    bool is_func() const { return kind == LIRSymKind::FUNC; }

    virtual LIRVarSym *as_varsym() { return nullptr; }
    virtual LIRFuncSym *as_funcsym() { return nullptr; }
};

/*
The representation of a physical variable (memory location) in the LIR.
*/
class LIRVarSym : public LIRSym {
public:
    LIRVarSym(
        std::string mangled, std::string name, Location loc, sema::sym::VarSymbol *sym,
        bool is_param)
        : LIRSym(LIRSymKind::VAR, std::move(mangled), std::move(name), loc), sym(sym),
          is_param(is_param) {}

    // The type of the variable.
    sema::sym::VarSymbol *sym;
    // Whether the variable is a function parameter.
    bool is_param;

    LIRVarSym *as_varsym() override { return this; }
};

/**
A function symbol for LIR.

Unlike the AST and MIR where multiple declarations (Function, FunctionMIR) can exist for a single
FuncSymbol, declarations must collapse at the LIR level, that is, there must be only one FunctionLIR
for every LIRFuncSym. To this end, each LIRFuncSym holds a back pointer to its corresponding
FunctionLIR node. This way, the and LIRFuncSym and FunctionLIR node are created the first time the
corresponding MIR node is encountered, and then reused for subsequent encounters of the same
FunctionMIR node. LIRSymbolMap is keyed by FuncSymbol, so insertion is idempotent, i.e. the same
LIRFuncSym (and thus the same FunctionLIR) is returned on subsequent insertion attempts.
*/
class LIRFuncSym : public LIRSym {
public:
    LIRFuncSym(std::string mangled, std::string name, Location loc, sema::sym::FuncSymbol *symbol)
        : LIRSym(LIRSymKind::FUNC, std::move(mangled), std::move(name), loc), symbol(symbol),
          signature(symbol->signature) {}

    sema::sym::FuncSymbol *symbol;
    sema::types::FunctionType *signature;

    /**
    Back-pointer to the corresponding FunctionLIR
    (only one `FunctionLIR` ever exists for every `LIRFuncSym`).
    */
    FunctionLIR *lir = nullptr;

    HashMap<sema::sym::VarSymbol *, Box<LIRVarSym>> map;

    LIRVarSym *insert(sema::sym::VarSymbol *sym, Box<LIRVarSym> var);

    LIRVarSym *lookup(std::string& mangled_name);

    LIRVarSym *lookup(sema::sym::VarSymbol *sym);

    LIRVarSym *operator[](sema::sym::VarSymbol *sym) { return lookup(sym); }

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
    HashMap<sema::sym::FuncSymbol *, Box<LIRFuncSym>> funcs;
    HashMap<sema::sym::VarSymbol *, Box<LIRVarSym>> globals;
};

} // namespace ecc::codegen::lir

#endif