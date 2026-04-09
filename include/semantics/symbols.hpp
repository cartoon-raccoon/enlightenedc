#pragma once

#ifndef ECC_SYMBOLS_H
#define ECC_SYMBOLS_H

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <stack>

#include "ast/ast.hpp"
#include "eval/value.hpp"
#include "semantics/types.hpp"
#include "util.hpp"

namespace ecc::sema::sym {
/*
Symbols and Symbol Table of Ecc.

A symbol represents something that was declared within an EnlightenedC program.
This can be a type, a variable, a function, etc.

Symbols are stored within a SymbolTable. This object maps symbol names to their
corresponding Symbol objects, within Scope objects.
*/

using namespace ecc;
using namespace util;

class PhysicalSymbol;
class AbstractSymbol;
class VarSymbol;
class FuncSymbol;
class TypeSymbol;
class LabelSymbol;
class Scope;
class SymbolTableWalker;

/*
The abstract symbol class.
*/
class Symbol {
public:
    // The kind of symbol.
    enum Kind : uint8_t {
        VAR,   // This symbol references a variable.
        FUNC,  // This symbol references a function definition.
        TYPE,  // This symbol references a declared type.
        LABEL, // This symbol references a label.
    };

    Symbol(Kind kind, std::string name, Scope *scope)
        : kind(kind), name(std::move(name)), scope(scope) {}

    Symbol(Kind kind, Location loc, std::string name, Scope *scope)
        : kind(kind), name(std::move(name)), loc(loc), scope(scope) {}

    Kind kind;

    std::string name;

    // The location of the symbol.
    Location loc;

    Scope *scope;

    /// If the symbol is public.
    bool is_public = false;

    /// If the symbol is static.
    bool is_static = false;

    /// If the symbol is global.
    bool is_global = false;

    virtual ~Symbol() = default;

    virtual bool is_abstract() { return true; };

    virtual std::string to_string() const = 0;

    virtual std::string mangle() const = 0;

    virtual PhysicalSymbol *as_physical() { return nullptr; }
    virtual AbstractSymbol *as_abstract() { return nullptr; }
    virtual VarSymbol *as_varsym() { return nullptr; }
    virtual FuncSymbol *as_funcsym() { return nullptr; }
    virtual TypeSymbol *as_typesym() { return nullptr; }
    virtual LabelSymbol *as_labsym() { return nullptr; }
};

// A symbol that has a phyiscal location in memory that can be referenced
// (e.g. a variable or function).
class PhysicalSymbol : public Symbol {
public:
    PhysicalSymbol(Kind kind, std::string name, Scope *scope)
        : Symbol(kind, std::move(name), scope) {}

    PhysicalSymbol(Kind kind, Location loc, std::string name, Scope *scope)
        : Symbol(kind, std::move(name), scope) {}

    // The linkage of the symbol.
    enum class Linkage : uint8_t {
        // The symbol is only visible within the current translation unit.
        INTERNAL,
        // The symbol is visible to other translation units, and should be linked to if
        // referenced.
        EXTERNAL,
        // The symbol is visible to other translation units, and should be linked to if
        // referenced,
        // with C linkage.
        EXTERNC
    } linkage = Linkage::INTERNAL;

    bool is_external() const { return linkage != Linkage::INTERNAL; }

    PhysicalSymbol *as_physical() override { return this; }

    virtual types::Type *get_type() = 0;

    bool is_abstract() override { return false; }
};

// A symbol that is abstract, and exists only for the purposes of the compiler.
// (e.g. a label or type declaration).
class AbstractSymbol : public Symbol {
public:
    AbstractSymbol(Kind kind, std::string name, Scope *scope)
        : Symbol(kind, std::move(name), scope) {}

    AbstractSymbol(Kind kind, Location loc, std::string name, Scope *scope)
        : Symbol(kind, std::move(name), scope) {}

    AbstractSymbol *as_abstract() override { return this; }

    bool is_abstract() override { return true; }
};

/*
A symbol representing a variable declaration.
*/
class VarSymbol : public PhysicalSymbol {
public:
    VarSymbol(Location loc, std::string name, Scope *scope, types::Type *type)
        : PhysicalSymbol(Symbol::Kind::VAR, loc, std::move(name), scope), type(type) {}

    VarSymbol(Location loc, std::string name, Scope *scope, types::Type *type, eval::Value value)
        : PhysicalSymbol(Symbol::Kind::VAR, loc, std::move(name), scope), type(type), value(value) {
    }

    /// The type of the symbol.
    types::Type *type;

    // The value of the Symbol, if defined.
    Optional<eval::Value> value;

    /// If the symbol is const.
    bool is_const = false;

    // Storage class information.

    bool is_funcparam = false;

    std::string to_string() const override;

    std::string mangle() const override;

    VarSymbol *as_varsym() override { return this; }

    types::Type *get_type() override { return type; }
};

/*
A symbol representing a function declaration
(function pointers and externally linked functions are handled by VarSymbol).
*/
class FuncSymbol : public PhysicalSymbol {
public:
    FuncSymbol(Location loc, std::string name, Scope *scope, types::FunctionType *signature,
               Vec<VarSymbol *> parameters)
        : PhysicalSymbol(Symbol::Kind::FUNC, loc, std::move(name), scope), signature(signature),
          parameters(std::move(parameters)) {}

    // The function signature.
    types::FunctionType *signature;

    // The list of parameters to the function.
    Vec<VarSymbol *> parameters;

    std::string to_string() const override;

    std::string mangle() const override;

    /// Create a function pointer VarSymbol from this FuncSymbol.
    Box<VarSymbol> as_funcptr(sema::types::TypeContext& tctxt, bool is_const = false);

    FuncSymbol *as_funcsym() override { return this; }

    types::Type *get_type() override { return signature; }
};

/*
A symbol representing a type declaration (class, union, enum).
*/
class TypeSymbol : public AbstractSymbol {
public:
    TypeSymbol(Location loc, std::string name, Scope *scope, types::BaseType *type)
        : AbstractSymbol(Symbol::Kind::TYPE, loc, std::move(name), scope), type(type) {}

    types::BaseType *type;

    std::string to_string() const override;

    std::string mangle() const override;

    TypeSymbol *as_typesym() override { return this; }
};

/*
A symbol representing a label (for use by goto).
*/
class LabelSymbol : public AbstractSymbol {
public:
    LabelSymbol(Location loc, std::string name, Scope *scope)
        : AbstractSymbol(Symbol::Kind::LABEL, loc, std::move(name), scope) {}

    std::string to_string() const override;

    std::string mangle() const override;

    LabelSymbol *as_labsym() override { return this; }
};

/*
A scope within which symbols are defined.

A scope stores symbols inside a translation unit.
*/
class Scope {
public:
    Scope(FuncSymbol *assoc, Scope *outer, uint64_t id, int idx_in_nested)
        : outer(outer), assoc(assoc), id(id), idx_in_nested(idx_in_nested) {}

    Scope(FuncSymbol *assoc, Scope *outer, uint64_t id)
        : outer(outer), assoc(assoc), id(id) {}

    // the outer scope enclosing the inner scope.
    Scope *outer;
    // A function associated with this scope.
    // if null, this is an anonymous scope.
    FuncSymbol *assoc;
    // an ASTNode associated with this scope, if any.
    ast::ASTNode *node = nullptr;
    // the symbol tables.
    std::unordered_map<std::string, Box<PhysicalSymbol>> phys_symbols;
    std::unordered_map<std::string, Box<TypeSymbol>> type_symbols;
    std::unordered_map<std::string, Box<LabelSymbol>> label_symbols;
    // inner scopes contained within this scope.
    Vec<Box<Scope>> nested;

    uint64_t id;

    bool is_global() const {
        return idx_in_nested < 0;
    }

    std::string to_string() const;

private:
    friend class SymbolTable;
    friend class SymbolTableWalker;

    // The index of the next nested scope to enter.
    int idx_in_nested = -1;
};

/*
The symbol table, storing all symbols in a given translation unit.
*/
class SymbolTable {
public:
    SymbolTable() : global(std::make_unique<Scope>(nullptr, nullptr, 0)) {}

    // The global scope.
    Box<Scope> global;

    // Clear the entire SymbolTable.
    void clear();

    std::string to_string() const;
};

/**
A Walker for the Symbol Table.
*/
class SymbolTableWalker {
    Ref<SymbolTable> st;

public:
    SymbolTableWalker(SymbolTable& st)
        : st(st), current(st.global.get()) {}

    SymbolTableWalker(const SymbolTableWalker& stw)
        : st(stw.st), 
        current(stw.current), 
        next_id(stw.next_id),
        next_scope_idx(stw.next_scope_idx),
        prev_scope_idxs(stw.prev_scope_idxs) {}

    SymbolTableWalker(const SymbolTable&& stw) = delete; // fixme: implement

    Scope *current;

    Scope *global() const {
        return st.get().global.get();
    }

    // Create and enter a new scope.
    void push_scope(FuncSymbol *assoc = nullptr);

    /*
    Enter the currently indexed scope in current scope.

    Throw error if no scopes exist, or there are no more scopes left to enter.
    */
    void enter_scope();

    // Exit the current scope to the outer one.
    void pop_scope();

    // Reset the current scope to the global scope and first index.
    void reset();

    /**
    Lookup a symbol by name. Returns null of no symbol exists.
    */
    Symbol *lookup(std::string& sym, bool current = false) const;

    VarSymbol *lookup_var(std::string& sym, bool current = false) const;

    FuncSymbol *lookup_func(std::string& sym, bool current = false) const;

    TypeSymbol *lookup_type(std::string& sym, bool current = false) const;

    /*
    Look up a label from Scope `from`, up to the first function scope.

    Unlike other lookup functions, which continue on to global scope,
    `lookup_label` only recurses outwards until a function boundary.
    This is because labels are scoped to function scope specifically.
    */
    LabelSymbol *lookup_label(std::string& sym, bool current = false) const;

    // Associate the current scope with the given FuncSymbol `sym`.
    // If current scope is already tied to a symbol, replaces it
    // with the new one depending on value of `override`.
    void tie_current_to(FuncSymbol *sym, bool override = false) const;

    // Add a new symbol to the current scope.
    // Returns a pointer to the inserted symbol for further use.
    // If a symbol with the same name already exists in the current scope,
    // It throws a Location where the symbol was previously defined.
    VarSymbol *insert(std::string& name, Box<VarSymbol> sym) const;

    FuncSymbol *insert(std::string& name, Box<FuncSymbol> sym) const;

    TypeSymbol *insert(std::string& name, Box<TypeSymbol> sym) const;

    LabelSymbol *insert(std::string& name, Box<LabelSymbol> sym) const;

private:
    // The ID to assign to the next scope.
    MonotonicCtr<uint64_t> next_id = 1;

    int next_scope_idx = 0;

    std::stack<int> prev_scope_idxs;
};

} // namespace ecc::sema::sym

#endif