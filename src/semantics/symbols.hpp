#ifndef ECC_SYMBOLS_H
#define ECC_SYMBOLS_H

#include "ast/ast.hpp"
#include "semantics/types.hpp"
#include "eval/value.hpp"
#include "util.hpp"

#include <cstdint>
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

class PhysicalSymbol;
class AbstractSymbol;
class VarSymbol;
class FuncSymbol;
class TypeSymbol;
class LabelSymbol;
class Scope;

/*
The abstract symbol class.
*/
class Symbol {
public:
    // The kind of symbol.
    enum Kind {
        VAR, // This symbol references a variable.
        FUNC, // This symbol references a function definition.
        TYPE, // This symbol references a declared type.
        LABEL, // This symbol references a label.
    };

    Symbol(Kind kind, std::string name, Scope *scope)
        : kind(kind), name(name), scope(scope) {}

    Symbol(Kind kind, Location loc, std::string name, Scope *scope)
        : kind(kind), name(name), loc(loc), scope(scope) {}

    Kind kind;

    std::string name;

    // The location of the symbol.
    Location loc;

    Scope *scope;

    /// If the symbol is public.
    bool is_public = false;
    /// If the symbol is static.
    bool is_static = false;

    virtual ~Symbol() = default;

    virtual bool is_abstract() { return true; };

    virtual std::string to_string() const  = 0;

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
    PhysicalSymbol(Kind kind, std::string name, Scope *scope) : Symbol(kind, name, scope) {}

    PhysicalSymbol(Kind kind, Location loc, std::string name, Scope *scope) : Symbol(kind, name, scope) {}

    // If the symbol is external.
    bool is_extern = false;

    // If the symbol is externally linked.
    bool is_externc = false;

    PhysicalSymbol *as_physical() { return this; }

    virtual types::Type *get_type() = 0;

    bool is_abstract() { return false; }
};

// A symbol that is abstract, and exists only for the purposes of the compiler.
// (e.g. a label or type declaration).
class AbstractSymbol : public Symbol {
public:
    AbstractSymbol(Kind kind, std::string name, Scope *scope) : Symbol(kind, name, scope) {}

    AbstractSymbol(Kind kind, Location loc, std::string name, Scope *scope) : Symbol(kind, name, scope) {}

    AbstractSymbol *as_abstract() { return this; }

    bool is_abstract() { return true; }
};

/*
A symbol representing a variable declaration.
*/
class VarSymbol : public PhysicalSymbol {
public:
    VarSymbol(Location loc, std::string name, Scope *scope, types::Type *type) 
        : PhysicalSymbol(Symbol::Kind::VAR, loc, name, scope), type(type) {}

    VarSymbol(Location loc, std::string name, Scope *scope, types::Type *type, exec::Value value) 
        : PhysicalSymbol(Symbol::Kind::VAR, loc, name, scope), type(type), value(value) {}

    /// The type of the symbol.
    types::Type *type;

    // The value of the Symbol, if defined.
    Optional<exec::Value> value;

    /// If the symbol is const.
    bool is_const = false;

    // Storage class information.

    bool is_funcparam = false;

    virtual std::string to_string() const override;

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
    FuncSymbol(Location loc, 
               std::string name,
               Scope *scope,
               types::FunctionType *signature, 
               Vec<VarSymbol *> parameters)
        : PhysicalSymbol(Symbol::Kind::FUNC, loc, name, scope), 
        signature(signature),
        parameters(std::move(parameters)) {}

    // The function signature.
    types::FunctionType *signature;

    // The list of parameters to the function.
    Vec<VarSymbol *> parameters;

    virtual std::string to_string() const override;

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
    TypeSymbol(Location loc, std::string name, Scope *scope, types::BaseType* type)
        : AbstractSymbol(Symbol::Kind::TYPE, loc, name, scope), type(type) {}

    types::BaseType *type;

    virtual std::string to_string() const override;

    std::string mangle() const override;

    TypeSymbol *as_typesym() override { return this; }
};

/*
A symbol representing a label (for use by goto).
*/
class LabelSymbol : public AbstractSymbol {
public:
    LabelSymbol(Location loc, std::string name, Scope *scope)
        : AbstractSymbol(Symbol::Kind::LABEL, loc, name, scope) {}

    virtual std::string to_string() const override;

    std::string mangle() const override;

    LabelSymbol *as_labsym() override { return this; }
};

/*
A scope within which symbols are defined.

A scope stores symbols inside a translation unit.
*/
class Scope {
public:
    Scope(FuncSymbol *assoc, Scope *outer, uint64_t id)
    : outer(outer), 
    assoc(assoc), 
    phys_symbols(), type_symbols(), label_symbols(),
    nested(), id(id) {}

    // the outer scope enclosing the inner scope.
    Scope *outer;
    // A function associated with this scope.
    // if null, this is an anonymous scope.
    FuncSymbol *assoc;
    // an ASTNode associated with this scope, if any.
    ast::ASTNode *node;
    // the symbol tables.
    std::unordered_map<std::string, Box<PhysicalSymbol>> phys_symbols = {};
    std::unordered_map<std::string, Box<TypeSymbol>> type_symbols = {};
    std::unordered_map<std::string, Box<LabelSymbol>> label_symbols = {};
    // inner scopes contained within this scope.
    Vec<Box<Scope>> nested;

    uint64_t id;

    std::string to_string() const;

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
    SymbolTable() : global(std::make_unique<Scope>(nullptr, nullptr, 0)), current(global.get()) {}

    // The global scope.
    Box<Scope> global;
    // The current scope.
    Scope *current;

    // The ID to assign to the next scope.
    uint64_t next_id = 1;

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

    // Reset the given scope and all nested scopes.
    void reset_from(Scope *scope);

    // Reset the current scope to the first index.
    void reset_current();

    // Clear the entire SymbolTable.
    void clear();

    /**
    Lookup a symbol by name. Returns null of no symbol exists.
    */
    Symbol *lookup(std::string& sym, bool current = false);

    /**
    Lookup a symbol by name from Scope `from`. Returns null if no symbol exists.
    */
    Symbol *lookup(std::string& sym, Scope *from, bool current = false);

    VarSymbol *lookup_var(std::string& sym, bool current = false);

    VarSymbol *lookup_var(std::string& sym, Scope *from, bool current = false);

    FuncSymbol *lookup_func(std::string& sym, bool current = false);

    FuncSymbol *lookup_func(std::string& sym, Scope *from, bool current = false);

    TypeSymbol *lookup_type(std::string& sym, bool current = false);

    TypeSymbol *lookup_type(std::string& sym, Scope *from, bool current = false);

    /*
    Look up a label from Scope `from`, up to the first function scope.

    Unlike other lookup functions, which continue on to global scope,
    `lookup_label` only recurses outwards until a function boundary.
    This is because labels are scoped to function scope specifically.
    */
    LabelSymbol *lookup_label(std::string& sym, bool current = false);

    /*
    Look up a label up to the first function scope.

    Unlike other lookup functions, which continue on to global scope,
    `lookup_label` only recurses outwards until a function boundary.
    This is because labels are scoped to function scope specifically.
    */
    LabelSymbol *lookup_label(std::string& sym, Scope *from, bool current = false);

    // Associate the current scope with the given FuncSymbol `sym`.
    // If current scope is already tied to a symbol, replaces it
    // with the new one depending on value of `override`.
    void tie_current_to(FuncSymbol *sym, bool override = false);

    // Add a new symbol to the current scope.
    // Returns a pointer to the inserted symbol for further use.
    // If a symbol with the same name already exists in the current scope,
    // It throws a Location where the symbol was previously defined.
    VarSymbol *insert(std::string name, Box<VarSymbol> sym);

    FuncSymbol *insert(std::string name, Box<FuncSymbol> sym);

    TypeSymbol *insert(std::string name, Box<TypeSymbol> sym);

    LabelSymbol *insert(std::string name, Box<LabelSymbol> sym);


    std::string to_string() const;

};

}

#endif