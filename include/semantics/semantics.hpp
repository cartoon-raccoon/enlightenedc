#pragma once

#ifndef ECC_SEMANTICS_H
#define ECC_SEMANTICS_H

#include "ast/ast.hpp"
#include "ast/visitor.hpp"
#include "semantics/mir/mir.hpp"
#include "semantics/mir/visitor.hpp"
#include "semantics/symbols.hpp"
#include "prelude.hpp"

namespace ecc::sema {
/*
Semantic Visitor Functionality.

Semantic visitors are a specialization of visitor that automatically handles bookkeeping tasks that
are routinely needed while walking a syntax tree. These include: pushing and popping a context stack,
pushing/entering and popping scopes, etc.

There are two subclasses of BaseSemanticVisitor, each specialized for an IR type:
BaseASTSemaVisitor, which is specialized for semantic visitation of AST nodes; and BaseMIRSemaVisitor,
which is specialized for semantic visitation of MIR nodes.
*/

using namespace ecc;
using namespace util;

template <typename Node>
class NodeGuard;
class BaseASTSemaVisitor;
class BaseMIRSemaVisitor;

class ASTScopeGuard;
template <typename Node>
class NodeGuard;

/**
The base semantic visitor class, parametrized over the type of node it walks.

The base bookkeeping provided by the BaseSemanticVisitor is context tracking: It maintains a stack of nodes
that get pushed and popped as the tree is walked. It does so using a NodeGuard, which is also parametrized
over the type of node being walked.
*/
template <typename Node>
class BaseSemanticVisitor {
public:
    /*
    The state of the BaseSemanticVisitor.
    */
    enum State : uint8_t {
        // The symbol table and type context have already been populated,
        // and should be read from instead.
        READ,
        // The visitor should populate the symbol table and type context.
        WRITE,
    } state;

    // Tracks the outer nodes that the current node rests in.
    Vec<Node *> ctxt_stack;

    Node *imm_ctxt() { return ctxt_stack.back(); }

    bool found_errors = false;

    virtual NodeGuard<Node> enter_node(Node *node) { return NodeGuard(*this, node); }

#ifndef NDEBUG
#include <sstream>

    int indent = 0;

    void inc_indent() { indent += 2; }

    void dec_indent() { indent -= 2; }

    template <typename... Args>
    void bsv_dbprint(Args... args) {
        std::stringstream ss;

        for (int i = 0; i < indent; i++) {
            ss << "  ";
        }

        dbprint(ss.str(), args...);
    }
#else
    template <typename... Args>
    void bsv_dbprint(Args... args) {}
#endif

    BaseSemanticVisitor(State state) : state(state) {}

    virtual ~BaseSemanticVisitor() = default;

}; // class BaseSemanticVisitor

/**
A mixin class that Semantic Visitors can multiply inherit from, to add error reporting functionality.
*/
class Fallible : public NoCopy {
    Vec<Box<EccWarning>> warnings;

    Vec<Box<EccSemError>> errors;

    Vec<EccDiagnostic *> diagnostic_order;

public:
    template <typename E, typename... Args>
        requires std::derived_from<E, EccSemError>
    void add_error(Args... args) {
        Box<EccSemError> err = make_box<E>(args...);
        EccDiagnostic *diag = err.get();
        errors.push_back(std::move(err));
        diagnostic_order.push_back(diag);
    }

    void add_warning(std::string msg, Location loc) {
        Box<EccWarning> warn = make_box<EccWarning>(std::move(msg), loc);
        EccDiagnostic *diag = warn.get();
        warnings.push_back(std::move(warn));
        diagnostic_order.push_back(diag);
    }

    bool has_diagnostics() { return !warnings.empty() || !errors.empty(); }

    bool has_errors() { return !errors.empty(); }

    /**
    Drains the diagnostics from other into `this`.
    */
    void drain(Fallible& other) {
        for (auto& warning : other.warnings) {
            warnings.push_back(std::move(warning));
        }

        other.warnings.clear();

        for (auto& error : other.errors) {
            errors.push_back(std::move(error));
        }

        other.errors.clear();

        for (auto *diag : other.diagnostic_order) {
            diagnostic_order.push_back(diag);
        }

        other.diagnostic_order.clear();
    }

    Span<EccDiagnostic *const> diagnostics() const {
        return diagnostic_order;
    }

    Span<EccDiagnostic *> diagnostics() {
        return diagnostic_order;
    }
};

/*
An RAII wrapper for automatically pushing and popping scopes on a symbol table.

When a ScopeGuard is created, it is supplied with a reference to a symbol table.
The ScopeGuard pushes a new scope into the symbol table.

When the destructor is called, the ScopeGuard automatically pops the scope from its
stored symbol table reference.
*/
class ASTScopeGuard : public NoCopy {
public:
    friend class BaseASTSemaVisitor;
    friend class NodeGuard<ast::ASTNode>;

    ASTScopeGuard(
        BaseSemanticVisitor<ast::ASTNode>::State state, sym::SymbolTableWalker& syms,
        sym::FuncSymbol *assoc)
        : st(syms) {
        if (state == BaseSemanticVisitor<ast::ASTNode>::State::READ) {
            (*st).get().enter_scope();
        } else {
            (*st).get().push_scope(assoc);
        }
    }

    ASTScopeGuard() {}

    // Allow the ScopeGuard to be moved.
    ASTScopeGuard(ASTScopeGuard&& other) noexcept : st(other.st) {}

    ~ASTScopeGuard() {
        if (st) {
            (*st).get().pop_scope();
        }
    }

    // Prevent deep copies of the ScopeGuard.
    ASTScopeGuard(const ASTScopeGuard&)            = delete;
    ASTScopeGuard& operator=(const ASTScopeGuard&) = delete;

private:
    Optional<Ref<sym::SymbolTableWalker>> st;
}; // class ScopeGuard

/*
An RAII wrapper for automatically managing node contexts.

When a NodeGuard is created, it pushes an associated ASTNode onto the
context. When it is destroyed, it pops the top node from the context.
*/
template <typename Node>
class NodeGuard : public NoCopy {
public:
    friend class BaseASTSemaVisitor;
    friend class BaseMIRSemaVisitor;
    friend class ASTScopeGuard;

    NodeGuard(const NodeGuard&)            = delete;
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
        context.get().push_back(node);
    }

    ~NodeGuard() {
        context.get().pop_back();
#ifndef NDEBUG
        bsv.dec_indent();
        bsv.bsv_dbprint("}");
#endif
    }

private:
    Ref<Vec<Node *>> context;
}; // class NodeGuard

/*
The base semantic walker class that handles scoping and AST walking.

The BaseASTSemanticVisitor class handles scoping within a SymbolTable and the basic
AST walking operations, overriding all `visit(ast::)` member functions in the abstract
`ast::ASTVisitor` base class. As such, a BaseSemanticVisitor simply walks the AST
without doing anything on the nodes, pushing and popping scopes as necessary.

Since the AST is walked before scopes are resolved, BaseASTSemaVisitor provides scope tracking
using the `ASTScopeGuard` and an `enter_scope()` method. Creating an ASTScopeGuard either
pushes a new scope or enters the next scope, depending on the mode the visitor is in, and
destroying an ASTScopeGuard pops the current scope.

# do_visit methods

Since the BaseSemanticVisitor has some core functionality that all subclasses will need,
any derived classes should not be override the visit() members. Instead, BaseSemanticVisitor
defines a do_visit() virtual member function for each ASTNode, for derived classes to override
with their specific functionality. This do_visit member is called by the visit() implementation
of BaseSemanticVisitor, after all scope management has been handled.
*/
class BaseASTSemaVisitor : public ast::ASTVisitor, public BaseSemanticVisitor<ast::ASTNode> {
public:
    BaseASTSemaVisitor(State state, const sym::SymbolTableWalker& syms)
        : BaseSemanticVisitor(state), syms(syms) {}

    sym::SymbolTableWalker syms;

    /// \brief Checks if there is `kind` in the context, and if so, how many layers up.
    /// Returns -1 if there is no `kind` in the context.
    int in_node(ast::ASTNode::NodeKind kind);

    ASTScopeGuard enter_scope(sym::FuncSymbol *assoc = nullptr);

    // Visitor method overrides
    //? Should these be marked final?

    // BaseSemanticVisitor provides a basic override of all Visitor methods,
    // so that Elaborator and Validator only need to override needed ones.
    virtual void do_visit(ast::Program& node);
    virtual void do_visit(ast::AttributeArg& node);
    virtual void do_visit(ast::Attribute& node);
    virtual void do_visit(ast::Function& node);

    virtual void do_visit(ast::TypeDeclaration& node);
    virtual void do_visit(ast::ConstexprDeclaration& node);
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
    virtual void do_visit(ast::NullptrExpression& node);
    virtual void do_visit(ast::CallExpression& node);
    virtual void do_visit(ast::MemberAccessExpression& node);
    virtual void do_visit(ast::ReinterpretExpression& node);
    virtual void do_visit(ast::ArraySubscriptExpression& node);
    virtual void do_visit(ast::PostfixExpression& node);
    virtual void do_visit(ast::SizeofExpression& node);

protected:
    void visit(ast::Program& node) override;
    void visit(ast::AttributeArg& node) override;
    void visit(ast::Attribute& node) override;
    void visit(ast::Function& node) override;

    void visit(ast::TypeDeclaration& node) override;
    void visit(ast::ConstexprDeclaration& node) override;
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
    void visit(ast::NullptrExpression& node) override;
    void visit(ast::CallExpression& node) override;
    void visit(ast::MemberAccessExpression& node) override;
    void visit(ast::ReinterpretExpression& node) override;
    void visit(ast::ArraySubscriptExpression& node) override;
    void visit(ast::PostfixExpression& node) override;
    void visit(ast::SizeofExpression& node) override;
}; // class BaseASTSemaVisitor

/**
The BaseMIRSemaVisitor handles semantic visitation of MIR nodes.

Since MIR nodes are walked when scopes are already resolved, each relevant MIR node holds a pointer to
its enclosing scope. This scope can then directly be set as the current scope on a `SymbolTableWalker`.

The `SymbolTableWalker` is exposed through a pure virtual function `symwalker()`, which returns a pointer
to the `SymbolTableWalker`. If the derived visitor does not maintain a `SymbolTableWalker` it is free to
return `nullptr`, to opt out of the automatic scope tracking.
*/
class BaseMIRSemaVisitor : public mir::MIRVisitor, public BaseSemanticVisitor<mir::MIRNode> {
public:
    BaseMIRSemaVisitor(State state) : BaseSemanticVisitor(state) {}

    /**
    Return a SymbolTableWalker, if any.
    */
    virtual sym::SymbolTableWalker *symwalker() = 0;

    /// \brief Checks if there is `kind` in the context, and if so, how many layers up.
    /// Returns -1 if there is no `kind` in the context.
    int in_node(mir::MIRNode::NodeKind kind);

    mir::MIRNode *get_context(mir::MIRNode::NodeKind kind);

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
    virtual void do_visit(mir::LiteralExprMIR& node);
    virtual void do_visit(mir::CallExprMIR& node);
    virtual void do_visit(mir::MemberAccExprMIR& node);
    virtual void do_visit(mir::ReintExprMIR& node);
    virtual void do_visit(mir::SubscrExprMIR& node);
    virtual void do_visit(mir::PostfixExprMIR& node);
    virtual void do_visit(mir::SizeofExprMIR& node);

protected:
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
    void visit(mir::LiteralExprMIR& node) override;
    void visit(mir::CallExprMIR& node) override;
    void visit(mir::MemberAccExprMIR& node) override;
    void visit(mir::ReintExprMIR& node) override;
    void visit(mir::SubscrExprMIR& node) override;
    void visit(mir::PostfixExprMIR& node) override;
    void visit(mir::SizeofExprMIR& node) override;

    // BaseSemanticVisitor provides a basic override of all Visitor methods,
    // so that Elaborator and Validator only need to override needed ones.

}; // class BaseMIRSemaVisitor

} // namespace ecc::sema

#endif