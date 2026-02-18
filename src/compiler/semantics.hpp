#ifndef ECC_SEMANTICS_H
#define ECC_SEMANTICS_H

#include "ast/visitor.hpp"
#include "compiler/types.hpp"
#include "util.hpp"

#include <map>

namespace ecc::compiler {

using namespace util;
/*
Semantic validation functionality.

Semantic Checking in Ecc occurs in two passes: Elaboration and Validation.

The elaboration pass walks the AST, populating the TypeContext and SymbolTable.

The validation pass then walks the AST, performing type checking and semantic
validation (e.g. no invalid struct member accesses).
*/

using namespace ecc;

/*
The abstract symbol class.
*/
class Symbol {
public:
    virtual ~Symbol() = default;
};

/*
The symbol table, storing all symbols in a given translation unit.
*/
class SymbolTable {
public:
    class Scope {
    public:
        Scope(Symbol *assoc) : outer(nullptr), assoc(assoc), symbols(), inners() {}
        // the outer scope enclosing the inner scope.
        Scope *outer;
        // a symbol associated with this scope, usually a function.
        // if null, this is an anonymous scope.
        Symbol *assoc;
        // the symbol table.
        std::map<std::string, Box<Symbol>> symbols = {};
        // inner scopes contained within this scope.
        Vec<Box<Scope>> inners;
    }; // class Scope

    SymbolTable() : global(std::make_unique<Scope>(nullptr)), current(global.get()) {}

    // The global scope.
    Box<Scope> global;
    // The current scope.
    Scope *current;
    // Create and enter a new scope.
    void push_scope(Symbol *assoc = nullptr);
    // Exit the current scope to the outer one.
    void pop_scope();
    // Lookup a symbol by name.
    Symbol *lookup(std::string sym);
    // Add a new symbol to the current scope.
    void insert(std::string name, Box<Symbol> sym);

};


/*
The base semantic walker class that handles scoping and AST walking.

The BaseSemanticVisitor class handles scoping within a SymbolTable and the basic
AST walking operations, overriding all `visit()` member functions in the abstract
`ast::ASTVisitor` base class. As such, a BaseSemanticVisitor simply walks the AST
without doing anything on the nodes, pushing and popping scopes as necessary.

Each class implementing a semantic validation pass should inherit from this class.
overriding the `visit()` member functions that concern them.
*/
class BaseSemanticVisitor : public ast::ASTVisitor {
public:
    /*
    An RAII wrapper for automatically pushing and popping scopes on a symbol table.

    When a ScopeGuard is created, it is supplied with a reference to a symbol table.
    The ScopeGuard pushes a new scope into the symbol table.

    When the destructor is called, the ScopeGuard automatically pops the scope from its
    stored symbol table reference.
    */
    class ScopeGuard {
    public:
        ScopeGuard(SymbolTable& st, Symbol *assoc) : st(st) {
            st.push_scope(assoc);
        }
    
        ~ScopeGuard() {
            st.pop_scope();
        }
    
        ScopeGuard(const ScopeGuard&) = delete;
        ScopeGuard& operator=(const ScopeGuard&) = delete;
    private:
        SymbolTable& st;
    }; // class ScopeGuard

    BaseSemanticVisitor(SymbolTable& syms, types::TypeContext& types)
    : syms(syms), types(types) {}

    // A reference to a symbol table.
    SymbolTable& syms;
    types::TypeContext& types;

    // Create and enter a new scope, supplying an associated symbol if needed.
    ScopeGuard enter_scope(Symbol *sym = nullptr);

    // todo: override all ASTVisitor methods
};

/*
The class that performs the elaboration pass.
*/
class Elaborator : public BaseSemanticVisitor {

};

/*
The class that performs the validation pass.
*/
class Validator : public BaseSemanticVisitor {

};

/*
The parent class that owns the symbol table and type context.
*/
class SemanticChecker {
public:
    Box<SymbolTable> symbols;
    Box<types::TypeContext> types;
};

}

#endif