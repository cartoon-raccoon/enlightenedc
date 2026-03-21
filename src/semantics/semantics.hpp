#ifndef ECC_SEMANTICS_H
#define ECC_SEMANTICS_H

#include "ast/visitor.hpp"
#include "ast/ast.hpp"
#include "semantics/symbols.hpp"
#include "semantics/types.hpp"
#include "semantics/mir/mir.hpp"
#include "semantics/mir/visitor.hpp"
#include "util.hpp"

#include <optional>

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

template <typename Node>
class NodeGuard;
class BaseASTSemaVisitor;
class BaseMIRSemaVisitor;

template <typename Node>
class ScopeGuard;
template <typename Node>
class NodeGuard;

template <typename Node>
class BaseSemanticVisitor {
public:
    /*
    The state of the BaseSemanticVisitor.
    */
    enum State {
        // The visitor should populate the symbol table and type context.
        READ,
        // The symbol table and type context have already been populated,
        // and should be read from instead.
        WRITE,
    } state;

        // The Symbol Table.
    sym::SymbolTable& syms;
    // The Type Context.
    types::TypeContext& types;

    // Tracks the outer nodes that the current node rests in.
    Vec<Node *> ctxt_stack;

    Node *imm_ctxt() {
        return ctxt_stack.back();
    }

    ScopeGuard<Node> enter_scope(sym::Symbol *assoc = nullptr) {
        return ScopeGuard(*this, assoc);
    }

    NodeGuard<Node> enter_node(Node *node) {
        return NodeGuard(*this, node);
    }

#ifndef NDEBUG
#include <sstream>

    int indent = 0;

    void inc_indent() { indent += 2; }

    void dec_indent() { indent -= 2; }

    template <typename ...Args>
    void bsv_dbprint(Args ...args) {
        std::stringstream ss;

        for (int i = 0; i < indent; i++) {
            ss << "  ";
        }

        dbprint(ss.str(), args ...);
    }
#else
    template <typename ...Args>
    void bsv_dbprint(Args ...args) {}
#endif

    BaseSemanticVisitor(sym::SymbolTable& syms, types::TypeContext& types, State state)
        : syms(syms), types(types) {}
}; // class BaseSemanticVisitor


/*
An RAII wrapper for automatically pushing and popping scopes on a symbol table.

When a ScopeGuard is created, it is supplied with a reference to a symbol table.
The ScopeGuard pushes a new scope into the symbol table.

When the destructor is called, the ScopeGuard automatically pops the scope from its
stored symbol table reference.
*/
template <typename Node>
class ScopeGuard {
public:
    friend class BaseASTSemaVisitor;
    friend class BaseMIRSemaVisitor;
    friend class NodeGuard<Node>;

    ScopeGuard(BaseSemanticVisitor<Node>& bsv, sym::Symbol *assoc) : st(bsv.syms) {
        if (bsv.state == BaseSemanticVisitor<Node>::State::READ) {
            bsv.bsv_dbprint("BSV currently in state READ, entering scope");
            st.enter_scope();
        } else {
            bsv.bsv_dbprint("BSV currently in state WRITE, pushing scope");
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
template <typename Node>
class NodeGuard {
public:
    friend class BaseASTSemaVisitor;
    friend class BaseMIRSemaVisitor;
    friend class ScopeGuard<Node>;

    NodeGuard(const NodeGuard&) = delete;
    NodeGuard& operator=(const NodeGuard&) = delete;

#ifndef NDEBUG
    BaseSemanticVisitor<Node>& bsv;
#endif
    /*
    Create a NodeGuard.

    If `true` is passed, a new scopeguard will be created as well, that will be
    destroyed in the NodeGuard's destructor.
    */
    NodeGuard(BaseSemanticVisitor<Node>& bsv, Node *node) 
    : 
#ifndef NDEBUG
    bsv(bsv), 
#endif
    context(bsv.ctxt_stack) {
#ifndef NDEBUG
        bsv.bsv_dbprint("Node {");
        bsv.inc_indent();
#endif
        context.push_back(node);
    }

    ~NodeGuard() {
        context.pop_back();
#ifndef NDEBUG
        bsv.dec_indent();
        bsv.bsv_dbprint("}");
#endif
    }

private:
    Vec<Node *>& context;
    std::optional<ScopeGuard<Node>> scope_guard;
}; // class NodeGuard



/*
The base semantic walker class that handles scoping and AST walking.

The BaseSemanticVisitor class handles scoping within a SymbolTable and the basic
AST walking operations, overriding all `visit(ast::)` member functions in the abstract
`ast::ASTVisitor` base class. As such, a BaseSemanticVisitor simply walks the AST
without doing anything on the nodes, pushing and popping scopes as necessary.

# do_visit methods

Since the BaseSemanticVisitor has some core functionality that all subclasses will need,
any derived classes should not be override the visit() members. Instead, BaseSemanticVisitor
defines a do_visit() virtual member function for each ASTNode, for derived classes to override
with their specific functionality. This do_visit member is called by the visit() implementation
of BaseSemanticVisitor, after all scope management has been handled.
*/
class BaseASTSemaVisitor : public ast::ASTVisitor, public BaseSemanticVisitor<ast::ASTNode> {
public:

    BaseASTSemaVisitor(State state, sym::SymbolTable& syms, types::TypeContext& types)
    : BaseSemanticVisitor(syms, types, state) {}

    /// \brief Checks if there is `kind` in the context, and if so, how many layers up.
    /// Returns -1 if there is no `kind` in the context.
    int in_node(ast::ASTNode::NodeKind kind);

    // Visitor method overrides
    //? Should these be marked final?

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
    void visit(ast::ClassSpecifier& node) override;
    void visit(ast::UnionSpecifier& node) override;
    void visit(ast::TypeIdentifier& node) override;
    void visit(ast::VoidSpecifier& node) override;
    void visit(ast::PrimitiveSpecifier& node) override;
    void visit(ast::Initializer& node) override;
    void visit(ast::TypeName& node) override;
    void visit(ast::IdentifierDeclarator& node) override;

    void visit(ast::CompoundStatement& node) override;
    void visit(ast::ExpressionStatement& node) override;
    void visit(ast::CaseStatement& node) override;
    void visit(ast::CaseRangeStatement& node) override;
    void visit(ast::DefaultStatement& node) override;
    void visit(ast::LabeledStatement& node) override;
    void visit(ast::PrintStatement& node) override;
    void visit(ast::IfStatement& node) override;
    void visit(ast::SwitchStatement& node) override;
    void visit(ast::WhileStatement& node) override;
    void visit(ast::DoWhileStatement& node) override;
    void visit(ast::ForStatement& node) override;
    void visit(ast::GotoStatement& node) override;
    void visit(ast::BreakStatement& node) override;
    void visit(ast::ContinueStatement& node) override;
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
    virtual void do_visit(ast::ClassSpecifier& node);
    virtual void do_visit(ast::UnionSpecifier& node);
    virtual void do_visit(ast::TypeIdentifier& node);
    virtual void do_visit(ast::VoidSpecifier& node);
    virtual void do_visit(ast::PrimitiveSpecifier& node);
    virtual void do_visit(ast::Initializer& node);
    virtual void do_visit(ast::TypeName& node);
    virtual void do_visit(ast::IdentifierDeclarator& node);

    virtual void do_visit(ast::CompoundStatement& node);
    virtual void do_visit(ast::ExpressionStatement& node);
    virtual void do_visit(ast::CaseStatement& node);
    virtual void do_visit(ast::CaseRangeStatement& node);
    virtual void do_visit(ast::DefaultStatement& node);
    virtual void do_visit(ast::LabeledStatement& node);
    virtual void do_visit(ast::PrintStatement& node);
    virtual void do_visit(ast::IfStatement& node);
    virtual void do_visit(ast::SwitchStatement& node);
    virtual void do_visit(ast::WhileStatement& node);
    virtual void do_visit(ast::DoWhileStatement& node);
    virtual void do_visit(ast::ForStatement& node);
    virtual void do_visit(ast::GotoStatement& node);
    virtual void do_visit(ast::BreakStatement& node);
    virtual void do_visit(ast::ContinueStatement& node);
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
}; // class BaseASTSemaVisitor


class BaseMIRSemaVisitor : public mir::MIRVisitor, public BaseSemanticVisitor<mir::MIRNode> {
public:

    BaseMIRSemaVisitor(State state, sym::SymbolTable& syms, types::TypeContext& types)
        : BaseSemanticVisitor(syms, types, state) {}

    /// \brief Checks if there is `kind` in the context, and if so, how many layers up.
    /// Returns -1 if there is no `kind` in the context.
    int in_node(mir::MIRNode::NodeKind kind);

    mir::MIRNode *get_context(mir::MIRNode::NodeKind kind);

    // Visitor method overrides

    void visit(mir::ProgramMIR& node) override;
    void visit(mir::FunctionMIR& node) override;
    
    void visit(mir::InitializerMIR& node) override;
    void visit(mir::TypeDeclMIR& node) override;
    void visit(mir::VarDeclMIR& node) override;

    void visit(mir::CompoundStmtMIR& node) override;
    void visit(mir::ExprStmtMIR& node) override;
    void visit(mir::SwitchStmtMIR& node) override;
    void visit(mir::CaseStmtMIR& node) override;
    void visit(mir::CaseRangeStmtMIR& node) override;
    void visit(mir::DefaultStmtMIR& node) override;
    void visit(mir::LabeledStmtMIR& node) override;
    void visit(mir::PrintStmtMIR& node) override;
    void visit(mir::IfStmtMIR& node) override;
    void visit(mir::LoopStmtMIR& node) override;
    void visit(mir::GotoStmtMIR& node) override;
    void visit(mir::BreakStmtMIR& node) override;
    void visit(mir::ContStmtMIR& node) override;
    void visit(mir::ReturnStmtMIR& node) override;

    void visit(mir::BinaryExprMIR& node) override;
    void visit(mir::UnaryExprMIR& node) override;
    void visit(mir::CastExprMIR& node) override;
    void visit(mir::AssignExprMIR& node) override;
    void visit(mir::CondExprMIR& node) override;
    void visit(mir::IdentExprMIR& node) override;
    void visit(mir::ConstExprMIR& node) override;
    void visit(mir::LiteralExprMIR& node) override;
    void visit(mir::CallExprMIR& node) override;
    void visit(mir::MemberAccExprMIR& node) override;
    void visit(mir::SubscrExprMIR& node) override;
    void visit(mir::PostfixExprMIR& node) override;
    void visit(mir::SizeofExprMIR& node) override;

    // BaseSemanticVisitor provides a basic override of all Visitor methods,
    // so that Elaborator and Validator only need to override needed ones.

protected:

    virtual void do_visit(mir::ProgramMIR& node);
    virtual void do_visit(mir::FunctionMIR& node);
    
    virtual void do_visit(mir::InitializerMIR& node);
    virtual void do_visit(mir::TypeDeclMIR& node);
    virtual void do_visit(mir::VarDeclMIR& node);

    virtual void do_visit(mir::CompoundStmtMIR& node);
    virtual void do_visit(mir::ExprStmtMIR& node);
    virtual void do_visit(mir::SwitchStmtMIR& node);
    virtual void do_visit(mir::CaseStmtMIR& node);
    virtual void do_visit(mir::CaseRangeStmtMIR& node);
    virtual void do_visit(mir::DefaultStmtMIR& node);
    virtual void do_visit(mir::LabeledStmtMIR& node);
    virtual void do_visit(mir::PrintStmtMIR& node);
    virtual void do_visit(mir::IfStmtMIR& node);
    virtual void do_visit(mir::LoopStmtMIR& node);
    virtual void do_visit(mir::GotoStmtMIR& node);
    virtual void do_visit(mir::BreakStmtMIR& node);
    virtual void do_visit(mir::ContStmtMIR& node);
    virtual void do_visit(mir::ReturnStmtMIR& node);

    virtual void do_visit(mir::BinaryExprMIR& node);
    virtual void do_visit(mir::UnaryExprMIR& node);
    virtual void do_visit(mir::CastExprMIR& node);
    virtual void do_visit(mir::AssignExprMIR& node);
    virtual void do_visit(mir::CondExprMIR& node);
    virtual void do_visit(mir::IdentExprMIR& node);
    virtual void do_visit(mir::ConstExprMIR& node);
    virtual void do_visit(mir::LiteralExprMIR& node);
    virtual void do_visit(mir::CallExprMIR& node);
    virtual void do_visit(mir::MemberAccExprMIR& node);
    virtual void do_visit(mir::SubscrExprMIR& node);
    virtual void do_visit(mir::PostfixExprMIR& node);
    virtual void do_visit(mir::SizeofExprMIR& node);

}; // class BaseMIRSemaVisitor

/*
The parent class that owns the symbol table and type context.
*/
class SemanticChecker {
public:
    SemanticChecker(sym::SymbolTable& symbols, types::TypeContext& types) :
    symbols(symbols),
    types(types) {}

    sym::SymbolTable& symbols;
    types::TypeContext& types;

    void check_semantics(ast::Program& prog, mir::ProgramMIR& mir);
};



}

#endif