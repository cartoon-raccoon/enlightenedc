#pragma once

#ifndef ECC_MIR_H
#define ECC_MIR_H

#include <variant>

#include "abstract/visitor.hpp"
#include "eval/evaluator.hpp"
#include "eval/value.hpp"
#include "location.hpp"
#include "semantics/mir/visitor.hpp"
#include "semantics/primitives.hpp"
#include "semantics/symbols.hpp"
#include "semantics/types.hpp"
#include "tokens.hpp"
#include "util.hpp"

using namespace ecc;
using namespace util;
using namespace location;

/*
\namespace ecc::sema::mir

Middle representation functionality.

The Middle Intermediate Representation, or MIR for short, is a simpler tree-based
structure that contains the semantic structure of the program, without unneccessary
syntactic details that are no longer needed. Symbols are mapped directly to Symbol
objects in the SymbolTable, and Expressions are directly tagged with Types.

The MIR is where most semantic validation steps are performed, such as typechecking
and some desugaring.
*/
namespace ecc::sema::mir {

template <typename DerivedT, typename BaseT>
using MIRVisitable = Visitable<DerivedT, BaseT, MIRVisitor>;

/**
A simpler version of the AST, mapping symbols directly to types.
*/
class MIRNode : public NoCopy {
public:
    enum class NodeKind : uint8_t {
        PROG_MIR,
        FUNC_MIR,
        INIT_MIR,

        TYPEDEC_MIR,
        VARDEC_MIR,

        CMPDSTMT_MIR,
        EXPRSTMT_MIR,
        SWITCHSTMT_MIR,
        CASESTMT_MIR,
        CASERGSTMT_MIR,
        DEFSTMT_MIR,
        LABSTMT_MIR,
        PRINTSTMT_MIR,
        IFSTMT_MIR,
        LOOPSTMT_MIR,
        GOTOSTMT_MIR,
        BREAKSTMT_MIR,
        CONTSTMT_MIR,
        RETSTMT_MIR,

        BINEXPR_MIR,
        UNEXPR_MIR,
        CASTEXPR_MIR,
        ASSGNEXPR_MIR,
        CONDEXPR_MIR,
        IDENTEXPR_MIR,
        CONSTEXPR_MIR,
        LITEXPR_MIR,
        STREXPR_MIR,
        CALLEXPR_MIR,
        MEMACCEXPR_MIR,
        REINTEXPR_MIR,
        SUBSCREXPR_MIR,
        PFIXEXPR_MIR,
        SIZEEXPR_MIR,
    };

    MIRNode(NodeKind kind) : kind(kind) {}

    MIRNode(Location loc, NodeKind kind) : kind(kind), loc(loc) {}
    virtual ~MIRNode() = default;

    NodeKind kind;
    Location loc;

    virtual NodeKind get_kind() { return kind; };

    virtual void accept(MIRVisitor& visitor) = 0;
};

class ProgItemMIR : public MIRNode {
public:
    ProgItemMIR(Location loc, NodeKind kind) : MIRNode(loc, kind) {}

    static bool classof(const MIRNode *node) {
        switch (node->kind) {
        case NodeKind::TYPEDEC_MIR:
        case NodeKind::VARDEC_MIR:
        case NodeKind::CMPDSTMT_MIR:
        case NodeKind::EXPRSTMT_MIR:
        case NodeKind::SWITCHSTMT_MIR:
        case NodeKind::CASESTMT_MIR:
        case NodeKind::CASERGSTMT_MIR:
        case NodeKind::DEFSTMT_MIR:
        case NodeKind::LABSTMT_MIR:
        case NodeKind::PRINTSTMT_MIR:
        case NodeKind::IFSTMT_MIR:
        case NodeKind::LOOPSTMT_MIR:
        case NodeKind::GOTOSTMT_MIR:
        case NodeKind::BREAKSTMT_MIR:
        case NodeKind::CONTSTMT_MIR:
        case NodeKind::RETSTMT_MIR:
        case NodeKind::FUNC_MIR:
            return true;
        default:
            return false;
        }
    }
};

class ExprMIR : public MIRNode {
public:
    ExprMIR(Location loc, NodeKind kind, sema::sym::Scope *scope)
        : MIRNode(loc, kind), scope(scope) {}
    ExprMIR(Location loc, NodeKind kind, sema::sym::Scope *scope, sema::types::Type *type)
        : MIRNode(loc, kind), scope(scope), eff_type(type) {}

    sema::sym::Scope *scope = nullptr;

    /**
    The actual type associated with the expression, populated at validation.
    */
    sema::types::Type *act_type = nullptr;

    /**
    The effective type associated with the expression, populated at validation.

    This is usually the type returned by `act_type->effective_type()`.
    */
    sema::types::Type *eff_type = nullptr;

    // Whether this expression can be assigned to,
    virtual bool is_assignable() { return false; }

    // Whether this expression can be called as a function.
    // By default, if the type of the expression is callable, then
    // the expression is callable.
    virtual bool is_callable() { return eff_type->is_callable(); }

    // Whether this expression is valid on the left side of an assign expression.
    virtual bool is_lvalue() { return false; }

    virtual bool is_subscriptable() { return eff_type->is_subscriptable(); }

    virtual bool is_const_foldable() = 0;

    void set_type(sema::types::Type *type) {
        act_type = type;
        eff_type = act_type->effective_type();
    }

    virtual eval::Value eval(eval::ExprEvaluator& ev) = 0;

    static bool classof(const MIRNode *node) {
        switch (node->kind) {
        case NodeKind::BINEXPR_MIR:
        case NodeKind::UNEXPR_MIR:
        case NodeKind::CASTEXPR_MIR:
        case NodeKind::ASSGNEXPR_MIR:
        case NodeKind::CONDEXPR_MIR:
        case NodeKind::IDENTEXPR_MIR:
        case NodeKind::LITEXPR_MIR:
        case NodeKind::CALLEXPR_MIR:
        case NodeKind::MEMACCEXPR_MIR:
        case NodeKind::REINTEXPR_MIR:
        case NodeKind::SUBSCREXPR_MIR:
        case NodeKind::PFIXEXPR_MIR:
        case NodeKind::SIZEEXPR_MIR:
            return true;
        default:
            return false;
        }
    }
};

class DeclMIR : public ProgItemMIR {
public:
    DeclMIR(Location loc, NodeKind kind) : ProgItemMIR(loc, kind) {}

    static bool classof(const MIRNode *node) {
        switch (node->kind) {
        case NodeKind::TYPEDEC_MIR:
        case NodeKind::VARDEC_MIR:
            return true;
        default:
            return false;
        }
    }
};

class StmtMIR : public ProgItemMIR {
public:
    StmtMIR(Location loc, NodeKind kind) : ProgItemMIR(loc, kind) {}

    static bool classof(const MIRNode *node) {
        switch (node->kind) {
        case NodeKind::CMPDSTMT_MIR:
        case NodeKind::EXPRSTMT_MIR:
        case NodeKind::SWITCHSTMT_MIR:
        case NodeKind::CASESTMT_MIR:
        case NodeKind::CASERGSTMT_MIR:
        case NodeKind::DEFSTMT_MIR:
        case NodeKind::LABSTMT_MIR:
        case NodeKind::PRINTSTMT_MIR:
        case NodeKind::IFSTMT_MIR:
        case NodeKind::LOOPSTMT_MIR:
        case NodeKind::GOTOSTMT_MIR:
        case NodeKind::BREAKSTMT_MIR:
        case NodeKind::CONTSTMT_MIR:
        case NodeKind::RETSTMT_MIR:
            return true;
        default:
            return false;
        }
    }
};

class InitializerMIR : public MIRVisitable<InitializerMIR, MIRNode> {
public:
    struct Member {
        std::string member;
        Box<InitializerMIR> initializer;
    };

    struct Index {
        eval::Value idx;
        Box<InitializerMIR> initializer;
    };

    using InitMIRType =
        std::variant<Box<ExprMIR>, Box<Member>, Box<Index>, Vec<Box<InitializerMIR>>>;

    InitializerMIR(Location loc, Box<ExprMIR> expr)
        : MIRVisitable<InitializerMIR, MIRNode>(loc, NodeKind::INIT_MIR),
          initializer(std::move(expr)) {}

    InitializerMIR(Location loc, std::string mem, Box<InitializerMIR> init)
        : MIRVisitable<InitializerMIR, MIRNode>(loc, NodeKind::INIT_MIR),
          initializer(std::make_unique<Member>(std::move(mem), std::move(init))) {}

    InitializerMIR(Location loc, eval::Value& idx, Box<InitializerMIR> init)
        : MIRVisitable<InitializerMIR, MIRNode>(loc, NodeKind::INIT_MIR),
          initializer(std::make_unique<Index>(idx, std::move(init))) {}

    InitializerMIR(Location loc, Vec<Box<InitializerMIR>> initializers)
        : MIRVisitable<InitializerMIR, MIRNode>(loc, NodeKind::INIT_MIR),
          initializer(std::move(initializers)) {}

    InitMIRType initializer;

    /**
    Check if an initializer is entirely literal expressions.

    An array initializer that is all literals can be optimized.
    */
    bool is_all_literals();

    static bool classof(const MIRNode *node) { return node->kind == NodeKind::INIT_MIR; }
};

class TypeDeclMIR : public MIRVisitable<TypeDeclMIR, DeclMIR> {
public:
    TypeDeclMIR(Location loc, sema::sym::TypeSymbol *sym)
        : MIRVisitable<TypeDeclMIR, DeclMIR>(loc, NodeKind::TYPEDEC_MIR), sym(sym) {}

    sema::sym::TypeSymbol *sym;

    static bool classof(const MIRNode *node) { return node->kind == NodeKind::TYPEDEC_MIR; }
};

// A MIR node containing a single variable declaration and optional initializer.
class VarDeclMIR : public MIRVisitable<VarDeclMIR, DeclMIR> {
public:
    struct VarDecl {
        sema::sym::VarSymbol *sym;
        Optional<Box<InitializerMIR>> initializer;
    };

    VarDeclMIR(Location loc) : MIRVisitable<VarDeclMIR, DeclMIR>(loc, NodeKind::VARDEC_MIR) {}

    Vec<VarDecl> decls;

    void add_decl(sema::sym::VarSymbol *sym);

    void add_decl(sema::sym::VarSymbol *sym, Box<InitializerMIR> init);

    static bool classof(const MIRNode *node) { return node->kind == NodeKind::VARDEC_MIR; }
};

class CompoundStmtMIR : public MIRVisitable<CompoundStmtMIR, StmtMIR> {
public:
    CompoundStmtMIR(Location loc)
        : MIRVisitable<CompoundStmtMIR, StmtMIR>(loc, NodeKind::CMPDSTMT_MIR) {}

    CompoundStmtMIR(Location loc, Vec<Box<ProgItemMIR>> items)
        : MIRVisitable<CompoundStmtMIR, StmtMIR>(loc, NodeKind::CMPDSTMT_MIR),
          items(std::move(items)) {}

    Vec<Box<ProgItemMIR>> items;

    void add_item(Box<ProgItemMIR> item);

    static bool classof(const MIRNode *node) { return node->kind == NodeKind::CMPDSTMT_MIR; }
};

class ExprStmtMIR : public MIRVisitable<ExprStmtMIR, StmtMIR> {
public:
    ExprStmtMIR(Location loc, Box<ExprMIR> expr)
        : MIRVisitable<ExprStmtMIR, StmtMIR>(loc, NodeKind::EXPRSTMT_MIR), expr(std::move(expr)) {}

    ExprStmtMIR(Location loc) : MIRVisitable<ExprStmtMIR, StmtMIR>(loc, NodeKind::EXPRSTMT_MIR) {}

    Optional<Box<ExprMIR>> expr;

    bool is_empty() const { return !expr.has_value(); }

    static bool classof(const MIRNode *node) { return node->kind == NodeKind::EXPRSTMT_MIR; }
};

class SwitchStmtMIR : public MIRVisitable<SwitchStmtMIR, StmtMIR> {
public:
    SwitchStmtMIR(Location loc, Box<ExprMIR> condition, Box<StmtMIR> body)
        : MIRVisitable<SwitchStmtMIR, StmtMIR>(loc, NodeKind::SWITCHSTMT_MIR),
          control_val(std::move(condition)), body(std::move(body)) {}

    Box<ExprMIR> control_val;
    Box<StmtMIR> body;

    static bool classof(const MIRNode *node) { return node->kind == NodeKind::SWITCHSTMT_MIR; }
};

class CaseStmtMIR : public MIRVisitable<CaseStmtMIR, StmtMIR> {
public:
    CaseStmtMIR(Location loc, eval::Value& case_val, Box<StmtMIR> stmt)
        : MIRVisitable<CaseStmtMIR, StmtMIR>(loc, NodeKind::CASESTMT_MIR), case_val(case_val),
          stmt(std::move(stmt)) {}

    eval::Value case_val;
    Box<StmtMIR> stmt;

    static bool classof(const MIRNode *node) { return node->kind == NodeKind::CASESTMT_MIR; }
};

class CaseRangeStmtMIR : public MIRVisitable<CaseRangeStmtMIR, StmtMIR> {
public:
    CaseRangeStmtMIR(
        Location loc, eval::Value& case_start, eval::Value& case_end, Box<StmtMIR> stmt)
        : MIRVisitable<CaseRangeStmtMIR, StmtMIR>(loc, NodeKind::CASERGSTMT_MIR),
          case_start(case_start), case_end(case_end), stmt(std::move(stmt)) {}

    eval::Value case_start;
    eval::Value case_end;
    Box<StmtMIR> stmt;

    static bool classof(const MIRNode *node) { return node->kind == NodeKind::CASERGSTMT_MIR; }
};

class DefaultStmtMIR : public MIRVisitable<DefaultStmtMIR, StmtMIR> {
public:
    DefaultStmtMIR(Location loc, Box<StmtMIR> stmt)
        : MIRVisitable<DefaultStmtMIR, StmtMIR>(loc, NodeKind::DEFSTMT_MIR), stmt(std::move(stmt)) {
    }

    Box<StmtMIR> stmt;

    static bool classof(const MIRNode *node) { return node->kind == NodeKind::DEFSTMT_MIR; }
};

class LabeledStmtMIR : public MIRVisitable<LabeledStmtMIR, StmtMIR> {
public:
    LabeledStmtMIR(Location loc, sema::sym::LabelSymbol *label, Box<StmtMIR> stmt)
        : MIRVisitable<LabeledStmtMIR, StmtMIR>(loc, NodeKind::LABSTMT_MIR), label(label),
          stmt(std::move(stmt)) {}

    sema::sym::LabelSymbol *label;
    Box<StmtMIR> stmt;

    static bool classof(const MIRNode *node) { return node->kind == NodeKind::LABSTMT_MIR; }
};

class PrintStmtMIR : public MIRVisitable<PrintStmtMIR, StmtMIR> {
public:
    PrintStmtMIR(Location loc, std::string format_string)
        : MIRVisitable<PrintStmtMIR, StmtMIR>(loc, NodeKind::PRINTSTMT_MIR),
          format_string(std::move(format_string)) {}

    PrintStmtMIR(Location loc, std::string format_string, Vec<Box<ExprMIR>> arguments)
        : MIRVisitable<PrintStmtMIR, StmtMIR>(loc, NodeKind::PRINTSTMT_MIR),
          format_string(std::move(format_string)), arguments(std::move(arguments)) {}

    std::string format_string;
    Vec<Box<ExprMIR>> arguments;

    static bool classof(const MIRNode *node) { return node->kind == NodeKind::PRINTSTMT_MIR; }
};

class IfStmtMIR : public MIRVisitable<IfStmtMIR, StmtMIR> {
public:
    IfStmtMIR(
        Location loc, Box<ExprMIR> condition, Box<StmtMIR> then_branch, Box<StmtMIR> else_branch)
        : MIRVisitable<IfStmtMIR, StmtMIR>(loc, NodeKind::IFSTMT_MIR),
          condition(std::move(condition)), then_branch(std::move(then_branch)),
          else_branch(std::move(else_branch)) {}

    IfStmtMIR(
        Location loc, Box<ExprMIR> condition, Box<StmtMIR> then_branch,
        Optional<Box<StmtMIR>> else_branch)
        : MIRVisitable<IfStmtMIR, StmtMIR>(loc, NodeKind::IFSTMT_MIR),
          condition(std::move(condition)), then_branch(std::move(then_branch)),
          else_branch(std::move(else_branch)) {}

    IfStmtMIR(Location loc, Box<ExprMIR> condition, Box<StmtMIR> then_branch)
        : MIRVisitable<IfStmtMIR, StmtMIR>(loc, NodeKind::IFSTMT_MIR),
          condition(std::move(condition)), then_branch(std::move(then_branch)) {}

    Box<ExprMIR> condition;
    Box<StmtMIR> then_branch;
    Optional<Box<StmtMIR>> else_branch;

    static bool classof(const MIRNode *node) { return node->kind == NodeKind::IFSTMT_MIR; }
};

/*
A basic loop that all loops expand into.
*/
class LoopStmtMIR : public MIRVisitable<LoopStmtMIR, StmtMIR> {
public:
    LoopStmtMIR(Location loc, Box<StmtMIR> body)
        : MIRVisitable<LoopStmtMIR, StmtMIR>(loc, NodeKind::LOOPSTMT_MIR), body(std::move(body)) {}

    LoopStmtMIR(
        Location loc, Optional<Box<ProgItemMIR>> init, Optional<Box<ExprMIR>> condition,
        Optional<Box<StmtMIR>> step, Box<StmtMIR> body, bool is_dowhile)
        : MIRVisitable<LoopStmtMIR, StmtMIR>(loc, NodeKind::LOOPSTMT_MIR), init(std::move(init)),
          condition(std::move(condition)), step(std::move(step)), body(std::move(body)),
          is_dowhile(is_dowhile) {}

    LoopStmtMIR(Location loc, Box<ExprMIR> condition, Box<StmtMIR> body, bool is_dowhile)
        : MIRVisitable<LoopStmtMIR, StmtMIR>(loc, NodeKind::LOOPSTMT_MIR),
          condition(std::move(condition)), body(std::move(body)), is_dowhile(is_dowhile) {}

    /*
    Initialization code at the start of the loop.
    Only for loops should be using this.
    ProgItem is used because the init code can be a variable declaration
    or an expression statement.
    */
    Optional<Box<ProgItemMIR>> init;
    /*
    The condition needed for the loop to continue.

    Can be missing (in the case of a for loop with no condition).
    */
    Optional<Box<ExprMIR>> condition;
    /*
    The step condition for updating a sentinel value in the loop.
    Only needed for `for` loops.
    */
    Optional<Box<StmtMIR>> step;
    // The actual body of the loop.
    Box<StmtMIR> body;
    bool is_dowhile = false;

    static bool classof(const MIRNode *node) { return node->kind == NodeKind::LOOPSTMT_MIR; }
};

class GotoStmtMIR : public MIRVisitable<GotoStmtMIR, StmtMIR> {
public:
    GotoStmtMIR(Location loc, std::string target)
        : MIRVisitable<GotoStmtMIR, StmtMIR>(loc, NodeKind::GOTOSTMT_MIR),
          target(std::move(target)) {}

    /*
    Since goto's can occur before their label is declared, do not resolve the
    label now, resolve it at validation, after the entire symbol table is complete.
    */

    // The plain target to resolve to.
    std::string target;

    // The resolved target symbol.
    sym::LabelSymbol *target_sym = nullptr;

    static bool classof(const MIRNode *node) { return node->kind == NodeKind::GOTOSTMT_MIR; }
};

class BreakStmtMIR : public MIRVisitable<BreakStmtMIR, StmtMIR> {
public:
    BreakStmtMIR(Location loc)
        : MIRVisitable<BreakStmtMIR, StmtMIR>(loc, NodeKind::BREAKSTMT_MIR) {}

    static bool classof(const MIRNode *node) { return node->kind == NodeKind::BREAKSTMT_MIR; }
};

class ContStmtMIR : public MIRVisitable<ContStmtMIR, StmtMIR> {
public:
    ContStmtMIR(Location loc) : MIRVisitable<ContStmtMIR, StmtMIR>(loc, NodeKind::CONTSTMT_MIR) {}

    static bool classof(const MIRNode *node) { return node->kind == NodeKind::CONTSTMT_MIR; }
};

class ReturnStmtMIR : public MIRVisitable<ReturnStmtMIR, StmtMIR> {
public:
    ReturnStmtMIR(Location loc)
        : MIRVisitable<ReturnStmtMIR, StmtMIR>(loc, NodeKind::RETSTMT_MIR) {}

    ReturnStmtMIR(Location loc, Box<ExprMIR> ret_expr)
        : MIRVisitable<ReturnStmtMIR, StmtMIR>(loc, NodeKind::RETSTMT_MIR),
          ret_expr(std::move(ret_expr)) {}

    Optional<Box<ExprMIR>> ret_expr;

    static bool classof(const MIRNode *node) { return node->kind == NodeKind::RETSTMT_MIR; }
};

class BinaryExprMIR : public MIRVisitable<BinaryExprMIR, ExprMIR> {
public:
    BinaryExprMIR(
        Location loc, sema::sym::Scope *scope, Box<ExprMIR> left, Box<ExprMIR> right,
        tokens::BinaryOp op)
        : MIRVisitable<BinaryExprMIR, ExprMIR>(loc, NodeKind::BINEXPR_MIR, scope),
          left(std::move(left)), right(std::move(right)), op(op) {}

    Box<ExprMIR> left;
    Box<ExprMIR> right;
    tokens::BinaryOp op;

    bool is_assignable() override { return false; }

    bool is_lvalue() override { return false; }

    bool is_const_foldable() override {
        return left->is_const_foldable() && right->is_const_foldable();
    }

    eval::Value eval(eval::ExprEvaluator& ev) override;

    static bool classof(const MIRNode *node) { return node->kind == NodeKind::BINEXPR_MIR; }
};

class UnaryExprMIR : public MIRVisitable<UnaryExprMIR, ExprMIR> {
public:
    UnaryExprMIR(Location loc, sema::sym::Scope *scope, Box<ExprMIR> operand, tokens::UnaryOp op)
        : MIRVisitable<UnaryExprMIR, ExprMIR>(loc, NodeKind::UNEXPR_MIR, scope),
          operand(std::move(operand)), op(op) {}

    Box<ExprMIR> operand;
    tokens::UnaryOp op;

    bool is_assignable() override { return is_lvalue() && !act_type->is_const(); }

    bool is_lvalue() override { return op == tokens::UnaryOp::DEREF; };

    bool is_const_foldable() override {
        return operand->is_const_foldable() && sema::prim::unaryop_is_const_foldable(op);
    }

    eval::Value eval(eval::ExprEvaluator& ev) override;

    static bool classof(const MIRNode *node) { return node->kind == NodeKind::UNEXPR_MIR; }
};

class CastExprMIR : public MIRVisitable<CastExprMIR, ExprMIR> {
public:
    enum class CastKind : uint8_t {
        Explicit,
        Implicit,
        ArrPtrDecay,
        FuncPtrDecay,
    };

    CastExprMIR(
        Location loc, sema::sym::Scope *scope, sema::types::Type *target, Box<ExprMIR> inner)
        : MIRVisitable<CastExprMIR, ExprMIR>(loc, NodeKind::CASTEXPR_MIR, scope), target(target),
          inner(std::move(inner)) {}

    CastKind castkind = CastKind::Explicit;
    sema::types::Type *target;
    Box<ExprMIR> inner;

    bool is_assignable() override { return false; }

    bool is_lvalue() override { return false; }

    bool is_const_foldable() override { return inner->is_const_foldable(); }

    eval::Value eval(eval::ExprEvaluator& ev) override;

    static bool classof(const MIRNode *node) { return node->kind == NodeKind::CASTEXPR_MIR; }
};

class AssignExprMIR : public MIRVisitable<AssignExprMIR, ExprMIR> {
public:
    AssignExprMIR(
        Location loc, sema::sym::Scope *scope, Box<ExprMIR> left, Box<ExprMIR> right,
        tokens::AssignOp op)
        : MIRVisitable<AssignExprMIR, ExprMIR>(loc, NodeKind::ASSGNEXPR_MIR, scope),
          left(std::move(left)), right(std::move(right)), op(op) {}

    Box<ExprMIR> left;
    Box<ExprMIR> right;
    tokens::AssignOp op;

    bool is_lvalue() override { return false; }

    bool is_const_foldable() override { return false; }

    eval::Value eval(eval::ExprEvaluator& ev) override;

    static bool classof(const MIRNode *node) { return node->kind == NodeKind::ASSGNEXPR_MIR; }
};

class CondExprMIR : public MIRVisitable<CondExprMIR, ExprMIR> {
public:
    CondExprMIR(
        Location loc, sema::sym::Scope *scope, Box<ExprMIR> condition, Box<ExprMIR> true_expr,
        Box<ExprMIR> false_expr)
        : MIRVisitable<CondExprMIR, ExprMIR>(loc, NodeKind::CONDEXPR_MIR, scope),
          condition(std::move(condition)), true_expr(std::move(true_expr)),
          false_expr(std::move(false_expr)) {}

    Box<ExprMIR> condition;
    Box<ExprMIR> true_expr;
    Box<ExprMIR> false_expr;

    bool is_lvalue() override { return false; }

    bool is_const_foldable() override {
        // if condition is const_foldable, we can eliminate one branch
        // so even if both branches are not const foldable, we can still optimize
        return condition->is_const_foldable();
    }

    eval::Value eval(eval::ExprEvaluator& ev) override;

    static bool classof(const MIRNode *node) { return node->kind == NodeKind::CONDEXPR_MIR; }
};

class IdentExprMIR : public MIRVisitable<IdentExprMIR, ExprMIR> {
public:
    IdentExprMIR(Location loc, sema::sym::Scope *scope, sema::sym::PhysicalSymbol *ident)
        : MIRVisitable<IdentExprMIR, ExprMIR>(loc, NodeKind::IDENTEXPR_MIR, scope), ident(ident) {}

    sema::sym::PhysicalSymbol *ident;

    bool is_assignable() override {
        // only non-const, non-array variables (not functions) are assignable.
        return ident->is_var() && ((!ident->get_type()->is_array()) && (!act_type->is_const()));
    }

    bool is_callable() override { return eff_type->is_callable(); }

    bool is_lvalue() override {
        // all physical symbols are lvalues.
        return true;
    }

    bool is_subscriptable() override { return eff_type->is_subscriptable(); }

    bool is_const_foldable() override { return eff_type->is_enum(); }

    eval::Value eval(eval::ExprEvaluator& ev) override;

    static bool classof(const MIRNode *node) { return node->kind == NodeKind::IDENTEXPR_MIR; }
};

class LiteralExprMIR : public MIRVisitable<LiteralExprMIR, ExprMIR> {
public:
    LiteralExprMIR(Location loc, sema::sym::Scope *scope, eval::Value value)
        : MIRVisitable<LiteralExprMIR, ExprMIR>(loc, NodeKind::LITEXPR_MIR, scope), value(value) {}

    LiteralExprMIR(Location loc, sema::sym::Scope *scope, std::string value)
        : MIRVisitable<LiteralExprMIR, ExprMIR>(loc, NodeKind::LITEXPR_MIR, scope), value(value) {}

    using LitValueMIR = std::variant<eval::Value, std::string>;

    LitValueMIR value;

    bool is_assignable() override { return false; }

    bool is_callable() override { return false; }

    bool is_lvalue() override { return false; }

    bool is_subscriptable() override { return false; }

    bool is_const_foldable() override { return is_primitive(); }

    bool is_primitive() const { return std::holds_alternative<eval::Value>(value); }

    bool is_string() const { return std::holds_alternative<std::string>(value); }

    eval::Value eval(eval::ExprEvaluator& ev) override;

    static bool classof(const MIRNode *node) { return node->kind == NodeKind::LITEXPR_MIR; }
};

class CallExprMIR : public MIRVisitable<CallExprMIR, ExprMIR> {
public:
    CallExprMIR(Location loc, sema::sym::Scope *scope, Box<ExprMIR> callee)
        : MIRVisitable<CallExprMIR, ExprMIR>(loc, NodeKind::CALLEXPR_MIR, scope),
          callee(std::move(callee)) {}

    CallExprMIR(Location loc, sema::sym::Scope *scope, Box<ExprMIR> callee, Vec<Box<ExprMIR>> args)
        : MIRVisitable<CallExprMIR, ExprMIR>(loc, NodeKind::CALLEXPR_MIR, scope),
          callee(std::move(callee)), args(std::move(args)) {}

    Box<ExprMIR> callee;
    Vec<Box<ExprMIR>> args;

    bool is_const_foldable() override { return false; }

    eval::Value eval(eval::ExprEvaluator& ev) override;

    static bool classof(const MIRNode *node) { return node->kind == NodeKind::CALLEXPR_MIR; }
};

class MemberAccExprMIR : public MIRVisitable<MemberAccExprMIR, ExprMIR> {
public:
    MemberAccExprMIR(
        Location loc, sema::sym::Scope *scope, Box<ExprMIR> object, std::string member,
        bool is_arrow)
        : MIRVisitable<MemberAccExprMIR, ExprMIR>(loc, NodeKind::MEMACCEXPR_MIR, scope),
          object(std::move(object)), member(std::move(member)), is_arrow(is_arrow) {}

    Box<ExprMIR> object;
    std::string member;
    bool is_arrow;

    bool is_lvalue() override {
        if (!is_arrow) {
            return object->is_lvalue();
        } else {
            return true;
        }
    }

    bool is_assignable() override {
        // must be lvalue, and neither the object or member is const
        return is_lvalue() && !(object->act_type->is_const() || act_type->is_const());
    }

    bool is_const_foldable() override { return false; }

    eval::Value eval(eval::ExprEvaluator& ev) override;

    static bool classof(const MIRNode *node) { return node->kind == NodeKind::MEMACCEXPR_MIR; }
};

class ReintExprMIR : public MIRVisitable<ReintExprMIR, ExprMIR> {
public:
    ReintExprMIR(
        Location loc, sema::sym::Scope *scope, Box<ExprMIR> object, tokens::PrimType target,
        bool is_arrow)
        : MIRVisitable<ReintExprMIR, ExprMIR>(loc, NodeKind::REINTEXPR_MIR, scope),
          object(std::move(object)), target(target), is_arrow(is_arrow) {}

    Box<ExprMIR> object;
    tokens::PrimType target;
    bool is_arrow;

    bool is_lvalue() override {
        if (!is_arrow) {
            return object->is_lvalue();
        } else {
            return true;
        }
    }

    bool is_assignable() override {
        // must be lvalue, and neither the object or member is const
        return is_lvalue() && !(object->act_type->is_const() || act_type->is_const());
    }

    bool is_const_foldable() override { return false; }

    eval::Value eval(eval::ExprEvaluator& ev) override;

    static bool classof(const MIRNode *node) { return node->kind == NodeKind::REINTEXPR_MIR; }
};

class SubscrExprMIR : public MIRVisitable<SubscrExprMIR, ExprMIR> {
public:
    SubscrExprMIR(Location loc, sema::sym::Scope *scope, Box<ExprMIR> array, Box<ExprMIR> index)
        : MIRVisitable<SubscrExprMIR, ExprMIR>(loc, NodeKind::SUBSCREXPR_MIR, scope),
          array(std::move(array)), index(std::move(index)) {}

    Box<ExprMIR> array;
    Box<ExprMIR> index;

    bool is_lvalue() override { return true; }

    bool is_assignable() override { return !act_type->is_const(); }

    bool is_const_foldable() override { return false; }

    eval::Value eval(eval::ExprEvaluator& ev) override;

    static bool classof(const MIRNode *node) { return node->kind == NodeKind::SUBSCREXPR_MIR; }
};

class PostfixExprMIR : public MIRVisitable<PostfixExprMIR, ExprMIR> {
public:
    PostfixExprMIR(
        Location loc, sema::sym::Scope *scope, Box<ExprMIR> operand, tokens::PostfixOp op)
        : MIRVisitable<PostfixExprMIR, ExprMIR>(loc, NodeKind::PFIXEXPR_MIR, scope),
          operand(std::move(operand)), op(op) {}

    Box<ExprMIR> operand;
    tokens::PostfixOp op;

    bool is_const_foldable() override { return false; }

    eval::Value eval(eval::ExprEvaluator& ev) override;

    static bool classof(const MIRNode *node) { return node->kind == NodeKind::PFIXEXPR_MIR; }
};

class SizeofExprMIR : public MIRVisitable<SizeofExprMIR, ExprMIR> {
public:
    using SizeofOperand = std::variant<Box<ExprMIR>, sema::types::Type *>;

    SizeofExprMIR(Location loc, sema::sym::Scope *scope)
        : MIRVisitable<SizeofExprMIR, ExprMIR>(loc, NodeKind::SIZEEXPR_MIR, scope) {}

    SizeofExprMIR(Location loc, sema::sym::Scope *scope, sema::types::Type *target)
        : MIRVisitable<SizeofExprMIR, ExprMIR>(loc, NodeKind::SIZEEXPR_MIR, scope),
          operand(target) {}

    SizeofExprMIR(Location loc, sema::sym::Scope *scope, Box<ExprMIR> target)
        : MIRVisitable<SizeofExprMIR, ExprMIR>(loc, NodeKind::SIZEEXPR_MIR, scope),
          operand(std::move(target)) {}

    SizeofOperand operand;

    bool is_const_foldable() override { return true; }

    eval::Value eval(eval::ExprEvaluator& ev) override;

    static bool classof(const MIRNode *node) { return node->kind == NodeKind::SIZEEXPR_MIR; }
};

class FunctionMIR : public MIRVisitable<FunctionMIR, ProgItemMIR> {
public:
    FunctionMIR(
        Location loc, sema::sym::FuncSymbol *sym, sema::sym::Scope *scope,
        Box<CompoundStmtMIR> body)
        : MIRVisitable<FunctionMIR, ProgItemMIR>(loc, NodeKind::FUNC_MIR), sym(sym), scope(scope),
          body(std::move(body)) {}

    // The symbol associated with the function.
    // This contains the name and signature.
    sema::sym::FuncSymbol *sym;
    // The scope associated with the function.
    sema::sym::Scope *scope;

    Box<CompoundStmtMIR> body;

    bool is_declaration() const { return body == nullptr; }

    static bool classof(const MIRNode *node) { return node->kind == NodeKind::FUNC_MIR; }
};

class ProgramMIR : public MIRVisitable<ProgramMIR, MIRNode> {
public:
    ProgramMIR() : MIRVisitable<ProgramMIR, MIRNode>(NodeKind::PROG_MIR) {}

    ProgramMIR(Vec<Box<ProgItemMIR>> items)
        : MIRVisitable<ProgramMIR, MIRNode>(NodeKind::PROG_MIR), items(std::move(items)) {}

    Vec<Box<ProgItemMIR>> items;

    void add_item(Box<ProgItemMIR> item);

    static bool classof(const MIRNode *node) { return node->kind == NodeKind::PROG_MIR; }
};

} // namespace ecc::sema::mir

#endif
