#pragma once

#ifndef ECC_MIR_H
#define ECC_MIR_H

#include <variant>

#include "eval/evaluator.hpp"
#include "eval/value.hpp"
#include "semantics/primitives.hpp"
#include "semantics/symbols.hpp"
#include "semantics/types.hpp"
#include "tokens.hpp"
#include "util.hpp"

using namespace ecc;
using namespace util;

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

class MIRVisitor;

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
};

class DeclMIR : public ProgItemMIR {
public:
    DeclMIR(Location loc, NodeKind kind) : ProgItemMIR(loc, kind) {}
};

class StmtMIR : public ProgItemMIR {
public:
    StmtMIR(Location loc, NodeKind kind) : ProgItemMIR(loc, kind) {}
};

class InitializerMIR : public MIRNode {
public:
    struct Member {
        std::string member;
        Box<InitializerMIR> initializer;
    };

    struct Index {
        eval::Value idx;
        Box<InitializerMIR> initializer;
    };

    using InitMIRType = std::variant<Box<ExprMIR>, Box<Member>, Box<Index>, Vec<Box<InitializerMIR>>>;

    InitializerMIR(Location loc, Box<ExprMIR> expr)
        : MIRNode(loc, NodeKind::INIT_MIR), initializer(std::move(expr)) {}

    InitializerMIR(Location loc, std::string mem, Box<InitializerMIR> init)
        : MIRNode(loc, NodeKind::INIT_MIR), 
        initializer(std::make_unique<Member>(std::move(mem), std::move(init))) {}

    InitializerMIR(Location loc, eval::Value& idx, Box<InitializerMIR> init)
        : MIRNode(loc, NodeKind::INIT_MIR), 
        initializer(std::make_unique<Index>(idx, std::move(init))) {}

    InitializerMIR(Location loc, Vec<Box<InitializerMIR>> initializers)
        : MIRNode(loc, NodeKind::INIT_MIR), initializer(std::move(initializers)) {}

    InitMIRType initializer;

    /**
    Check if an initializer is entirely literal expressions.

    An array initializer that is all literals can be optimized.
    */
    bool is_all_literals();

    void accept(MIRVisitor& visitor) override;
};

class TypeDeclMIR : public DeclMIR {
public:
    TypeDeclMIR(Location loc, sema::sym::TypeSymbol *sym)
        : DeclMIR(loc, NodeKind::TYPEDEC_MIR), sym(sym) {}

    sema::sym::TypeSymbol *sym;

    void accept(MIRVisitor& visitor) override;
};

// A MIR node containing a single variable declaration and optional initializer.
class VarDeclMIR : public DeclMIR {
public:
    struct VarDecl {
        sema::sym::VarSymbol *sym;
        Optional<Box<InitializerMIR>> initializer;
    };

    VarDeclMIR(Location loc) : DeclMIR(loc, NodeKind::VARDEC_MIR) {}

    Vec<VarDecl> decls;

    void add_decl(sema::sym::VarSymbol *sym);

    void add_decl(sema::sym::VarSymbol *sym, Box<InitializerMIR> init);

    void accept(MIRVisitor& visitor) override;
};

class CompoundStmtMIR : public StmtMIR {
public:
    CompoundStmtMIR(Location loc) : StmtMIR(loc, NodeKind::CMPDSTMT_MIR) {}

    CompoundStmtMIR(Location loc, Vec<Box<ProgItemMIR>> items)
        : StmtMIR(loc, NodeKind::CMPDSTMT_MIR), items(std::move(items)) {}

    Vec<Box<ProgItemMIR>> items;

    void add_item(Box<ProgItemMIR> item);

    void accept(MIRVisitor& visitor) override;
};

class ExprStmtMIR : public StmtMIR {
public:
    ExprStmtMIR(Location loc, Box<ExprMIR> expr)
        : StmtMIR(loc, NodeKind::EXPRSTMT_MIR), expr(std::move(expr)) {}

    ExprStmtMIR(Location loc) : StmtMIR(loc, NodeKind::EXPRSTMT_MIR) {}

    Optional<Box<ExprMIR>> expr;

    bool is_empty() const { return !expr.has_value(); }

    void accept(MIRVisitor& visitor) override;
};

class SwitchStmtMIR : public StmtMIR {
public:
    SwitchStmtMIR(Location loc, Box<ExprMIR> condition, Box<StmtMIR> body)
        : StmtMIR(loc, NodeKind::SWITCHSTMT_MIR), control_val(std::move(condition)),
          body(std::move(body)) {}

    Box<ExprMIR> control_val;
    Box<StmtMIR> body;

    void accept(MIRVisitor& visitor) override;
};

class CaseStmtMIR : public StmtMIR {
public:
    CaseStmtMIR(Location loc, eval::Value& case_val, Box<StmtMIR> stmt)
        : StmtMIR(loc, NodeKind::CASESTMT_MIR), case_val(case_val), stmt(std::move(stmt)) {}

    eval::Value case_val;
    Box<StmtMIR> stmt;

    void accept(MIRVisitor& visitor) override;
};

class CaseRangeStmtMIR : public StmtMIR {
public:
    CaseRangeStmtMIR(Location loc, eval::Value& case_start, eval::Value& case_end,
                     Box<StmtMIR> stmt)
        : StmtMIR(loc, NodeKind::CASERGSTMT_MIR), case_start(case_start), case_end(case_end),
          stmt(std::move(stmt)) {}

    eval::Value case_start;
    eval::Value case_end;
    Box<StmtMIR> stmt;

    void accept(MIRVisitor& visitor) override;
};

class DefaultStmtMIR : public StmtMIR {
public:
    DefaultStmtMIR(Location loc, Box<StmtMIR> stmt)
        : StmtMIR(loc, NodeKind::DEFSTMT_MIR), stmt(std::move(stmt)) {}

    Box<StmtMIR> stmt;

    void accept(MIRVisitor& visitor) override;
};

class LabeledStmtMIR : public StmtMIR {
public:
    LabeledStmtMIR(Location loc, sema::sym::LabelSymbol *label, Box<StmtMIR> stmt)
        : StmtMIR(loc, NodeKind::LABSTMT_MIR), label(label), stmt(std::move(stmt)) {}

    sema::sym::LabelSymbol *label;
    Box<StmtMIR> stmt;

    void accept(MIRVisitor& visitor) override;
};

class PrintStmtMIR : public StmtMIR {
public:
    PrintStmtMIR(Location loc, std::string format_string)
        : StmtMIR(loc, NodeKind::PRINTSTMT_MIR), format_string(std::move(format_string)) {}

    PrintStmtMIR(Location loc, std::string format_string, Vec<Box<ExprMIR>> arguments)
        : StmtMIR(loc, NodeKind::PRINTSTMT_MIR), format_string(std::move(format_string)),
          arguments(std::move(arguments)) {}

    std::string format_string;
    Vec<Box<ExprMIR>> arguments;

    void accept(MIRVisitor& visitor) override;
};

class IfStmtMIR : public StmtMIR {
public:
    IfStmtMIR(Location loc, Box<ExprMIR> condition, Box<StmtMIR> then_branch,
              Box<StmtMIR> else_branch)
        : StmtMIR(loc, NodeKind::IFSTMT_MIR), condition(std::move(condition)),
          then_branch(std::move(then_branch)), else_branch(std::move(else_branch)) {}

    IfStmtMIR(Location loc, Box<ExprMIR> condition, Box<StmtMIR> then_branch,
              Optional<Box<StmtMIR>> else_branch)
        : StmtMIR(loc, NodeKind::IFSTMT_MIR), condition(std::move(condition)),
          then_branch(std::move(then_branch)), else_branch(std::move(else_branch)) {}

    IfStmtMIR(Location loc, Box<ExprMIR> condition, Box<StmtMIR> then_branch)
        : StmtMIR(loc, NodeKind::IFSTMT_MIR), condition(std::move(condition)),
          then_branch(std::move(then_branch)) {}

    Box<ExprMIR> condition;
    Box<StmtMIR> then_branch;
    Optional<Box<StmtMIR>> else_branch;

    void accept(MIRVisitor& visitor) override;
};

/*
A basic loop that all loops expand into.
*/
class LoopStmtMIR : public StmtMIR {
public:
    LoopStmtMIR(Location loc, Box<StmtMIR> body)
        : StmtMIR(loc, NodeKind::LOOPSTMT_MIR), body(std::move(body)) {}

    LoopStmtMIR(Location loc, Optional<Box<ProgItemMIR>> init, Optional<Box<ExprMIR>> condition,
                Optional<Box<StmtMIR>> step, Box<StmtMIR> body, bool is_dowhile)
        : StmtMIR(loc, NodeKind::LOOPSTMT_MIR), init(std::move(init)),
          condition(std::move(condition)), step(std::move(step)), body(std::move(body)),
          is_dowhile(is_dowhile) {}

    LoopStmtMIR(Location loc, Box<ExprMIR> condition, Box<StmtMIR> body, bool is_dowhile)
        : StmtMIR(loc, NodeKind::LOOPSTMT_MIR), condition(std::move(condition)),
          body(std::move(body)), is_dowhile(is_dowhile) {}

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

    void accept(MIRVisitor& visitor) override;
};

class GotoStmtMIR : public StmtMIR {
public:
    GotoStmtMIR(Location loc, std::string target)
        : StmtMIR(loc, NodeKind::GOTOSTMT_MIR), target(std::move(target)) {}

    /*
    Since goto's can occur before their label is declared, do not resolve the
    label now, resolve it at validation, after the entire symbol table is complete.
    */

    // The plain target to resolve to.
    std::string target;

    // The resolved target symbol.
    sym::LabelSymbol *target_sym = nullptr;

    void accept(MIRVisitor& visitor) override;
};

class BreakStmtMIR : public StmtMIR {
public:
    BreakStmtMIR(Location loc) : StmtMIR(loc, NodeKind::BREAKSTMT_MIR) {}

    void accept(MIRVisitor& visitor) override;
};

class ContStmtMIR : public StmtMIR {
public:
    ContStmtMIR(Location loc) : StmtMIR(loc, NodeKind::CONTSTMT_MIR) {}

    void accept(MIRVisitor& visitor) override;
};

class ReturnStmtMIR : public StmtMIR {
public:
    ReturnStmtMIR(Location loc) : StmtMIR(loc, NodeKind::RETSTMT_MIR) {}

    ReturnStmtMIR(Location loc, Box<ExprMIR> ret_expr)
        : StmtMIR(loc, NodeKind::RETSTMT_MIR), ret_expr(std::move(ret_expr)) {}

    Optional<Box<ExprMIR>> ret_expr;

    void accept(MIRVisitor& visitor) override;
};

class BinaryExprMIR : public ExprMIR {
public:
    BinaryExprMIR(Location loc, sema::sym::Scope *scope, Box<ExprMIR> left, Box<ExprMIR> right,
                  tokens::BinaryOp op)
        : ExprMIR(loc, NodeKind::BINEXPR_MIR, scope), left(std::move(left)),
          right(std::move(right)), op(op) {}

    Box<ExprMIR> left;
    Box<ExprMIR> right;
    tokens::BinaryOp op;

    bool is_assignable() override { return false; }

    bool is_lvalue() override { return false; }

    void accept(MIRVisitor& visitor) override;

    bool is_const_foldable() override {
        return left->is_const_foldable() && right->is_const_foldable();
    }

    eval::Value eval(eval::ExprEvaluator& ev) override;
};

class UnaryExprMIR : public ExprMIR {
public:
    UnaryExprMIR(Location loc, sema::sym::Scope *scope, Box<ExprMIR> operand, tokens::UnaryOp op)
        : ExprMIR(loc, NodeKind::UNEXPR_MIR, scope), operand(std::move(operand)), op(op) {}

    Box<ExprMIR> operand;
    tokens::UnaryOp op;

    bool is_assignable() override { return is_lvalue(); }

    bool is_lvalue() override { return op == tokens::UnaryOp::DEREF; };

    bool is_const_foldable() override {
        return operand->is_const_foldable() && sema::prim::unaryop_is_const_foldable(op);
    }

    void accept(MIRVisitor& visitor) override;

    eval::Value eval(eval::ExprEvaluator& ev) override;
};

class CastExprMIR : public ExprMIR {
public:
    CastExprMIR(Location loc, sema::sym::Scope *scope, sema::types::Type *target,
                Box<ExprMIR> inner)
        : ExprMIR(loc, NodeKind::CASTEXPR_MIR, scope), target(target), inner(std::move(inner)) {}

    sema::types::Type *target;
    Box<ExprMIR> inner;

    bool is_assignable() override { return false; }

    bool is_lvalue() override { return false; }

    bool is_const_foldable() override { return inner->is_const_foldable(); }

    void accept(MIRVisitor& visitor) override;

    eval::Value eval(eval::ExprEvaluator& ev) override;
};

class AssignExprMIR : public ExprMIR {
public:
    AssignExprMIR(Location loc, sema::sym::Scope *scope, Box<ExprMIR> left, Box<ExprMIR> right,
                  tokens::AssignOp op)
        : ExprMIR(loc, NodeKind::ASSGNEXPR_MIR, scope), left(std::move(left)),
          right(std::move(right)), op(op) {}

    Box<ExprMIR> left;
    Box<ExprMIR> right;
    tokens::AssignOp op;

    bool is_const_foldable() override { return false; }

    void accept(MIRVisitor& visitor) override;

    eval::Value eval(eval::ExprEvaluator& ev) override;
};

class CondExprMIR : public ExprMIR {
public:
    CondExprMIR(Location loc, sema::sym::Scope *scope, Box<ExprMIR> condition,
                Box<ExprMIR> true_expr, Box<ExprMIR> false_expr)
        : ExprMIR(loc, NodeKind::CONDEXPR_MIR, scope), condition(std::move(condition)),
          true_expr(std::move(true_expr)), false_expr(std::move(false_expr)) {}

    Box<ExprMIR> condition;
    Box<ExprMIR> true_expr;
    Box<ExprMIR> false_expr;

    bool is_lvalue() override { return false; }

    bool is_const_foldable() override {
        // if condition is const_foldable, we can eliminate one branch
        // so even if both branches are not const foldable, we can still optimize
        return condition->is_const_foldable();
    }

    void accept(MIRVisitor& visitor) override;

    eval::Value eval(eval::ExprEvaluator& ev) override;
};

class IdentExprMIR : public ExprMIR {
public:
    IdentExprMIR(Location loc, sema::sym::Scope *scope, sema::sym::PhysicalSymbol *ident)
        : ExprMIR(loc, NodeKind::IDENTEXPR_MIR, scope), ident(ident) {}

    sema::sym::PhysicalSymbol *ident;

    bool is_assignable() override { return is_lvalue(); }

    bool is_callable() override { return eff_type->is_callable(); }

    bool is_lvalue() override { return ident->kind == sema::sym::Symbol::Kind::VAR; }

    bool is_subscriptable() override { return eff_type->is_subscriptable(); }

    bool is_const_foldable() override { return eff_type->is_enum(); }

    void accept(MIRVisitor& visitor) override;

    eval::Value eval(eval::ExprEvaluator& ev) override;
};

class LiteralExprMIR : public ExprMIR {
public:
    LiteralExprMIR(Location loc, sema::sym::Scope *scope, eval::Value value)
        : ExprMIR(loc, NodeKind::LITEXPR_MIR, scope), value(value) {}

    LiteralExprMIR(Location loc, sema::sym::Scope *scope, std::string value)
        : ExprMIR(loc, NodeKind::LITEXPR_MIR, scope), value(value) {}

    using LitValueMIR = std::variant<eval::Value, std::string>;

    LitValueMIR value;

    bool is_assignable() override { return false; }

    bool is_callable() override { return false; }

    bool is_lvalue() override { return false; }

    bool is_subscriptable() override { return false; }

    bool is_const_foldable() override { return true; }

    void accept(MIRVisitor& visitor) override;

    eval::Value eval(eval::ExprEvaluator& ev) override;
};

class CallExprMIR : public ExprMIR {
public:
    CallExprMIR(Location loc, sema::sym::Scope *scope, Box<ExprMIR> callee)
        : ExprMIR(loc, NodeKind::CALLEXPR_MIR, scope), callee(std::move(callee)) {}

    CallExprMIR(Location loc, sema::sym::Scope *scope, Box<ExprMIR> callee, Vec<Box<ExprMIR>> args)
        : ExprMIR(loc, NodeKind::CALLEXPR_MIR, scope), callee(std::move(callee)),
          args(std::move(args)) {}

    Box<ExprMIR> callee;
    Vec<Box<ExprMIR>> args;

    bool is_const_foldable() override { return false; }

    void accept(MIRVisitor& visitor) override;

    eval::Value eval(eval::ExprEvaluator& ev) override;
};

class MemberAccExprMIR : public ExprMIR {
public:
    MemberAccExprMIR(Location loc, sema::sym::Scope *scope, Box<ExprMIR> object, std::string member,
                     bool is_arrow)
        : ExprMIR(loc, NodeKind::MEMACCEXPR_MIR, scope), object(std::move(object)),
          member(std::move(member)), is_arrow(is_arrow) {}

    Box<ExprMIR> object;
    std::string member;
    bool is_arrow;

    bool is_lvalue() override { return true; }

    bool is_const_foldable() override { return false; }

    void accept(MIRVisitor& visitor) override;

    eval::Value eval(eval::ExprEvaluator& ev) override;
};

class SubscrExprMIR : public ExprMIR {
public:
    SubscrExprMIR(Location loc, sema::sym::Scope *scope, Box<ExprMIR> array, Box<ExprMIR> index)
        : ExprMIR(loc, NodeKind::SUBSCREXPR_MIR, scope), array(std::move(array)),
          index(std::move(index)) {}

    Box<ExprMIR> array;
    Box<ExprMIR> index;

    bool is_lvalue() override { return true; }

    bool is_const_foldable() override { return false; }

    void accept(MIRVisitor& visitor) override;

    eval::Value eval(eval::ExprEvaluator& ev) override;
};

class PostfixExprMIR : public ExprMIR {
public:
    PostfixExprMIR(Location loc, sema::sym::Scope *scope, Box<ExprMIR> operand,
                   tokens::PostfixOp op)
        : ExprMIR(loc, NodeKind::PFIXEXPR_MIR, scope), operand(std::move(operand)), op(op) {}

    Box<ExprMIR> operand;
    tokens::PostfixOp op;

    bool is_const_foldable() override { return false; }

    void accept(MIRVisitor& visitor) override;

    eval::Value eval(eval::ExprEvaluator& ev) override;
};

class SizeofExprMIR : public ExprMIR {
public:
    using SizeofOperand = std::variant<Box<ExprMIR>, sema::types::Type *>;

    SizeofExprMIR(Location loc, sema::sym::Scope *scope)
        : ExprMIR(loc, NodeKind::SIZEEXPR_MIR, scope) {}

    SizeofExprMIR(Location loc, sema::sym::Scope *scope, sema::types::Type *target)
        : ExprMIR(loc, NodeKind::SIZEEXPR_MIR, scope), operand(target) {}

    SizeofExprMIR(Location loc, sema::sym::Scope *scope, Box<ExprMIR> target)
        : ExprMIR(loc, NodeKind::SIZEEXPR_MIR, scope), operand(std::move(target)) {}

    SizeofOperand operand;

    bool is_const_foldable() override { return true; }

    void accept(MIRVisitor& visitor) override;

    eval::Value eval(eval::ExprEvaluator& ev) override;
};

class FunctionMIR : public ProgItemMIR {
public:
    FunctionMIR(Location loc, sema::sym::FuncSymbol *sym, sema::sym::Scope *scope,
                Box<CompoundStmtMIR> body)
        : ProgItemMIR(loc, NodeKind::FUNC_MIR), sym(sym), scope(scope), body(std::move(body)) {}

    // The symbol associated with the function.
    // This contains the name and signature.
    sema::sym::FuncSymbol *sym;
    // The scope associated with the function.
    sema::sym::Scope *scope;

    Box<CompoundStmtMIR> body;

    void accept(MIRVisitor& visitor) override;
};

class ProgramMIR : public MIRNode {
public:
    ProgramMIR() : MIRNode(NodeKind::PROG_MIR) {}

    ProgramMIR(Vec<Box<ProgItemMIR>> items)
        : MIRNode(NodeKind::PROG_MIR), items(std::move(items)) {}

    Vec<Box<ProgItemMIR>> items;

    void add_item(Box<ProgItemMIR> item);

    void accept(MIRVisitor& visitor) override;
};

} // namespace ecc::sema::mir

#endif