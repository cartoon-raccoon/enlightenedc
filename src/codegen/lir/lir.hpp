#ifndef ECC_LIR_H
#define ECC_LIR_H

#include <string>

#include "semantics/types.hpp"
#include "codegen/lir/symbols.hpp"
#include "frontend/tokens.hpp"
#include "eval/value.hpp"
#include "util.hpp"

using namespace ecc;
using namespace util;

namespace ecc::codegen::lir {

class LIRVisitor;

class LIRNode {
public:
    enum class NodeKind {
        PROG_LIR,
        FUNC_LIR,
        VARDECL_LIR,
        GOTOSTMT_LIR,
        RETSTMT_LIR,
        SWITCHSTMT_LIR,
        BREAKSTMT_LIR,
        CONTSTMT_LIR,
        EXPRSTMT_LIR,
        IFSTMT_LIR,
        LOOPSTMT_LIR,
        LABELSTMT_LIR,
        PRINTSTMT_LIR,
        BINEXPR_LIR,
        UNEXPR_LIR,
        CASTEXPR_LIR,
        ASSIGNEXPR_LIR,
        CONDEXPR_LIR,
        IDENTEXPR_LIR,
        LITEXPR_LIR,
        CALLEXPR_LIR,
        MEMACCEXPR_LIR,
        SUBSCREXPR_LIR,
        PFIXEXPR_LIR,
    };

    LIRNode(NodeKind kind) : kind(kind), loc() {}
    LIRNode(Location loc, NodeKind kind) : kind(kind), loc(loc) {}
    virtual ~LIRNode() = default;

    NodeKind kind;
    // Location is now optional because a lot of nodes in LIR will be compiler generated
    // and so will not have an intrinsic source code location.
    std::optional<Location> loc;

    virtual void accept(LIRVisitor& visitor) = 0;
};

class ProgItemLIR : public LIRNode {
public:
    ProgItemLIR(NodeKind kind) : LIRNode(kind) {}
    ProgItemLIR(Location loc, NodeKind kind) : LIRNode(loc, kind) {}

    virtual void accept(LIRVisitor& visitor) = 0;
};

class DeclLIR : public ProgItemLIR {
public:
    DeclLIR(NodeKind kind) : ProgItemLIR(kind) {}
    DeclLIR(Location loc, NodeKind kind) : ProgItemLIR(loc, kind) {}

    virtual void accept(LIRVisitor& visitor) = 0;
};

class StmtLIR : public ProgItemLIR {
public:
    StmtLIR(NodeKind kind) : ProgItemLIR(kind) {}
    StmtLIR(Location loc, NodeKind kind) : ProgItemLIR(loc, kind) {}

    virtual bool is_terminal() { return false; }

    virtual void accept(LIRVisitor& visitor) = 0;
};

/*
*/
class TerminalLIR : public StmtLIR {
public:
    TerminalLIR(NodeKind kind) : StmtLIR(kind) {}
    TerminalLIR(Location loc, NodeKind kind) : StmtLIR(loc, kind) {}

    virtual bool is_terminal() { return true; }

    virtual void accept(LIRVisitor& visitor) = 0;
};

class ExprLIR : public LIRNode {
public:
    ExprLIR(NodeKind kind, sema::types::Type *type) : 
        LIRNode(kind), type(type) {}
    ExprLIR(Location loc, NodeKind kind, sema::types::Type *type) : 
        LIRNode(loc, kind), type(type) {}

    sema::types::Type *type;

    virtual void accept(LIRVisitor& visitor) = 0;
};

class FunctionLIR : public ProgItemLIR {
public:
    FunctionLIR(Location loc, 
                std::string mangled, 
                std::string name, 
                sema::types::Type *returnty)
        : ProgItemLIR(loc, NodeKind::FUNC_LIR), locals(), body() {}
    
    std::string mangled_name;
    std::string name;

    sema::types::Type *returnty;

    Vec<LIRVar *> locals;
    Vec<Box<ProgItemLIR>> body;

    void accept(LIRVisitor& visitor) override;
};

class VarDeclLIR : public DeclLIR {
public:
    VarDeclLIR(Location loc, LIRVar *var)
        : DeclLIR(loc, NodeKind::VARDECL_LIR), var(var) {}
    VarDeclLIR(LIRVar *var)
        : DeclLIR(NodeKind::VARDECL_LIR), var(var) {}
    
    LIRVar *var;

    void accept(LIRVisitor& visitor) override;
};

class GotoStmtLIR : public TerminalLIR {
public:
    GotoStmtLIR(std::string mangled_target)
        : TerminalLIR(NodeKind::GOTOSTMT_LIR), mangled_target(mangled_target) {}
    
    GotoStmtLIR(Location loc, std::string mangled_target, std::string target)
        : TerminalLIR(loc, NodeKind::GOTOSTMT_LIR), 
        mangled_target(mangled_target), target(target) {}

    // The mangled target name.
    std::string mangled_target;
    // The original target name as defined in the source code.
    // Does not exist for compiler-generated targets.
    std::optional<std::string> target;

    bool is_terminal() override { return true; }

    void accept(LIRVisitor& visitor) override;
};

class ReturnStmtLIR : public TerminalLIR {
public:
    ReturnStmtLIR(Location loc, Box<ExprLIR> ret_value)
        : TerminalLIR(loc, NodeKind::RETSTMT_LIR), 
        ret_value(std::move(ret_value)) {}

    ReturnStmtLIR(Location loc)
        : TerminalLIR(loc, NodeKind::RETSTMT_LIR) {}

    std::optional<Box<ExprLIR>> ret_value;

    bool is_terminal() override { return true; }

    void accept(LIRVisitor& visitor) override;
};

class SwitchStmtLIR : public TerminalLIR {
public:
    SwitchStmtLIR(Location loc, Box<ExprLIR> condition)
        : TerminalLIR(loc, NodeKind::SWITCHSTMT_LIR),
        condition(std::move(condition)) {}
    
    Box<ExprLIR> condition;
    Vec<Box<ProgItemLIR>> body;

    bool is_terminal() override { return true; }

    void accept(LIRVisitor& visitor) override;
};

class BreakStmtLIR : public TerminalLIR {
public:
    BreakStmtLIR(Location loc) : TerminalLIR(loc, NodeKind::BREAKSTMT_LIR) {}

    void accept(LIRVisitor& visitor) override;
};

class ContStmtLIR : public TerminalLIR {
public:
    ContStmtLIR(Location loc) : TerminalLIR(loc, NodeKind::CONTSTMT_LIR) {}

    void accept(LIRVisitor& visitor) override;
};

class ExprStmtLIR : public StmtLIR {
public:
    ExprStmtLIR(Location loc, Box<ExprLIR> expr)
        : StmtLIR(loc, NodeKind::EXPRSTMT_LIR), expr(std::move(expr)) {}
    
    Box<ExprLIR> expr;

    void accept(LIRVisitor& visitor) override;
};

class LabelStmtLIR : public ProgItemLIR {
public:
    std::string mangled_label;
    std::string label;

    Vec<Box<ProgItemLIR>> body;

    void accept(LIRVisitor& visitor) override;
};


class IfStmtLIR : public StmtLIR {
public:
    Box<ExprLIR> condition;
    Vec<Box<ProgItemLIR>> then_br;
    std::optional<Vec<Box<ProgItemLIR>>> else_br;

    void accept(LIRVisitor& visitor) override;
};

class LoopStmtLIR : public StmtLIR {
public:
    void accept(LIRVisitor& visitor) override;
};

class PrintStmtLIR : public StmtLIR {
public:
    std::string format_string;
    Vec<Box<ExprLIR>> args;

    void accept(LIRVisitor& visitor) override;
};

class BinaryExprLIR : public ExprLIR {
public:
    BinaryExprLIR(Location loc,
                  sema::types::Type *type,
                  Box<ExprLIR> left, 
                  Box<ExprLIR> right, 
                  tokens::BinaryOp op)
        : ExprLIR(loc, NodeKind::BINEXPR_LIR, type),
        left(std::move(left)), right(std::move(right)), op(op) {}

    Box<ExprLIR> left;
    Box<ExprLIR> right;
    tokens::BinaryOp op;

    void accept(LIRVisitor& visitor) override;
};

class UnaryExprLIR : public ExprLIR {
public:
    UnaryExprLIR(Location loc,
                 sema::types::Type *type, 
                 Box<ExprLIR> operand, 
                 tokens::UnaryOp op)
        : ExprLIR(loc, NodeKind::UNEXPR_LIR, type), 
        operand(std::move(operand)), op(op) {}
    
    Box<ExprLIR> operand;
    tokens::UnaryOp op;

    void accept(LIRVisitor& visitor) override;
};

class CastExprLIR : public ExprLIR {
public:
    CastExprLIR(Location loc,
                sema::types::Type *type,
                Box<ExprLIR> inner, 
                sema::types::Type *target)
        : ExprLIR(loc, NodeKind::CASTEXPR_LIR, type), 
        target(target),
        inner(std::move(inner)) {}
    
    sema::types::Type *target;
    Box<ExprLIR> inner;

    void accept(LIRVisitor& visitor) override;
};

class AssignExprLIR : public ExprLIR {
public:
    AssignExprLIR(Location loc,
                  sema::types::Type *type,
                  Box<ExprLIR> left, Box<ExprLIR> right, 
                  tokens::AssignOp op)
        : ExprLIR(loc, NodeKind::ASSIGNEXPR_LIR, type), 
        left(std::move(left)), right(std::move(right)), op(op) {}
    
    Box<ExprLIR> left;
    Box<ExprLIR> right;
    tokens::AssignOp op;

    void accept(LIRVisitor& visitor) override;
};

class CondExprLIR : public ExprLIR {
public:
    Box<ExprLIR> condition;
    Box<ExprLIR> true_value;
    Box<ExprLIR> false_value;

    void accept(LIRVisitor& visitor) override;
};

class IdentExprLIR : public ExprLIR {
public:
    void accept(LIRVisitor& visitor) override;
};

class LiteralExprLIR : public ExprLIR {
public:
    exec::Value value;

    void accept(LIRVisitor& visitor) override;
};

class CallExprLIR : public ExprLIR {
public:
    Box<ExprLIR> callee;
    Vec<ExprLIR> args;

    void accept(LIRVisitor& visit);
};

class MemberAccExprLIR : public ExprLIR {
public:
    Box<ExprLIR> object;
    std::string member;

    void accept(LIRVisitor& visitor) override;
};

class SubscrExprLIR : public ExprLIR {
public:
    Box<ExprLIR> array;
    Box<ExprLIR> index;
    void accept(LIRVisitor& visitor) override;
};

class PostfixExprLIR : public ExprLIR {
public:

    void accept(LIRVisitor& visitor) override;
};

class ProgramLIR : public LIRNode {
public:
    ProgramLIR() : LIRNode(NodeKind::PROG_LIR), functions(), progitems() {}
    Vec<Box<FunctionLIR>> functions;
    /*
    In HolyC, statements can be declared in the global scope, and they will be executed
    as if they were in something like main().

    As such, we have to separate functions and progitems, so that if a function declaration
    comes in between two statements, the function is not generated between the code for these
    two statements, which is obviously a bug.
    */
    Vec<Box<ProgItemLIR>> progitems;

    void accept(LIRVisitor& visitor) override;
};

}

#endif