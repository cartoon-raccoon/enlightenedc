#ifndef ECC_MIR_H
#define ECC_MIR_H

#include <optional>
#include <variant>

#include "semantics/types.hpp"
#include "semantics/symbols.hpp"
#include "codegen/value.hpp"
#include "util.hpp"
#include "frontend/tokens.hpp"


using namespace ecc;
using namespace util;

namespace ecc::compiler::mir {
/*
Middle representation functionality.
*/

class MIRVisitor;

/*
A simpler version of the AST, mapping symbols directly to types.
*/
class MIRNode {
public:
    enum MIRNodeKind {
        PROG_MIR,
        FUNC_MIR,
        INIT_MIR,
        TYPEDEC_MIR,
        VARDEC_MIR,
        CMPDSTMT_MIR,
        EXPRSTMT_MIR,
        SWITCHSTMT_MIR,
        CASESTMT_MIR,
        DEFSTMT_MIR,
        LABSTMT_MIR,
        PRINTSTMT_MIR,
        IFSTMT_MIR,
        LOOPSTMT_MIR,
        GOTOSTMT_MIR,
        BREAKSTMT_MIR,
        RETSTMT_MIR,
        BINEXPR_MIR,
        UNEXPR_MIR,
        CASTEXPR_MIR,
        ASSGNEXPR_MIR,
        CONDEXPR_MIR,
        IDENTEXPR_MIR,
        LITEXPR_MIR,
        STREXPR_MIR,
        CALLEXPR_MIR,
        MEMACCEXPR_MIR,
        SUBSCREXPR_MIR,
        PFIXEXPR_MIR,
        SIZEEXPR_MIR,
    };

    MIRNode(Location loc, MIRNodeKind kind) : loc(loc) {}
    virtual ~MIRNode() = default;

    MIRNodeKind kind;
    Location loc;

    virtual void accept(MIRVisitor& visitor) = 0;
};

class ProgramItemMIR : public MIRNode {
public:
    ProgramItemMIR(Location loc, MIRNodeKind kind) : MIRNode(loc, kind) {}

    virtual void accept(MIRVisitor& visitor) = 0;
};

class ExprMIR : public MIRNode {
public:
    ExprMIR(Location loc, MIRNodeKind kind) : MIRNode(loc, kind) {}
    ExprMIR(Location loc, MIRNodeKind kind,
            sema::types::Type *type) : MIRNode(loc, kind), type(type) {}

    sema::types::Type *type;

    // Whether this expression can be assigned to,
    virtual bool is_assignable() { return false; }

    // Whether this expression can be called as a function.
    virtual bool is_callable() { return false; }

    virtual bool is_indexable() { return false; }

    virtual void accept(MIRVisitor& visitor) = 0;
};

class DeclMIR : public ProgramItemMIR {
public:
    DeclMIR(Location loc, MIRNodeKind kind) : ProgramItemMIR(loc, kind) {}
    virtual void accept(MIRVisitor& visitor) = 0;
};

class StmtMIR : public ProgramItemMIR {
public:
    StmtMIR(Location loc, MIRNodeKind kind) : ProgramItemMIR(loc, kind) {}
    virtual void accept(MIRVisitor& visitor) = 0;
};

class InitializerMIR : public MIRNode {
public:
    InitializerMIR(Location loc, Box<ExprMIR> expr)
        : MIRNode(loc, INIT_MIR), initializer(std::move(expr)) {}
    InitializerMIR(Location loc, Vec<Box<InitializerMIR>> initializers)
        : MIRNode(loc, INIT_MIR), initializer(std::move(initializers)) {}
    
    std::variant<Box<ExprMIR>, Vec<Box<InitializerMIR>>> initializer;

    void accept(MIRVisitor& visitor) override;
};

class TypeDeclMIR : public DeclMIR {
public:
    TypeDeclMIR(Location loc, sema::sym::TypeSymbol *sym)
        : DeclMIR(loc, TYPEDEC_MIR), sym(sym) {}
    
    sema::sym::TypeSymbol *sym;

    void accept(MIRVisitor& visitor) override;
};

// A MIR node containing a single variable declaration and optional initializer.
class VarDeclMIR : public DeclMIR {
public:
    VarDeclMIR(Location loc, sema::sym::VarSymbol *sym)
        : DeclMIR(loc, VARDEC_MIR), sym(sym) {}
    
    VarDeclMIR(Location loc, sema::sym::VarSymbol *sym, Box<InitializerMIR> init)
        : DeclMIR(loc, VARDEC_MIR), sym(sym), initializer(std::move(init)) {}
    
    sema::sym::VarSymbol *sym;
    std::optional<Box<InitializerMIR>> initializer;

    void accept(MIRVisitor& visitor) override;
};

class CompoundStmtMIR : public StmtMIR {
public:
    CompoundStmtMIR(Location loc)
        : StmtMIR(loc, CMPDSTMT_MIR), items() {}
    
    CompoundStmtMIR(Location loc, Vec<Box<ProgramItemMIR>> items)
        : StmtMIR(loc, CMPDSTMT_MIR), items(std::move(items)) {}
    
    Vec<Box<ProgramItemMIR>> items;

    void add_item(Box<ProgramItemMIR> item);

    void accept(MIRVisitor& visitor) override;
};

class ExprStmtMIR : public StmtMIR {
public:
    ExprStmtMIR(Location loc, Box<ExprMIR> expr)
        : StmtMIR(loc, EXPRSTMT_MIR), expr(std::move(expr)) {}
    
    ExprStmtMIR(Location loc)
        : StmtMIR(loc, EXPRSTMT_MIR) {}
    
    std::optional<Box<ExprMIR>> expr;

    bool is_empty() { return expr.has_value(); }

    void accept(MIRVisitor& visitor) override;
};

class SwitchStmtMIR : public StmtMIR {
public:
    SwitchStmtMIR(Location loc, Box<ExprMIR> condition, Box<StmtMIR> body)
        : StmtMIR(loc, SWITCHSTMT_MIR), 
        condition(std::move(condition)), body(std::move(body)) {}
    
    Box<ExprMIR> condition;
    Box<StmtMIR> body;

    void accept(MIRVisitor& visitor) override;
};

class CaseRangeStmtMIR : public StmtMIR {
public:
    CaseRangeStmtMIR(Location loc, Box<exec::Value> case_val, Box<StmtMIR> stmt)
        : StmtMIR(loc, CASESTMT_MIR), 
        case_start(std::make_unique<exec::Value>(*case_val)),
        case_end(std::move(case_val)),
        stmt(std::move(stmt)) {}

    CaseRangeStmtMIR(Location loc, 
                     Box<exec::Value> case_start, 
                     Box<exec::Value> case_end,
                     Box<StmtMIR> stmt)
        : StmtMIR(loc, CASESTMT_MIR), 
        case_start(std::move(case_start)), 
        case_end(std::move(case_end)),
        stmt(std::move(stmt)) {}

    Box<exec::Value> case_start;
    Box<exec::Value> case_end;
    Box<StmtMIR> stmt;

    void accept(MIRVisitor& visitor) override;
};

class DefaultStmtMIR : public StmtMIR {
public:
    DefaultStmtMIR(Location loc, Box<StmtMIR> stmt)
        : StmtMIR(loc, DEFSTMT_MIR), stmt(std::move(stmt)) {}
    
    Box<StmtMIR> stmt;

    void accept(MIRVisitor& visitor) override;
};

class LabeledStmtMIR : public StmtMIR {
public:
    LabeledStmtMIR(Location loc, sema::sym::LabelSymbol *label, Box<StmtMIR> stmt)
        : StmtMIR(loc, LABSTMT_MIR), label(label), stmt(std::move(stmt)) {}

    sema::sym::LabelSymbol *label;
    Box<StmtMIR> stmt;

    void accept(MIRVisitor& visitor) override;
};

class PrintStmtMIR : public StmtMIR {
public:
    PrintStmtMIR(Location loc, std::string format_string, Vec<Box<ExprMIR>> arguments)
        : StmtMIR(loc, PRINTSTMT_MIR), format_string(format_string), arguments(std::move(arguments)) {}
    
    std::string format_string;
    Vec<Box<ExprMIR>> arguments;

    void accept(MIRVisitor& visitor) override;
};

class IfStmtMIR : public StmtMIR {
public:
    IfStmtMIR(Location loc, 
              Box<ExprMIR> condition, Box<StmtMIR> then_branch, 
              Box<StmtMIR> else_branch)
        : StmtMIR(loc, IFSTMT_MIR), 
        condition(std::move(condition)), 
        then_branch(std::move(then_branch)),
        else_branch(std::move(else_branch)) {}

    IfStmtMIR(Location loc, 
              Box<ExprMIR> condition, Box<StmtMIR> then_branch, 
              std::optional<Box<StmtMIR>> else_branch)
        : StmtMIR(loc, IFSTMT_MIR), 
        condition(std::move(condition)), 
        then_branch(std::move(then_branch)),
        else_branch(std::move(else_branch)) {}

    IfStmtMIR(Location loc, 
              Box<ExprMIR> condition, Box<StmtMIR> then_branch)
        : StmtMIR(loc, IFSTMT_MIR), 
        condition(std::move(condition)), 
        then_branch(std::move(then_branch)) {}
    
    Box<ExprMIR> condition;
    Box<StmtMIR> then_branch;
    std::optional<Box<StmtMIR>> else_branch;

    void accept(MIRVisitor& visitor) override;
};

/*
A basic loop that all loops expand into.
*/
class LoopStmtMIR : public StmtMIR {
public:
    LoopStmtMIR(Location loc, Box<StmtMIR> init,
                Box<ExprMIR> condition, Box<StmtMIR> step,
                Box<LabeledStmtMIR> body,
                bool is_dowhile)
        : StmtMIR(loc, LOOPSTMT_MIR),
        init(std::move(init)),
        condition(std::move(condition)),
        step(std::move(step)),
        body(std::move(body)),
        is_dowhile(is_dowhile) {}
    
    LoopStmtMIR(Location loc, Box<ExprMIR> condition, 
                Box<LabeledStmtMIR> body, bool is_dowhile)
        : StmtMIR(loc, LOOPSTMT_MIR),
        condition(std::move(condition)),
        body(std::move(body)),
        is_dowhile(is_dowhile) {}
    
    std::optional<Box<StmtMIR>> init; // only needed for for loops.
    Box<ExprMIR> condition;
    std::optional<Box<StmtMIR>> step; // only needed for for loops.
    Box<LabeledStmtMIR> body;
    bool is_dowhile;

    void accept(MIRVisitor& visitor) override;
};

class GotoStmtMIR : public StmtMIR {
public:
    GotoStmtMIR(Location loc, sema::sym::LabelSymbol *target)
        : StmtMIR(loc, GOTOSTMT_MIR), target(target) {}
    
    sema::sym::LabelSymbol *target;

    void accept(MIRVisitor& visitor) override;
};

class BreakStmtMIR : public StmtMIR {
public:
    BreakStmtMIR(Location loc) : StmtMIR(loc, BREAKSTMT_MIR) {}
    
    void accept(MIRVisitor& visitor) override;
};

class ReturnStmtMIR : public StmtMIR {
public:
    ReturnStmtMIR(Location loc, Box<ExprMIR> ret_expr)
        : StmtMIR(loc, RETSTMT_MIR), ret_expr(std::move(ret_expr)) {}
    
    Box<ExprMIR> ret_expr;

    void accept(MIRVisitor& visitor) override;
};

class BinaryExprMIR : public ExprMIR {
public:
    BinaryExprMIR(Location loc, 
                  Box<ExprMIR> left, Box<ExprMIR> right, 
                  tokens::BinaryOp op)
        : ExprMIR(loc, BINEXPR_MIR), 
        left(std::move(left)), right(std::move(right)), 
        op(op) {}
    
    Box<ExprMIR> left;
    Box<ExprMIR> right;
    tokens::BinaryOp op;

    bool is_assignable() override { return false; }

    void accept(MIRVisitor& visitor) override;
};

class UnaryExprMIR : public ExprMIR {
public:
    UnaryExprMIR(Location loc, Box<ExprMIR> operand, tokens::UnaryOp op)
        : ExprMIR(loc, UNEXPR_MIR), operand(std::move(operand)), op(op) {}
    
    Box<ExprMIR> operand;
    tokens::UnaryOp op;

    void accept(MIRVisitor& visitor) override;
};

class CastExprMIR : public ExprMIR {
public:
    CastExprMIR(Location loc, sema::types::Type *target, Box<ExprMIR> inner)
        : ExprMIR(loc, CASTEXPR_MIR), target(target), inner(std::move(inner)) {}
    
    sema::types::Type *target;
    Box<ExprMIR> inner;

    void accept(MIRVisitor& visitor) override;
};

class AssignExprMIR : public ExprMIR {
public:
    AssignExprMIR(Location loc, 
                  Box<ExprMIR> left, Box<ExprMIR> right, 
                  tokens::AssignOp op)
        : ExprMIR(loc, ASSGNEXPR_MIR), 
        left(std::move(left)), right(std::move(right)), 
        op(op) {}

    Box<ExprMIR> left;
    Box<ExprMIR> right;
    tokens::AssignOp op;

    void accept(MIRVisitor& visitor) override;
};

class CondExprMIR : public ExprMIR {
public:
    CondExprMIR(Location loc, Box<ExprMIR> condition,
                Box<ExprMIR> true_expr, Box<ExprMIR> false_expr)
        : ExprMIR(loc, CONDEXPR_MIR), true_expr(std::move(true_expr)),
        false_expr(std::move(false_expr)) {}

    Box<ExprMIR> condition;
    Box<ExprMIR> true_expr;
    Box<ExprMIR> false_expr;

    void accept(MIRVisitor& visitor) override;
};

class IdentExprMIR : public ExprMIR {
public:
    IdentExprMIR(Location loc, sema::sym::Symbol *ident)
        : ExprMIR(loc, IDENTEXPR_MIR), ident(ident) {}

    sema::sym::Symbol *ident;

    void accept(MIRVisitor& visitor) override;
};

class LiteralExprMIR : public ExprMIR {
public:
    LiteralExprMIR(Location loc, Box<exec::Value> value)
        : ExprMIR(loc, LITEXPR_MIR), value(std::move(value)) {}

    Box<exec::Value> value;

    void accept(MIRVisitor& visitor) override;
};

class StringExprMIR : public ExprMIR {
public:
    StringExprMIR(Location loc, std::string value)
        : ExprMIR(loc, STREXPR_MIR), value(value) {}

    std::string value;

    void accept(MIRVisitor& visitor) override;
};

class CallExprMIR : public ExprMIR {
public:
    CallExprMIR(Location loc, Box<ExprMIR> callee, Vec<Box<ExprMIR>> args)
        : ExprMIR(loc, CALLEXPR_MIR), callee(std::move(callee)), args(std::move(args)) {}

    Box<ExprMIR> callee;
    Vec<Box<ExprMIR>> args;

    void accept(MIRVisitor& visitor) override;
};

class MemberAccExprMIR : public ExprMIR {
public:
    MemberAccExprMIR(Location loc, Box<ExprMIR> object, std::string member)
        : ExprMIR(loc, MEMACCEXPR_MIR), object(std::move(object)), member(member) {}
    
    Box<ExprMIR> object;
    std::string member;

    void accept(MIRVisitor& visitor) override;
};

class SubscrExprMIR : public ExprMIR {
public:
    SubscrExprMIR(Location loc, Box<ExprMIR> array, Box<ExprMIR> index)
        : ExprMIR(loc, SUBSCREXPR_MIR), 
        array(std::move(array)), index(std::move(index)) {}

    Box<ExprMIR> array;
    Box<ExprMIR> index;

    void accept(MIRVisitor& visitor) override;
};

class PostfixExprMIR : public ExprMIR {
public:
    PostfixExprMIR(Location loc, Box<ExprMIR> operand, tokens::PostfixOp op)
        : ExprMIR(loc, PFIXEXPR_MIR), operand(std::move(operand)), op(op) {}

    Box<ExprMIR> operand;
    tokens::PostfixOp op;

    void accept(MIRVisitor& visitor) override;
};

class SizeofExprMIR : public ExprMIR {
public:
    SizeofExprMIR(Location loc, sema::types::Type *target)
        : ExprMIR(loc, SIZEEXPR_MIR), target(target) {}
    
    sema::types::Type *target;

    void accept(MIRVisitor& visitor) override;
};

class FunctionMIR : public ProgramItemMIR {
public:
    FunctionMIR(Location loc,
                sema::sym::FuncSymbol *sym,
                sema::sym::Scope *scope,
                Box<CompoundStmtMIR> body)
        : ProgramItemMIR(loc, FUNC_MIR),
        sym(sym), scope(scope), body(std::move(body)) {}
    
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
    ProgramMIR(Location loc) : MIRNode(loc, PROG_MIR), items() {}

    ProgramMIR(Location loc, Vec<Box<ProgramItemMIR>> items)
        : MIRNode(loc, PROG_MIR), items(std::move(items)) {}
    
    Vec<Box<ProgramItemMIR>> items;

    void add_item(Box<ProgramItemMIR> item);

    void accept(MIRVisitor& visitor) override;
};

}

#endif