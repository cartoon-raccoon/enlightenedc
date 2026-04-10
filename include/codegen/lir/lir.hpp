#pragma once

#ifndef ECC_LIR_H
#define ECC_LIR_H

#include <cstddef>
#include <string>

#include "codegen/lir/symbols.hpp"
#include "eval/value.hpp"
#include "semantics/types.hpp"
#include "tokens.hpp"
#include "util.hpp"

using namespace ecc;
using namespace util;

namespace ecc::codegen::lir {

class LIRVisitor;
class LabelLIR;
class StmtLIR;
class FunctionLIR;
class ProgItemLIRStream;

class LIRNode : public NoCopy {
public:
    enum class NodeKind : uint8_t {
        PROG_LIR,
        FUNC_LIR,
        VARDECL_LIR,
        LABDECL_LIR,
        GOTOSTMT_LIR,
        RETSTMT_LIR,
        SWITCHSTMT_LIR,
        BREAKSTMT_LIR,
        CONTSTMT_LIR,
        CASEDECL_LIR,
        DEFDECL_LIR,
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

    LIRNode(NodeKind kind) : kind(kind) {}
    LIRNode(Location loc, NodeKind kind) : kind(kind), loc(loc) {}
    virtual ~LIRNode() = default;

    NodeKind kind;
    // Location is now optional because a lot of nodes in LIR will be compiler generated
    // and so will not have an intrinsic source code location.
    Optional<Location> loc;

    virtual void accept(LIRVisitor& visitor) = 0;
};

class VarDeclLIR : public LIRNode {
public:
    VarDeclLIR(Location loc, LIRVarSym *var) : LIRNode(loc, NodeKind::VARDECL_LIR), lirsym(var) {}

    VarDeclLIR(LIRVarSym *var) : LIRNode(NodeKind::VARDECL_LIR), lirsym(var) {}

    LIRVarSym *lirsym;

    void accept(LIRVisitor& visitor) override;
};

class ProgItemLIR : public LIRNode {
public:
    ProgItemLIR(NodeKind kind) : LIRNode(kind) {}
    ProgItemLIR(Location loc, NodeKind kind) : LIRNode(loc, kind) {}

    virtual LabelLIR *as_decl() { return nullptr; }
    virtual StmtLIR *as_stmt() { return nullptr; }

    //virtual Box<ProgItemLIRStream> progitem_stream();
};

class LabelLIR : public ProgItemLIR {
public:
    LabelLIR(NodeKind kind) : ProgItemLIR(kind) {}
    LabelLIR(Location loc, NodeKind kind) : ProgItemLIR(loc, kind) {}

    LabelLIR *as_decl() override { return this; }
};

class StmtLIR : public ProgItemLIR {
public:
    enum class StmtKind : uint8_t {
        // A terminal statement ends a 
        TERMINAL,
        NONTERMINAL,
    };

    StmtLIR(NodeKind kind, StmtKind stkind)
        : ProgItemLIR(kind), stkind(stkind) {}
    StmtLIR(Location loc, NodeKind kind, StmtKind stkind)
        : ProgItemLIR(loc, kind), stkind(stkind) {}

    StmtKind stkind;

    StmtLIR *as_stmt() override { return this; }

    virtual bool is_terminal() { return stkind == StmtKind::TERMINAL; }
};

/*
A LIR node that terminates a CFG block. Such nodes include If, Continue, Switch, etc.
 */
class TerminalLIR : public StmtLIR {
public:
    TerminalLIR(NodeKind kind) : StmtLIR(kind, StmtKind::TERMINAL) {}
    TerminalLIR(Location loc, NodeKind kind) : StmtLIR(loc, kind, StmtKind::TERMINAL) {}

    bool is_terminal() override { return true; }
};

/*
A LIR node that does not terminate a CFG block.
*/
class NonTerminalLIR : public StmtLIR {
public:
    NonTerminalLIR(NodeKind kind) : StmtLIR(kind, StmtKind::NONTERMINAL) {}
    NonTerminalLIR(Location loc, NodeKind kind) : StmtLIR(loc, kind, StmtKind::NONTERMINAL) {}

    bool is_terminal() override { return false; }
};

class ExprLIR : public LIRNode {
public:
    ExprLIR(NodeKind kind, sema::types::Type *type) : LIRNode(kind), type(type) {}
    ExprLIR(Location loc, NodeKind kind, sema::types::Type *type)
        : LIRNode(loc, kind), type(type) {}

    sema::types::Type *type;
};

class FunctionLIR : public LIRNode {
public:
    FunctionLIR(Location loc, std::string mangled, std::string name, LIRFuncSym *func)
        : LIRNode(loc, NodeKind::FUNC_LIR), mangled_name(std::move(mangled)),
          name(std::move(name)), lirsym(func) {}

    std::string mangled_name;
    std::string name;

    LIRFuncSym *lirsym;

    Vec<Box<VarDeclLIR>> locals;
    Vec<Box<ProgItemLIR>> body;

    void accept(LIRVisitor& visitor) override;
};

class GotoStmtLIR : public TerminalLIR {
public:
    GotoStmtLIR(std::string mangled_target)
        : TerminalLIR(NodeKind::GOTOSTMT_LIR), mangled_target(std::move(mangled_target)) {}

    GotoStmtLIR(Location loc, std::string mangled_target, std::string target)
        : TerminalLIR(loc, NodeKind::GOTOSTMT_LIR), mangled_target(std::move(mangled_target)),
          target(target) {}

    // The mangled target name.
    std::string mangled_target;
    // The original target name as defined in the source code.
    // Does not exist for compiler-generated targets.
    Optional<std::string> target;

    bool is_terminal() override { return true; }

    void accept(LIRVisitor& visitor) override;
};

class ReturnStmtLIR : public TerminalLIR {
public:
    ReturnStmtLIR(Location loc, Box<ExprLIR> ret_value)
        : TerminalLIR(loc, NodeKind::RETSTMT_LIR), ret_value(std::move(ret_value)) {}

    ReturnStmtLIR(Location loc) : TerminalLIR(loc, NodeKind::RETSTMT_LIR) {}

    Optional<Box<ExprLIR>> ret_value;

    bool is_terminal() override { return true; }

    void accept(LIRVisitor& visitor) override;
};

class SwitchStmtLIR : public TerminalLIR {
public:
    SwitchStmtLIR(Location loc, Box<ExprLIR> condition)
        : TerminalLIR(loc, NodeKind::SWITCHSTMT_LIR), condition(std::move(condition)) {}

    Box<ExprLIR> condition;
    Vec<Box<ProgItemLIR>> body;

    bool is_terminal() override { return true; }

    void accept(LIRVisitor& visitor) override;
};

class CaseLIR : public LabelLIR {
public:
    CaseLIR(Location loc, eval::Value case_value)
        : LabelLIR(loc, NodeKind::CASEDECL_LIR), case_value(std::move(case_value)) {}

    eval::Value case_value;

    void accept(LIRVisitor& visitor) override;
};

class DefaultLIR : public LabelLIR {
public:
    DefaultLIR(Location loc) : LabelLIR(loc, NodeKind::DEFDECL_LIR) {}

    void accept(LIRVisitor& visitor) override;
};

class LabelDeclLIR : public LabelLIR {
public:
    LabelDeclLIR(Location loc, std::string mangled_label, std::string label)
        : LabelLIR(loc, NodeKind::LABDECL_LIR), mangled_label(std::move(mangled_label)),
          label(std::move(label)) {}

    std::string mangled_label;
    std::string label;

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

class ExprStmtLIR : public NonTerminalLIR {
public:
    ExprStmtLIR(Location loc, Box<ExprLIR> expr)
        : NonTerminalLIR(loc, NodeKind::EXPRSTMT_LIR), expr(std::move(expr)) {}

    Box<ExprLIR> expr;

    void accept(LIRVisitor& visitor) override;
};

class IfStmtLIR : public TerminalLIR {
public:
    IfStmtLIR(Location loc, Box<ExprLIR> condition) : TerminalLIR(loc, NodeKind::IFSTMT_LIR) {}

    Box<ExprLIR> condition;
    Vec<Box<ProgItemLIR>> then_br;
    Optional<Vec<Box<ProgItemLIR>>> else_br;

    void accept(LIRVisitor& visitor) override;
};

class LoopStmtLIR : public TerminalLIR {
public:
    LoopStmtLIR(Location loc) : TerminalLIR(loc, NodeKind::LOOPSTMT_LIR) {}

    Optional<Vec<Box<ProgItemLIR>>> init;

    Optional<Box<ExprLIR>> condition;

    Optional<Vec<Box<ProgItemLIR>>> step;

    Vec<Box<ProgItemLIR>> body;

    bool is_dowhile = false;

    void accept(LIRVisitor& visitor) override;
};

class PrintStmtLIR : public NonTerminalLIR {
public:
    PrintStmtLIR(Location loc, std::string format_string, Vec<Box<ExprLIR>> args)
        : NonTerminalLIR(loc, NodeKind::PRINTSTMT_LIR), format_string(std::move(format_string)),
          args(std::move(args)) {}

    std::string format_string;
    Vec<Box<ExprLIR>> args;

    void accept(LIRVisitor& visitor) override;
};

class BinaryExprLIR : public ExprLIR {
public:
    BinaryExprLIR(Location loc, sema::types::Type *type, Box<ExprLIR> left, Box<ExprLIR> right,
                  tokens::BinaryOp op)
        : ExprLIR(loc, NodeKind::BINEXPR_LIR, type), left(std::move(left)), right(std::move(right)),
          op(op) {}

    Box<ExprLIR> left;
    Box<ExprLIR> right;
    tokens::BinaryOp op;

    void accept(LIRVisitor& visitor) override;
};

class UnaryExprLIR : public ExprLIR {
public:
    UnaryExprLIR(Location loc, sema::types::Type *type, Box<ExprLIR> operand, tokens::UnaryOp op)
        : ExprLIR(loc, NodeKind::UNEXPR_LIR, type), operand(std::move(operand)), op(op) {}

    Box<ExprLIR> operand;
    tokens::UnaryOp op;

    void accept(LIRVisitor& visitor) override;
};

class CastExprLIR : public ExprLIR {
public:
    CastExprLIR(Location loc, sema::types::Type *type, Box<ExprLIR> inner,
                sema::types::Type *target)
        : ExprLIR(loc, NodeKind::CASTEXPR_LIR, type), target(target), inner(std::move(inner)) {}

    sema::types::Type *target;
    Box<ExprLIR> inner;

    void accept(LIRVisitor& visitor) override;
};

class AssignExprLIR : public ExprLIR {
public:
    AssignExprLIR(Location loc, sema::types::Type *type, Box<ExprLIR> left, Box<ExprLIR> right,
                  tokens::AssignOp op)
        : ExprLIR(loc, NodeKind::ASSIGNEXPR_LIR, type), left(std::move(left)),
          right(std::move(right)), op(op) {}

    Box<ExprLIR> left;
    Box<ExprLIR> right;
    tokens::AssignOp op;

    void accept(LIRVisitor& visitor) override;
};

class CondExprLIR : public ExprLIR {
public:
    CondExprLIR(Location loc, sema::types::Type *type, 
                Box<ExprLIR> cond, Box<ExprLIR> true_val, Box<ExprLIR> false_val)
        : ExprLIR(loc, NodeKind::CONDEXPR_LIR, type),
        condition(std::move(cond)),
        true_value(std::move(true_val)),
        false_value(std::move(false_val)) {}
    
    Box<ExprLIR> condition;
    Box<ExprLIR> true_value;
    Box<ExprLIR> false_value;

    void accept(LIRVisitor& visitor) override;
};

class IdentExprLIR : public ExprLIR {
public:
    IdentExprLIR(Location loc, LIRSym *sym, sema::types::Type *type)
        : ExprLIR(loc, NodeKind::IDENTEXPR_LIR, type), sym(sym) {}

    LIRSym *sym;

    void accept(LIRVisitor& visitor) override;
};

class LiteralExprLIR : public ExprLIR {
public:
    LiteralExprLIR(Location loc, eval::Value value, sema::types::Type *type)
        : ExprLIR(loc, NodeKind::LITEXPR_LIR, type), value(std::move(value)) {}

    eval::Value value;

    void accept(LIRVisitor& visitor) override;
};

class CallExprLIR : public ExprLIR {
public:
    Box<ExprLIR> callee;
    Vec<Box<ExprLIR>> args;

    void accept(LIRVisitor& visit) override;
};

class MemberAccExprLIR : public ExprLIR {
public:
    Box<ExprLIR> object;
    size_t member_idx;

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
    Box<ExprLIR> operand;
    tokens::PostfixOp op;

    void accept(LIRVisitor& visitor) override;
};

class ProgramLIR : public LIRNode {
public:
    ProgramLIR() : LIRNode(NodeKind::PROG_LIR) {}

    Vec<Box<FunctionLIR>> functions;
    /*
    In HolyC, statements can be declared in the global scope, and they will be executed
    as if they were in something like main().

    As such, we have to separate functions and progitems, so that if a function declaration
    comes in between two statements, the function is not generated between the code for these
    two statements, which is obviously a bug.

    Additionally, since we will be inserting an implicit main() but all variables declared
    outside a function are global, we also need to separate out declarations.
    */
    Vec<Box<VarDeclLIR>> globals;
    Vec<Box<ProgItemLIR>> progitems;

    void accept(LIRVisitor& visitor) override;
};

} // namespace ecc::codegen::lir

#endif