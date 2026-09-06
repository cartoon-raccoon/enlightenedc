#pragma once

#include "semantics/symdata.hpp"
#ifndef ECC_LIR_SYMBOLS_H
#define ECC_LIR_SYMBOLS_H

#include "semantics/symbols.hpp"
#include "semantics/types.hpp"
#include "util.hpp"

using namespace ecc;
using namespace util;

namespace ecc::lower::lir {

class LIRVarSym;
class LIRFuncSym;
class FunctionLIR;

class LIRSym : public NoCopy {
public:
    enum class LIRSymKind : bool {
        FUNC,
        VAR,
    };

    LIRSym(LIRSymKind kind, Rc<sema::sym::SymData> symdata, Location loc)
        : kind(kind), symdata(std::move(symdata)), loc(loc) {}

    virtual ~LIRSym() = default;

    LIRSymKind kind;

    Rc<sema::sym::SymData> symdata;

    Optional<Location> loc;

    const std::string& get_name() const { return symdata->get_name(); }

    const std::string& get_mangled_name() const { return symdata->get_mangled_name(); }

    sema::sym::Linkage get_linkage() { return symdata->get_linkage(); }

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
    LIRVarSym(sema::sym::VarSymbol *sym)
        : LIRSym(LIRSymKind::VAR, sym->get_symdata_rc(), sym->loc), is_param(sym->is_funcparam()) {}

    LIRVarSym(sema::sym::VarSymbol *sym, LIRFuncSym *function)
        : LIRSym(LIRSymKind::VAR, sym->get_symdata_rc(), sym->loc), is_param(sym->is_funcparam()),
          function(function) {}

    // Whether the variable is a function parameter.
    bool is_param;

    /**
    The function symbol associated with this VarSym.
    */
    LIRFuncSym *function = nullptr;

    sema::sym::VarSymData *get_symdata() { return dyncast<sema::sym::VarSymData>(symdata.get()); }

    sema::types::Type *get_type() { return get_symdata()->get_type(); }

    void associate_to_funcsym(LIRFuncSym *funcsym) { function = funcsym; }

    bool is_global() const { return function == nullptr; }

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
    LIRFuncSym(sema::sym::FuncSymbol *sym)
        : LIRSym(LIRSymKind::FUNC, sym->get_symdata_rc(), sym->loc) {}

    Vec<LIRVarSym *> params;

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

    sema::sym::FuncSymData *get_symdata() const {
        return dyncast<sema::sym::FuncSymData>(symdata.get());
    }

    sema::types::FunctionType *get_signature() const { return get_symdata()->get_signature(); }

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

    LIRFuncSym *lookup_func(sema::sym::FuncSymbol *sym);

    LIRSym *lookup(sema::sym::PhysicalSymbol *sym);

private:
    HashMap<sema::sym::FuncSymbol *, Box<LIRFuncSym>> funcs;
    HashMap<sema::sym::VarSymbol *, Box<LIRVarSym>> globals;
};

} // namespace ecc::lower::lir

#endif