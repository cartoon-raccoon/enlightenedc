#ifndef ECC_SYMBOLS_H
#define ECC_SYMBOLS_H

#include "semantics/types.hpp"
#include "util.hpp"

#include <unordered_map>
#include <memory>

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

/*
The abstract symbol class.
*/
class Symbol {
public:
    // The kind of symbol.
    enum Kind {
        Var, // This symbol references a variable.
        Func, // This symbol references a function definition.
        Ty, // This symbol references a declared type.
        Lab, // This symbol references a label.
    };

    Symbol(Kind kind) : kind(kind) {}

    Symbol(Kind kind, Location loc) : kind(kind), loc(loc) {}

    Kind kind;

    // std::optional<ecc::exec::Value> val;

    // The location of the symbol.
    Location loc;

    /// If the symbol is public.
    bool is_public = false;
    /// If the symbol is static.
    bool is_static = false;

    virtual ~Symbol() = default;
};

/*
A symbol representing a variable declaration.
*/
class VarSymbol : public Symbol {
public:
    VarSymbol() : Symbol(Symbol::Kind::Var), type(nullptr) {}

    VarSymbol(types::Type *type) : Symbol(Symbol::Kind::Var), type(type) {}

    /// The type of the symbol.
    types::Type *type;

    /// If the symbol is const.
    bool is_const = false;

    // Storage class information.

    /// If the symbol is external.
    bool is_extern = false;
};

/*
A symbol representing a function declaration (function pointers are handled by VarSymbol).
*/
class FuncSymbol : public Symbol {
public:
    FuncSymbol() : Symbol(Symbol::Kind::Func), signature(nullptr) {}

    FuncSymbol(types::FunctionType *signature) : Symbol(Symbol::Kind::Func), signature(signature) {}

    // The function signature.
    types::FunctionType *signature;

    bool is_extern = false;
};

/*
A symbol representing a type declaration (class, union, enum).
*/
class TypeSymbol : public Symbol {
public:
    TypeSymbol() : Symbol(Symbol::Kind::Ty), type(nullptr) {}

    TypeSymbol(types::Type* type) : Symbol(Symbol::Kind::Ty), type(type) {}

    types::Type *type;
};

/*
A symbol representing a label (for use by goto).
*/
class LabelSymbol : public Symbol {
public:
    LabelSymbol() : Symbol(Symbol::Kind::Lab) {}
};

/*
A scope within which symbols are defined.

A scope stores symbols inside a translation unit.
*/
class Scope {
public:
    Scope(Symbol *assoc, Scope *outer)
    : outer(outer), assoc(assoc), symbols(), nested() {}

    // the outer scope enclosing the inner scope.
    Scope *outer;
    // a symbol associated with this scope, usually a function.
    // if null, this is an anonymous scope.
    Symbol *assoc;
    // the symbol table.
    std::unordered_map<std::string, Box<Symbol>> symbols = {};
    // inner scopes contained within this scope.
    Vec<Box<Scope>> nested;

private:

    friend class SymbolTable;

    // The index of the next nested scope to enter.
    int nested_idx = 0;
};

/*
The symbol table, storing all symbols in a given translation unit.
*/
class SymbolTable {
public:
    SymbolTable() : global(std::make_unique<Scope>(nullptr, nullptr)), current(global.get()) {}

    // The global scope.
    Box<Scope> global;
    // The current scope.
    Scope *current;

    // Create and enter a new scope.
    void push_scope(Symbol *assoc = nullptr);

    /*
    Enter the currently indexed scope in current scope.

    Throw error if no scopes exist, or there are no more scopes left to enter.
    */
    void enter_scope();

    // Exit the current scope to the outer one.
    void pop_scope();

    // Reset the current scope to the global scope and first index.
    void reset();

    // Reset the current scope to the first index.
    void reset_current();

    // Clear the entire SymbolTable.
    void clear();

    // Lookup a symbol by name. Returns null if no symbol exists.
    Symbol *lookup(std::string sym);

    // Associate the current scope with the given Symbol `sym`.
    // If current scope is already tied to a symbol, replaces it
    // with the new one depending on value of `override`.
    void tie_current_to(Symbol *sym, bool override = false);

    // Add a new symbol to the current scope.
    Symbol *insert(std::string name, Box<Symbol> sym);

};

}

#endif