#ifndef ECC_SEMANTICS_H
#define ECC_SEMANTICS_H

#include "ast/visitor.hpp"
#include "ast/ast.hpp"
#include "semantics/symbols.hpp"
#include "semantics/types.hpp"
#include "util.hpp"

#include <optional>
#include <memory>

namespace ecc::sema {
/*
Semantic validation functionality.

Semantic Checking in Ecc occurs in two passes: Elaboration and Validation.

The elaboration pass walks the AST, populating the TypeContext and SymbolTable.

The validation pass then walks the AST, performing type checking and semantic
validation (e.g. no invalid struct member accesses).
*/

using namespace ecc;
using namespace util;


/*
The base semantic walker class that handles scoping and AST walking.

The BaseSemanticVisitor class handles scoping within a SymbolTable and the basic
AST walking operations, overriding all `visit(ast::)` member functions in the abstract
`ast::ASTVisitor` base class. As such, a BaseSemanticVisitor simply walks the AST
without doing anything on the nodes, pushing and popping scopes as necessary.

Each class implementing a semantic validation pass should inherit from this class.
overriding the `visit(ast::)` member functions that concern them.
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

        ScopeGuard(BaseSemanticVisitor& bsv, sym::Symbol *assoc) : st(bsv.syms) {
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
        sym::SymbolTable& st;
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
        NodeGuard(BaseSemanticVisitor& bsv, ast::ASTNode *node, bool new_scope = false) : context(bsv.ctxt_stack) {
            if (new_scope) {
                scope_guard.emplace(std::move(bsv.enter_scope()));
            }
            context.push_back(node);
        }

        ~NodeGuard() {
            context.pop_back();
        }

    private:
        Vec<ast::ASTNode *>& context;
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

    BaseSemanticVisitor(State state, sym::SymbolTable& syms, types::TypeContext& types)
    : state(state), syms(syms), types(types) {}

    // The current state of the BaseSemanticVisitor.
    State state;
    // The Symbol Table.
    sym::SymbolTable& syms;
    // The Type Context.
    types::TypeContext& types;
    // Tracks the outer nodes that the current node rests in.
    Vec<ast::ASTNode *> ctxt_stack;

    /*
    If in write mode, creates and enters a new scope.

    If in read mode, enters the next scope.
    */
    ScopeGuard enter_scope(sym::Symbol *sym = nullptr);

    // Enter an AST node. If new_scope is true, additionally creates a new scope.
    NodeGuard enter_node(ast::ASTNode *node, bool new_scope = false);

    // Get the AST node immediately outside this node.
    ast::ASTNode *imm_ctxt();

    // Switch from READ to WRITE or vice versa.
    void switch_state();

    /// \brief Checks if there is `kind` in the context, and if so, how many layers up.
    /// Returns -1 if there is no `kind` in the context.
    int in_node(ast::ASTNode::NodeKind kind);

    // Visitor method overrides

    // BaseSemanticVisitor provides a basic override of all Visitor methods,
    // so that Elaborator and Validator only need to override needed ones.

    void visit(ast::Program& node) override;
    void visit(ast::Function& node) override;

    void visit(ast::TypeDeclaration& node) override;
    void visit(ast::VariableDeclaration& node) override;
    void visit(ast::ParameterDeclaration& node) override;
    void visit(ast::Declarator& node) override;
    void visit(ast::ParenDeclarator& node) override;
    void visit(ast::ArrayDeclarator& node) override;
    void visit(ast::FunctionDeclarator& node) override;
    void visit(ast::InitDeclarator& node) override;
    void visit(ast::Pointer& node) override;
    void visit(ast::ClassDeclarator& node) override;
    void visit(ast::ClassDeclaration& node) override;
    void visit(ast::Enumerator& node) override;
    void visit(ast::StorageClassSpecifier& node) override;
    void visit(ast::TypeQualifier& node) override;
    void visit(ast::EnumSpecifier& node) override;
    void visit(ast::ClassOrUnionSpecifier& node) override;
    void visit(ast::PrimitiveSpecifier& node) override;
    void visit(ast::Initializer& node) override;
    void visit(ast::TypeName& node) override;
    void visit(ast::IdentifierDeclarator& node) override;

    void visit(ast::CompoundStatement& node) override;
    void visit(ast::ExpressionStatement& node) override;
    void visit(ast::CaseDefaultStatement& node) override;
    void visit(ast::LabeledStatement& node) override;
    void visit(ast::PrintStatement& node) override;
    void visit(ast::IfStatement& node) override;
    void visit(ast::SwitchStatement& node) override;
    void visit(ast::WhileStatement& node) override;
    void visit(ast::DoWhileStatement& node) override;
    void visit(ast::ForStatement& node) override;
    void visit(ast::GotoStatement& node) override;
    void visit(ast::BreakStatement& node) override;
    void visit(ast::ReturnStatement& node) override;

    void visit(ast::BinaryExpression& node) override;
    void visit(ast::CastExpression& node) override;
    void visit(ast::UnaryExpression& node) override;
    void visit(ast::AssignmentExpression& node) override;
    void visit(ast::ConditionalExpression& node) override;
    void visit(ast::IdentifierExpression& node) override;
    void visit(ast::ConstExpression& node) override;
    void visit(ast::LiteralExpression& node) override;
    void visit(ast::StringExpression& node) override;
    void visit(ast::CallExpression& node) override;
    void visit(ast::MemberAccessExpression& node) override;
    void visit(ast::ArraySubscriptExpression& node) override;
    void visit(ast::PostfixExpression& node) override;
    void visit(ast::SizeofExpression& node) override;

protected:
    virtual void do_visit(ast::Program& node);
    virtual void do_visit(ast::Function& node);

    virtual void do_visit(ast::TypeDeclaration& node);
    virtual void do_visit(ast::VariableDeclaration& node);
    virtual void do_visit(ast::ParameterDeclaration& node);
    virtual void do_visit(ast::Declarator& node);
    virtual void do_visit(ast::ParenDeclarator& node);
    virtual void do_visit(ast::ArrayDeclarator& node);
    virtual void do_visit(ast::FunctionDeclarator& node);
    virtual void do_visit(ast::InitDeclarator& node);
    virtual void do_visit(ast::Pointer& node);
    virtual void do_visit(ast::ClassDeclarator& node);
    virtual void do_visit(ast::ClassDeclaration& node);
    virtual void do_visit(ast::Enumerator& node);
    virtual void do_visit(ast::StorageClassSpecifier& node);
    virtual void do_visit(ast::TypeQualifier& node);
    virtual void do_visit(ast::EnumSpecifier& node);
    virtual void do_visit(ast::ClassOrUnionSpecifier& node);
    virtual void do_visit(ast::PrimitiveSpecifier& node);
    virtual void do_visit(ast::Initializer& node);
    virtual void do_visit(ast::TypeName& node);
    virtual void do_visit(ast::IdentifierDeclarator& node);

    virtual void do_visit(ast::CompoundStatement& node);
    virtual void do_visit(ast::ExpressionStatement& node);
    virtual void do_visit(ast::CaseDefaultStatement& node);
    virtual void do_visit(ast::LabeledStatement& node);
    virtual void do_visit(ast::PrintStatement& node);
    virtual void do_visit(ast::IfStatement& node);
    virtual void do_visit(ast::SwitchStatement& node);
    virtual void do_visit(ast::WhileStatement& node);
    virtual void do_visit(ast::DoWhileStatement& node);
    virtual void do_visit(ast::ForStatement& node);
    virtual void do_visit(ast::GotoStatement& node);
    virtual void do_visit(ast::BreakStatement& node);
    virtual void do_visit(ast::ReturnStatement& node);

    virtual void do_visit(ast::BinaryExpression& node);
    virtual void do_visit(ast::CastExpression& node);
    virtual void do_visit(ast::UnaryExpression& node);
    virtual void do_visit(ast::AssignmentExpression& node);
    virtual void do_visit(ast::ConditionalExpression& node);
    virtual void do_visit(ast::IdentifierExpression& node);
    virtual void do_visit(ast::ConstExpression& node);
    virtual void do_visit(ast::LiteralExpression& node);
    virtual void do_visit(ast::StringExpression& node);
    virtual void do_visit(ast::CallExpression& node);
    virtual void do_visit(ast::MemberAccessExpression& node);
    virtual void do_visit(ast::ArraySubscriptExpression& node);
    virtual void do_visit(ast::PostfixExpression& node);
    virtual void do_visit(ast::SizeofExpression& node);
};

/*
The class that performs the validation pass.
*/
class Validator : public BaseSemanticVisitor {
public:
    Validator(sym::SymbolTable& syms, types::TypeContext& types)
    : BaseSemanticVisitor(BaseSemanticVisitor::State::READ, syms, types) {}
};

/*
The parent class that owns the symbol table and type context.
*/
class SemanticChecker {
public:
    SemanticChecker() :
    symbols(std::make_unique<sym::SymbolTable>()),
    types(std::make_unique<types::TypeContext>()) {}

    Box<sym::SymbolTable> symbols;
    Box<types::TypeContext> types;

    void check_semantics(ast::ASTNode& prog);
};

}

#endif