#ifndef ECC_SEMANTICS_H
#define ECC_SEMANTICS_H

#include "ast/visitor.hpp"
#include "ast/ast.hpp"
#include "compiler/types.hpp"
#include "util.hpp"

#include <optional>
#include <unordered_map>
#include <memory>

namespace ecc::compiler {

using namespace util;
using namespace types;
/*
Semantic validation functionality.

Semantic Checking in Ecc occurs in two passes: Elaboration and Validation.

The elaboration pass walks the AST, populating the TypeContext and SymbolTable.

The validation pass then walks the AST, performing type checking and semantic
validation (e.g. no invalid struct member accesses).
*/

using namespace ecc;
using namespace ecc::ast;

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
    } kind;

    Symbol(Kind kind) : kind(kind) {}

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

    VarSymbol(Type *type) : Symbol(Symbol::Kind::Var), type(type) {}

    /// The type of the symbol.
    Type *type;

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

    FuncSymbol(FunctionType *signature) : Symbol(Symbol::Kind::Func), signature(signature) {}

    // The function signature.
    FunctionType *signature;

    bool is_extern = false;
};

/*
A symbol representing a type declaration (class, union, enum).
*/
class TypeSymbol : public Symbol {
public:
    TypeSymbol() : Symbol(Symbol::Kind::Ty), type(nullptr) {}

    TypeSymbol(Type* type) : Symbol(Symbol::Kind::Ty), type(type) {}

    Type *type;
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
    Scope(Symbol *assoc, Scope *outer) : outer(outer), assoc(assoc), symbols(), inners() {}
    // the outer scope enclosing the inner scope.
    Scope *outer;
    // a symbol associated with this scope, usually a function.
    // if null, this is an anonymous scope.
    Symbol *assoc;
    // the symbol table.
    std::unordered_map<std::string, Box<Symbol>> symbols = {};
    // inner scopes contained within this scope.
    Vec<Box<Scope>> inners;
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

    // Enter the next scope.
    void enter_scope();

    // Exit the current scope to the outer one.
    void pop_scope();

    // Reset to the current global scope and first index.
    void reset();

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
    class NodeGuard;
    /*
    An RAII wrapper for automatically pushing and popping scopes on a symbol table.

    When a ScopeGuard is created, it is supplied with a reference to a symbol table.
    The ScopeGuard pushes a new scope into the symbol table.

    When the destructor is called, the ScopeGuard automatically pops the scope from its
    stored symbol table reference.
    */
    class ScopeGuard {
    public:
        friend class BaseSemanticVisitor;
        friend class BaseSemanticVisitor::NodeGuard;

        ScopeGuard(BaseSemanticVisitor& bsv, Symbol *assoc) : st(bsv.syms) {
            if (bsv.state == BaseSemanticVisitor::State::READ) {
                st.enter_scope();
            } else {
                st.push_scope(assoc);
            }
        }

        // Allow the ScopeGuard to be moved.
        ScopeGuard(ScopeGuard&& other) : st(other.st) {}
    
        ~ScopeGuard() {
            st.pop_scope();
        }

        // Prevent deep copies of the ScopeGuard.
        ScopeGuard(const ScopeGuard&) = delete;
        ScopeGuard& operator=(const ScopeGuard&) = delete;
        
    private:
        SymbolTable& st;
    }; // class ScopeGuard

    /*
    An RAII wrapper for automatically managing node contexts.

    When a NodeGuard is created, it pushes an associated ASTNode onto the
    context. When it is destroyed, it pops the top node from the context.
    */
    class NodeGuard {
    public:
        friend class BaseSemanticVisitor;
        friend class BaseSemanticVisitor::ScopeGuard;

        NodeGuard(const NodeGuard&) = delete;
        NodeGuard& operator=(const NodeGuard&) = delete;
        /*
        Create a NodeGuard.

        If `true` is passed, a new scopeguard will be created as well, that will be
        destroyed in the NodeGuard's destructor.
        */
        NodeGuard(BaseSemanticVisitor& bsv, ASTNode *node, bool new_scope = false) : context(bsv.ctxt_stack) {
            if (new_scope) {
                scope_guard.emplace(std::move(bsv.enter_scope()));
            }
            context.push_back(node);
        }

        ~NodeGuard() {
            context.pop_back();
        }

    private:
        Vec<ASTNode *>& context;
        std::optional<ScopeGuard> scope_guard;
    }; // class NodeGuard

    /*
    The state of the BaseSemanticVisitor.
    */
    enum State {
        // The visitor should populate the symbol table and type context.
        READ,
        // The symbol table and type context have already been populated,
        // and should be read from instead.
        WRITE,
    };

    BaseSemanticVisitor(State state, SymbolTable& syms, TypeContext& types)
    : state(state), syms(syms), types(types) {}

    // The current state of the BaseSemanticVisitor.
    State state;
    // The Symbol Table.
    SymbolTable& syms;
    // The Type Context.
    TypeContext& types;
    // Tracks the outer nodes that the current node rests in.
    Vec<ASTNode *> ctxt_stack;

    /*
    If in write mode, creates and enters a new scope.

    If in read mode, enters the next scope.
    */
    ScopeGuard enter_scope(Symbol *sym = nullptr);

    // Enter an AST node. If new_scope is true, additionally creates a new scope.
    NodeGuard enter_node(ASTNode *node, bool new_scope = false);

    // Get the AST node immediately outside this node.
    ASTNode *imm_ctxt();

    // Switch from READ to WRITE or vice versa.
    void switch_state();

    /// \brief Checks if there is `kind` in the context, and if so, how many layers up.
    /// Returns -1 if there is no `kind` in the context.
    int in_node(ASTNode::NodeKind kind);

    // Visitor method overrides

    // BaseSemanticVisitor provides a basic override of all Visitor methods,
    // so that Elaborator and Validator only need to override needed ones.

    void visit(Program& node) override;
    void visit(Function& node) override;

    void visit(TypeDeclaration& node) override;
    void visit(VariableDeclaration& node) override;
    void visit(ParameterDeclaration& node) override;
    void visit(Declarator& node) override;
    void visit(ParenDeclarator& node) override;
    void visit(ArrayDeclarator& node) override;
    void visit(FunctionDeclarator& node) override;
    void visit(InitDeclarator& node) override;
    void visit(Pointer& node) override;
    void visit(ClassDeclarator& node) override;
    void visit(ClassDeclaration& node) override;
    void visit(Enumerator& node) override;
    void visit(StorageClassSpecifier& node) override;
    void visit(TypeQualifier& node) override;
    void visit(EnumSpecifier& node) override;
    void visit(ClassOrUnionSpecifier& node) override;
    void visit(PrimitiveSpecifier& node) override;
    void visit(Initializer& node) override;
    void visit(TypeName& node) override;
    void visit(IdentifierDeclarator& node) override;

    void visit(CompoundStatement& node) override;
    void visit(ExpressionStatement& node) override;
    void visit(CaseDefaultStatement& node) override;
    void visit(LabeledStatement& node) override;
    void visit(PrintStatement& node) override;
    void visit(IfStatement& node) override;
    void visit(SwitchStatement& node) override;
    void visit(WhileStatement& node) override;
    void visit(DoWhileStatement& node) override;
    void visit(ForStatement& node) override;
    void visit(GotoStatement& node) override;
    void visit(BreakStatement& node) override;
    void visit(ReturnStatement& node) override;

    void visit(BinaryExpression& node) override;
    void visit(CastExpression& node) override;
    void visit(UnaryExpression& node) override;
    void visit(AssignmentExpression& node) override;
    void visit(ConditionalExpression& node) override;
    void visit(IdentifierExpression& node) override;
    void visit(ConstExpression& node) override;
    void visit(LiteralExpression& node) override;
    void visit(StringExpression& node) override;
    void visit(CallExpression& node) override;
    void visit(MemberAccessExpression& node) override;
    void visit(ArraySubscriptExpression& node) override;
    void visit(PostfixExpression& node) override;
    void visit(SizeofExpression& node) override;

protected:
    virtual void do_visit(Program& node);
    virtual void do_visit(Function& node);

    virtual void do_visit(TypeDeclaration& node);
    virtual void do_visit(VariableDeclaration& node);
    virtual void do_visit(ParameterDeclaration& node);
    virtual void do_visit(Declarator& node);
    virtual void do_visit(ParenDeclarator& node);
    virtual void do_visit(ArrayDeclarator& node);
    virtual void do_visit(FunctionDeclarator& node);
    virtual void do_visit(InitDeclarator& node);
    virtual void do_visit(Pointer& node);
    virtual void do_visit(ClassDeclarator& node);
    virtual void do_visit(ClassDeclaration& node);
    virtual void do_visit(Enumerator& node);
    virtual void do_visit(StorageClassSpecifier& node);
    virtual void do_visit(TypeQualifier& node);
    virtual void do_visit(EnumSpecifier& node);
    virtual void do_visit(ClassOrUnionSpecifier& node);
    virtual void do_visit(PrimitiveSpecifier& node);
    virtual void do_visit(Initializer& node);
    virtual void do_visit(TypeName& node);
    virtual void do_visit(IdentifierDeclarator& node);

    virtual void do_visit(CompoundStatement& node);
    virtual void do_visit(ExpressionStatement& node);
    virtual void do_visit(CaseDefaultStatement& node);
    virtual void do_visit(LabeledStatement& node);
    virtual void do_visit(PrintStatement& node);
    virtual void do_visit(IfStatement& node);
    virtual void do_visit(SwitchStatement& node);
    virtual void do_visit(WhileStatement& node);
    virtual void do_visit(DoWhileStatement& node);
    virtual void do_visit(ForStatement& node);
    virtual void do_visit(GotoStatement& node);
    virtual void do_visit(BreakStatement& node);
    virtual void do_visit(ReturnStatement& node);

    virtual void do_visit(BinaryExpression& node);
    virtual void do_visit(CastExpression& node);
    virtual void do_visit(UnaryExpression& node);
    virtual void do_visit(AssignmentExpression& node);
    virtual void do_visit(ConditionalExpression& node);
    virtual void do_visit(IdentifierExpression& node);
    virtual void do_visit(ConstExpression& node);
    virtual void do_visit(LiteralExpression& node);
    virtual void do_visit(StringExpression& node);
    virtual void do_visit(CallExpression& node);
    virtual void do_visit(MemberAccessExpression& node);
    virtual void do_visit(ArraySubscriptExpression& node);
    virtual void do_visit(PostfixExpression& node);
    virtual void do_visit(SizeofExpression& node);
};

/*
The class that performs the validation pass.
*/
class Validator : public BaseSemanticVisitor {
public:
    Validator(SymbolTable& syms, TypeContext& types)
    : BaseSemanticVisitor(BaseSemanticVisitor::State::READ, syms, types) {}
};

/*
The parent class that owns the symbol table and type context.
*/
class SemanticChecker {
public:
    SemanticChecker() :
    symbols(std::make_unique<SymbolTable>()),
    types(std::make_unique<TypeContext>()) {}

    Box<SymbolTable> symbols;
    Box<types::TypeContext> types;

    void check_semantics(ASTNode& prog);
};

}

#endif