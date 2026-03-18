#ifndef ECC_MIR_H
#define ECC_MIR_H

#include <optional>
#include <variant>

#include "semantics/types.hpp"
#include "semantics/symbols.hpp"
#include "eval/value.hpp"
#include "util.hpp"
#include "frontend/tokens.hpp"


using namespace ecc;
using namespace util;

namespace ecc::sema::mir {
/*
Middle representation functionality.

The Middle Intermediate Representation, or MIR for short, is a simpler tree-based
structure that contains the semantic structure of the program, without unneccessary
syntactic details that are no longer needed. Symbols are mapped directly to Symbol
objects in the SymbolTable, and Expressions are directly tagged with Types.

The MIR is where most semantic validation steps are performed, such as typechecking
and some desugaring.
*/

class MIRVisitor;

/*
A simpler version of the AST, mapping symbols directly to types.
*/
class MIRNode {
public:
    enum class MIRNodeKind {
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
        CONSTEXPR_MIR,
        LITEXPR_MIR,
        STREXPR_MIR,
        CALLEXPR_MIR,
        MEMACCEXPR_MIR,
        SUBSCREXPR_MIR,
        PFIXEXPR_MIR,
        SIZEEXPR_MIR,
    };

    MIRNode(MIRNodeKind kind) : loc() {}

    MIRNode(Location loc, MIRNodeKind kind) : loc(loc) {}
    virtual ~MIRNode() = default;

    MIRNodeKind kind;
    Location loc;

    virtual void accept(MIRVisitor& visitor) = 0;
};

class ProgItemMIR : public MIRNode {
public:
    ProgItemMIR(Location loc, MIRNodeKind kind) : MIRNode(loc, kind) {}

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

class DeclMIR : public ProgItemMIR {
public:
    DeclMIR(Location loc, MIRNodeKind kind) : ProgItemMIR(loc, kind) {}
    virtual void accept(MIRVisitor& visitor) = 0;
};

class StmtMIR : public ProgItemMIR {
public:
    StmtMIR(Location loc, MIRNodeKind kind) : ProgItemMIR(loc, kind) {}
    virtual void accept(MIRVisitor& visitor) = 0;
};

class InitializerMIR : public MIRNode {
public:
    InitializerMIR(Location loc, Box<ExprMIR> expr)
        : MIRNode(loc, MIRNodeKind::INIT_MIR), initializer(std::move(expr)) {}
    InitializerMIR(Location loc, Vec<Box<InitializerMIR>> initializers)
        : MIRNode(loc, MIRNodeKind::INIT_MIR), initializer(std::move(initializers)) {}
    
    std::variant<Box<ExprMIR>, Vec<Box<InitializerMIR>>> initializer;

    void accept(MIRVisitor& visitor) override;
};

class TypeDeclMIR : public DeclMIR {
public:
    TypeDeclMIR(Location loc, sema::sym::TypeSymbol *sym)
        : DeclMIR(loc, MIRNodeKind::TYPEDEC_MIR), sym(sym) {}
    
    sema::sym::TypeSymbol *sym;

    void accept(MIRVisitor& visitor) override;
};

// A MIR node containing a single variable declaration and optional initializer.
class VarDeclMIR : public DeclMIR {
public:
    struct VarDecl {
        sema::sym::VarSymbol *sym;
        std::optional<Box<InitializerMIR>> initializer;
    };

    VarDeclMIR(Location loc)
        : DeclMIR(loc, MIRNodeKind::VARDEC_MIR) {}
    
    Vec<VarDecl> decls;

    void add_decl(sema::sym::VarSymbol *sym);

    void add_decl(sema::sym::VarSymbol *sym, Box<InitializerMIR> init);

    void accept(MIRVisitor& visitor) override;
};

class CompoundStmtMIR : public StmtMIR {
public:
    CompoundStmtMIR(Location loc)
        : StmtMIR(loc, MIRNodeKind::CMPDSTMT_MIR), items() {}
    
    CompoundStmtMIR(Location loc, Vec<Box<ProgItemMIR>> items)
        : StmtMIR(loc, MIRNodeKind::CMPDSTMT_MIR), items(std::move(items)) {}
    
    Vec<Box<ProgItemMIR>> items;

    void add_item(Box<ProgItemMIR> item);

    void accept(MIRVisitor& visitor) override;
};

class ExprStmtMIR : public StmtMIR {
public:
    ExprStmtMIR(Location loc, Box<ExprMIR> expr)
        : StmtMIR(loc, MIRNodeKind::EXPRSTMT_MIR), expr(std::move(expr)) {}
    
    ExprStmtMIR(Location loc)
        : StmtMIR(loc, MIRNodeKind::EXPRSTMT_MIR) {}
    
    std::optional<Box<ExprMIR>> expr;

    bool is_empty() { return expr.has_value(); }

    void accept(MIRVisitor& visitor) override;
};

class SwitchStmtMIR : public StmtMIR {
public:
    SwitchStmtMIR(Location loc, Box<ExprMIR> condition, Box<StmtMIR> body)
        : StmtMIR(loc, MIRNodeKind::SWITCHSTMT_MIR), 
        condition(std::move(condition)), body(std::move(body)) {}
    
    Box<ExprMIR> condition;
    Box<StmtMIR> body;

    void accept(MIRVisitor& visitor) override;
};

class CaseStmtMIR : public StmtMIR {
public:
    CaseStmtMIR(Location loc, Box<ExprMIR> case_expr, Box<StmtMIR> stmt)
        : StmtMIR(loc, MIRNodeKind::CASESTMT_MIR), 
        case_expr(std::move(case_expr)),
        stmt(std::move(stmt)) {}

    Box<ExprMIR> case_expr;
    Box<StmtMIR> stmt;

    void accept(MIRVisitor& visitor) override;
};

class CaseRangeStmtMIR : public StmtMIR {
public:
    CaseRangeStmtMIR(Location loc, 
                     Box<ExprMIR> case_start, 
                     Box<ExprMIR> case_end,
                     Box<StmtMIR> stmt)
        : StmtMIR(loc, MIRNodeKind::CASESTMT_MIR), 
        case_start(std::move(case_start)), 
        case_end(std::move(case_end)),
        stmt(std::move(stmt)) {}

    Box<ExprMIR> case_start;
    Box<ExprMIR> case_end;
    Box<StmtMIR> stmt;

    void accept(MIRVisitor& visitor) override;
};

class DefaultStmtMIR : public StmtMIR {
public:
    DefaultStmtMIR(Location loc, Box<StmtMIR> stmt)
        : StmtMIR(loc, MIRNodeKind::DEFSTMT_MIR), stmt(std::move(stmt)) {}
    
    Box<StmtMIR> stmt;

    void accept(MIRVisitor& visitor) override;
};

class LabeledStmtMIR : public StmtMIR {
public:
    LabeledStmtMIR(Location loc, sema::sym::LabelSymbol *label, Box<StmtMIR> stmt)
        : StmtMIR(loc, MIRNodeKind::LABSTMT_MIR), label(label), stmt(std::move(stmt)) {}

    sema::sym::LabelSymbol *label;
    Box<StmtMIR> stmt;

    void accept(MIRVisitor& visitor) override;
};

class PrintStmtMIR : public StmtMIR {
public:
    PrintStmtMIR(Location loc, std::string format_string, Vec<Box<ExprMIR>> arguments)
        : StmtMIR(loc, MIRNodeKind::PRINTSTMT_MIR), 
        format_string(format_string), arguments(std::move(arguments)) {}
    
    std::string format_string;
    Vec<Box<ExprMIR>> arguments;

    void accept(MIRVisitor& visitor) override;
};

class IfStmtMIR : public StmtMIR {
public:
    IfStmtMIR(Location loc, 
              Box<ExprMIR> condition, Box<StmtMIR> then_branch, 
              Box<StmtMIR> else_branch)
        : StmtMIR(loc, MIRNodeKind::IFSTMT_MIR), 
        condition(std::move(condition)), 
        then_branch(std::move(then_branch)),
        else_branch(std::move(else_branch)) {}

    IfStmtMIR(Location loc, 
              Box<ExprMIR> condition, Box<StmtMIR> then_branch, 
              std::optional<Box<StmtMIR>> else_branch)
        : StmtMIR(loc, MIRNodeKind::IFSTMT_MIR), 
        condition(std::move(condition)), 
        then_branch(std::move(then_branch)),
        else_branch(std::move(else_branch)) {}

    IfStmtMIR(Location loc, 
              Box<ExprMIR> condition, Box<StmtMIR> then_branch)
        : StmtMIR(loc, MIRNodeKind::IFSTMT_MIR), 
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
    LoopStmtMIR(Location loc, Box<LabeledStmtMIR> body) 
        : StmtMIR(loc, MIRNodeKind::LOOPSTMT_MIR),
        body(std::move(body)) {}

    LoopStmtMIR(Location loc, 
                std::optional<Box<ProgItemMIR>> init,
                std::optional<Box<ExprMIR>> condition, 
                std::optional<Box<StmtMIR>> step,
                Box<LabeledStmtMIR> body,
                bool is_dowhile)
        : StmtMIR(loc, MIRNodeKind::LOOPSTMT_MIR),
        init(std::move(init)),
        condition(std::move(condition)),
        step(std::move(step)),
        body(std::move(body)),
        is_dowhile(is_dowhile) {}
    
    LoopStmtMIR(Location loc, 
                Box<ExprMIR> condition, 
                Box<LabeledStmtMIR> body,
                bool is_dowhile)
        : StmtMIR(loc, MIRNodeKind::LOOPSTMT_MIR),
        condition(std::move(condition)),
        body(std::move(body)),
        is_dowhile(is_dowhile) {}

    /*
    Initialization code at the start of the loop.
    Only for loops should be using this.
    ProgItem is used because the init code can be a variable declaration
    or an expression statement.
    */
    std::optional<Box<ProgItemMIR>> init;
    /*
    The condition needed for the loop to continue.
    */
    std::optional<Box<ExprMIR>> condition;
    /*
    The step condition for updating a sentinel value in the loop.
    Only needed for `for` loops.
    */
    std::optional<Box<StmtMIR>> step;
    // The actual body of the loop.
    Box<LabeledStmtMIR> body;
    bool is_dowhile;

    void accept(MIRVisitor& visitor) override;
};

class GotoStmtMIR : public StmtMIR {
public:
    GotoStmtMIR(Location loc, sema::sym::LabelSymbol *target)
        : StmtMIR(loc, MIRNodeKind::GOTOSTMT_MIR), target(target) {}
    
    sema::sym::LabelSymbol *target;

    void accept(MIRVisitor& visitor) override;
};

class BreakStmtMIR : public StmtMIR {
public:
    BreakStmtMIR(Location loc) : StmtMIR(loc, MIRNodeKind::BREAKSTMT_MIR) {}
    
    void accept(MIRVisitor& visitor) override;
};

class ReturnStmtMIR : public StmtMIR {
public:
    ReturnStmtMIR(Location loc)
        : StmtMIR(loc, MIRNodeKind::RETSTMT_MIR) {}

    ReturnStmtMIR(Location loc, Box<ExprMIR> ret_expr)
        : StmtMIR(loc, MIRNodeKind::RETSTMT_MIR), ret_expr(std::move(ret_expr)) {}
    
    std::optional<Box<ExprMIR>> ret_expr;

    void accept(MIRVisitor& visitor) override;
};

class BinaryExprMIR : public ExprMIR {
public:
    BinaryExprMIR(Location loc, 
                  Box<ExprMIR> left, Box<ExprMIR> right, 
                  tokens::BinaryOp op)
        : ExprMIR(loc, MIRNodeKind::BINEXPR_MIR), 
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
        : ExprMIR(loc, MIRNodeKind::UNEXPR_MIR), operand(std::move(operand)), op(op) {}
    
    Box<ExprMIR> operand;
    tokens::UnaryOp op;

    void accept(MIRVisitor& visitor) override;
};

class CastExprMIR : public ExprMIR {
public:
    CastExprMIR(Location loc, sema::types::Type *target, Box<ExprMIR> inner)
        : ExprMIR(loc, MIRNodeKind::CASTEXPR_MIR), 
        target(target), inner(std::move(inner)) {}
    
    sema::types::Type *target;
    Box<ExprMIR> inner;

    void accept(MIRVisitor& visitor) override;
};

class AssignExprMIR : public ExprMIR {
public:
    AssignExprMIR(Location loc, 
                  Box<ExprMIR> left, Box<ExprMIR> right, 
                  tokens::AssignOp op)
        : ExprMIR(loc, MIRNodeKind::ASSGNEXPR_MIR), 
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
        : ExprMIR(loc, MIRNodeKind::CONDEXPR_MIR), 
        condition(std::move(condition)),
        true_expr(std::move(true_expr)),
        false_expr(std::move(false_expr)) {}

    Box<ExprMIR> condition;
    Box<ExprMIR> true_expr;
    Box<ExprMIR> false_expr;

    void accept(MIRVisitor& visitor) override;
};

class IdentExprMIR : public ExprMIR {
public:
    IdentExprMIR(Location loc, sema::sym::PhysicalSymbol *ident)
        : ExprMIR(loc, MIRNodeKind::IDENTEXPR_MIR), ident(ident) {}

    sema::sym::PhysicalSymbol *ident;

    void accept(MIRVisitor& visitor) override;
};

class ConstExprMIR : public ExprMIR {
public:
    ConstExprMIR(Location loc, Box<ExprMIR> inner)
        : ExprMIR(loc, MIRNodeKind::CONSTEXPR_MIR), inner(std::move(inner)) {}
    
    Box<ExprMIR> inner;

    void accept(MIRVisitor& visitor) override;
};

class LiteralExprMIR : public ExprMIR {
public:
    LiteralExprMIR(Location loc, exec::Value value)
        : ExprMIR(loc, MIRNodeKind::LITEXPR_MIR), value(value) {}

    exec::Value value;

    void accept(MIRVisitor& visitor) override;
};

class StringExprMIR : public ExprMIR {
public:
    StringExprMIR(Location loc, std::string value)
        : ExprMIR(loc, MIRNodeKind::STREXPR_MIR), value(value) {}

    std::string value;

    void accept(MIRVisitor& visitor) override;
};

class CallExprMIR : public ExprMIR {
public:
    CallExprMIR(Location loc, Box<ExprMIR> callee, Vec<Box<ExprMIR>> args)
        : ExprMIR(loc, MIRNodeKind::CALLEXPR_MIR), 
        callee(std::move(callee)), 
        args(std::move(args)) {}

    Box<ExprMIR> callee;
    Vec<Box<ExprMIR>> args;

    void accept(MIRVisitor& visitor) override;
};

class MemberAccExprMIR : public ExprMIR {
public:
    MemberAccExprMIR(Location loc, 
                     Box<ExprMIR> object, std::string member, bool is_arrow)
        : ExprMIR(loc, MIRNodeKind::MEMACCEXPR_MIR), 
        object(std::move(object)), member(member), is_arrow(is_arrow) {}
    
    Box<ExprMIR> object;
    std::string member;
    bool is_arrow;

    void accept(MIRVisitor& visitor) override;
};

class SubscrExprMIR : public ExprMIR {
public:
    SubscrExprMIR(Location loc, Box<ExprMIR> array, Box<ExprMIR> index)
        : ExprMIR(loc, MIRNodeKind::SUBSCREXPR_MIR), 
        array(std::move(array)), index(std::move(index)) {}

    Box<ExprMIR> array;
    Box<ExprMIR> index;

    void accept(MIRVisitor& visitor) override;
};

class PostfixExprMIR : public ExprMIR {
public:
    PostfixExprMIR(Location loc, Box<ExprMIR> operand, tokens::PostfixOp op)
        : ExprMIR(loc, MIRNodeKind::PFIXEXPR_MIR), operand(std::move(operand)), op(op) {}

    Box<ExprMIR> operand;
    tokens::PostfixOp op;

    void accept(MIRVisitor& visitor) override;
};

class SizeofExprMIR : public ExprMIR {
public:
    using SizeofOperand = std::variant<Box<ExprMIR>, sema::types::Type *>;

    SizeofExprMIR(Location loc) : ExprMIR(loc, MIRNodeKind::SIZEEXPR_MIR) {}

    SizeofExprMIR(Location loc, sema::types::Type *target)
        : ExprMIR(loc, MIRNodeKind::SIZEEXPR_MIR), operand(target) {}

    SizeofExprMIR(Location loc, Box<ExprMIR> target)
        : ExprMIR(loc, MIRNodeKind::SIZEEXPR_MIR), operand(std::move(target)) {}
    
    SizeofOperand operand;

    void accept(MIRVisitor& visitor) override;
};

class FunctionMIR : public ProgItemMIR {
public:
    FunctionMIR(Location loc,
                sema::sym::FuncSymbol *sym,
                sema::sym::Scope *scope,
                Box<CompoundStmtMIR> body)
        : ProgItemMIR(loc, MIRNodeKind::FUNC_MIR),
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
    ProgramMIR() : MIRNode(MIRNodeKind::PROG_MIR), items() {}

    ProgramMIR(Vec<Box<ProgItemMIR>> items)
        : MIRNode(MIRNodeKind::PROG_MIR), items(std::move(items)) {}
    
    Vec<Box<ProgItemMIR>> items;

    void add_item(Box<ProgItemMIR> item);

    void accept(MIRVisitor& visitor) override;
};

}

#endif