#pragma once

#ifndef ECC_LIR_H
#define ECC_LIR_H

#include <cstddef>
#include <string>
#include <variant>

#include "abstract/visitor.hpp"
#include "eval/value.hpp"
#include "lowering/lir/symbols.hpp"
#include "lowering/lir/visitor.hpp"
#include "semantics/types.hpp"
#include "tokens.hpp"
#include "util.hpp"

using namespace ecc;
using namespace util;

namespace ecc::lower::lir {

class LabelLIR;
class StmtLIR;
class FunctionLIR;
class ProgItemLIRStream;

template <typename DerivedT, typename BaseT>
using LIRVisitable = Visitable<DerivedT, BaseT, LIRVisitor>;

class LIRNode : public NoCopy {
public:
    enum class NodeKind : uint8_t {
        PROG_LIR,
        FUNC_LIR,

        VARDECL_LIR,
        LABDECL_LIR,

        SCLINIT_LIR,
        AGGINIT_LIR,
        STRINIT_LIR,
        FUNCINIT_LIR,
        ZEROINIT_LIR,

        GOTOSTMT_LIR,
        RETSTMT_LIR,
        SWITCHSTMT_LIR,
        BREAKSTMT_LIR,
        CONTSTMT_LIR,
        CASEDECL_LIR,
        DEFDECL_LIR,
        EXPRSTMT_LIR,
        MEMCPY_LIR,
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
        ZEROEXPR_LIR,
        CALLEXPR_LIR,
        MEMACCEXPR_LIR,
        REINTEXPR_LIR,
        SUBSCREXPR_LIR,
        PFIXEXPR_LIR,
    };

    LIRNode(NodeKind kind) : kind(kind) {}

    LIRNode(Location loc, NodeKind kind) : kind(kind), loc(loc) {}

    LIRNode(Optional<Location> loc, NodeKind kind) : kind(kind), loc(loc) {}

    virtual ~LIRNode() = default;

    NodeKind kind;
    // Location is now optional because a lot of nodes in LIR will be compiler generated
    // and so will not have an intrinsic source code location.
    Optional<Location> loc;

    virtual void accept(LIRVisitor& visitor) = 0;
};

/**
A constant initializer for a global, indicating that code to initialize the corresponding
variable should not be generated, and instead the value should be baked into the final
executable.
*/
class ConstInitLIR : public LIRNode {
public:
    ConstInitLIR(Location loc, NodeKind kind, sema::types::Type *type)
        : LIRNode(loc, kind), type(type) {}

    ConstInitLIR(NodeKind kind, sema::types::Type *type) : LIRNode(kind), type(type) {}

    sema::types::Type *type;

    static bool classof(const LIRNode *node) {
        switch (node->kind) {
        case NodeKind::SCLINIT_LIR:
        case NodeKind::AGGINIT_LIR:
        case NodeKind::STRINIT_LIR:
        case NodeKind::FUNCINIT_LIR:
        case NodeKind::ZEROINIT_LIR:
            return true;
        default:
            return false;
        }
    }
};

class ScalarInitLIR : public LIRVisitable<ScalarInitLIR, ConstInitLIR> {
public:
    ScalarInitLIR(Location loc, sema::types::PrimitiveType *type, eval::Value& val)
        : LIRVisitable<ScalarInitLIR, ConstInitLIR>(loc, NodeKind::SCLINIT_LIR, type), val(val) {}

    eval::Value val;

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::SCLINIT_LIR; }
};

/**
A const-initializer for an aggregate type (Array or Class).
*/
class AggregateInitLIR : public LIRVisitable<AggregateInitLIR, ConstInitLIR> {
public:
    AggregateInitLIR(Location loc, sema::types::Type *type)
        : LIRVisitable<AggregateInitLIR, ConstInitLIR>(loc, NodeKind::AGGINIT_LIR, type) {}

    Vec<Chunk<ConstInitLIR>> elements;

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::AGGINIT_LIR; }
};

class StringInitLIR : public LIRVisitable<StringInitLIR, ConstInitLIR> {
public:
    StringInitLIR(Location loc, sema::types::Type *type, std::string str)
        : LIRVisitable<StringInitLIR, ConstInitLIR>(loc, NodeKind::STRINIT_LIR, type),
          str(std::move(str)) {}

    std::string str;

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::STRINIT_LIR; }
};

class FuncInitLIR : public LIRVisitable<FuncInitLIR, ConstInitLIR> {
public:
    FuncInitLIR(Location loc, sema::types::FunctionType *sig, FunctionLIR *func)
        : LIRVisitable<FuncInitLIR, ConstInitLIR>(loc, NodeKind::FUNCINIT_LIR, sig), func(func) {}

    FunctionLIR *func;

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::FUNCINIT_LIR; }
};

class ZeroInitLIR : public LIRVisitable<ZeroInitLIR, ConstInitLIR> {
public:
    ZeroInitLIR(sema::types::Type *type)
        : LIRVisitable<ZeroInitLIR, ConstInitLIR>(NodeKind::ZEROINIT_LIR, type) {}

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::ZEROINIT_LIR; }
};

class VarDeclLIR : public LIRVisitable<VarDeclLIR, LIRNode> {
public:
    VarDeclLIR(Location loc, LIRVarSym *var)
        : LIRVisitable<VarDeclLIR, LIRNode>(loc, NodeKind::VARDECL_LIR), lirsym(var) {}

    VarDeclLIR(LIRVarSym *var)
        : LIRVisitable<VarDeclLIR, LIRNode>(NodeKind::VARDECL_LIR), lirsym(var) {}

    LIRVarSym *lirsym;
    Chunk<ConstInitLIR> init = nullptr;

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::VARDECL_LIR; }
};

class ProgItemLIR : public LIRNode {
public:
    ProgItemLIR(NodeKind kind) : LIRNode(kind) {}
    ProgItemLIR(Location loc, NodeKind kind) : LIRNode(loc, kind) {}

    virtual LabelLIR *as_decl() { return nullptr; }
    virtual StmtLIR *as_stmt() { return nullptr; }

    // virtual Chunk<ProgItemLIRStream> progitem_stream();

    // NOTE: LabelLIR and StmtLIR are the only two subclasses of ProgItemLIR; VarDeclLIR and
    // FunctionLIR are siblings that derive LIRNode directly, not ProgItemLIR.
    static bool classof(const LIRNode *node) {
        switch (node->kind) {
        case NodeKind::CASEDECL_LIR:
        case NodeKind::DEFDECL_LIR:
        case NodeKind::LABDECL_LIR:
        case NodeKind::GOTOSTMT_LIR:
        case NodeKind::RETSTMT_LIR:
        case NodeKind::SWITCHSTMT_LIR:
        case NodeKind::BREAKSTMT_LIR:
        case NodeKind::CONTSTMT_LIR:
        case NodeKind::EXPRSTMT_LIR:
        case NodeKind::IFSTMT_LIR:
        case NodeKind::LOOPSTMT_LIR:
        case NodeKind::PRINTSTMT_LIR:
            return true;
        default:
            return false;
        }
    }
};

class LabelLIR : public ProgItemLIR {
public:
    LabelLIR(NodeKind kind) : ProgItemLIR(kind) {}
    LabelLIR(Location loc, NodeKind kind) : ProgItemLIR(loc, kind) {}

    LabelLIR *as_decl() override { return this; }

    static bool classof(const LIRNode *node) {
        switch (node->kind) {
        case NodeKind::CASEDECL_LIR:
        case NodeKind::DEFDECL_LIR:
        case NodeKind::LABDECL_LIR:
            return true;
        default:
            return false;
        }
    }
};

class StmtLIR : public ProgItemLIR {
public:
    enum class StmtKind : uint8_t {
        // A terminal statement ends a
        TERMINAL,
        NONTERMINAL,
    };

    StmtLIR(NodeKind kind, StmtKind stkind) : ProgItemLIR(kind), stkind(stkind) {}
    StmtLIR(Location loc, NodeKind kind, StmtKind stkind)
        : ProgItemLIR(loc, kind), stkind(stkind) {}

    StmtKind stkind;

    StmtLIR *as_stmt() override { return this; }

    virtual bool is_terminal() { return stkind == StmtKind::TERMINAL; }

    static bool classof(const LIRNode *node) {
        switch (node->kind) {
        case NodeKind::GOTOSTMT_LIR:
        case NodeKind::RETSTMT_LIR:
        case NodeKind::SWITCHSTMT_LIR:
        case NodeKind::BREAKSTMT_LIR:
        case NodeKind::CONTSTMT_LIR:
        case NodeKind::EXPRSTMT_LIR:
        case NodeKind::IFSTMT_LIR:
        case NodeKind::LOOPSTMT_LIR:
        case NodeKind::PRINTSTMT_LIR:
            return true;
        default:
            return false;
        }
    }
};

/*
A LIR node that terminates a CFG block. Such nodes include If, Continue, Switch, etc.
 */
class TerminalLIR : public StmtLIR {
public:
    TerminalLIR(NodeKind kind) : StmtLIR(kind, StmtKind::TERMINAL) {}
    TerminalLIR(Location loc, NodeKind kind) : StmtLIR(loc, kind, StmtKind::TERMINAL) {}

    bool is_terminal() override { return true; }

    // Distinguished from NonTerminalLIR by StmtKind rather than by NodeKind, since several
    // NodeKind values (e.g. IFSTMT_LIR, LOOPSTMT_LIR) are always terminal and others (e.g.
    // EXPRSTMT_LIR) are always nonterminal -- stkind is the authoritative discriminator here.
    static bool classof(const LIRNode *node) {
        if (!StmtLIR::classof(node)) {
            return false;
        }
        const auto *stmt = static_cast<const StmtLIR *>(node); // NOLINT(*-static-cast-downcast)
        return stmt->stkind == StmtKind::TERMINAL;
    }
};

/*
A LIR node that does not terminate a CFG block.
*/
class NonTerminalLIR : public StmtLIR {
public:
    NonTerminalLIR(NodeKind kind) : StmtLIR(kind, StmtKind::NONTERMINAL) {}
    NonTerminalLIR(Location loc, NodeKind kind) : StmtLIR(loc, kind, StmtKind::NONTERMINAL) {}

    bool is_terminal() override { return false; }

    static bool classof(const LIRNode *node) {
        if (!StmtLIR::classof(node)) {
            return false;
        }
        const auto *stmt = static_cast<const StmtLIR *>(node); // NOLINT(*-static-cast-downcast)
        return stmt->stkind == StmtKind::NONTERMINAL;
    }
};

class ExprLIR : public LIRNode {
public:
    ExprLIR(NodeKind kind, sema::types::Type *type)
        : LIRNode(kind), act_type(type), eff_type(type->effective_type()) {}

    ExprLIR(Location loc, NodeKind kind, sema::types::Type *type)
        : LIRNode(loc, kind), act_type(type), eff_type(type->effective_type()) {}

    ExprLIR(Optional<Location> loc, NodeKind kind, sema::types::Type *type)
        : LIRNode(loc, kind), act_type(type), eff_type(type->effective_type()) {}

    sema::types::Type *act_type;
    sema::types::Type *eff_type;

    void set_type(sema::types::Type *type) {
        act_type = type;
        eff_type = type->effective_type();
    }

    /**
    Create a boxed clone of `this`.
    */
    virtual Chunk<ExprLIR> clone_chunk() = 0;

    static bool classof(const LIRNode *node) {
        switch (node->kind) {
        case NodeKind::BINEXPR_LIR:
        case NodeKind::UNEXPR_LIR:
        case NodeKind::CASTEXPR_LIR:
        case NodeKind::ASSIGNEXPR_LIR:
        case NodeKind::CONDEXPR_LIR:
        case NodeKind::IDENTEXPR_LIR:
        case NodeKind::LITEXPR_LIR:
        case NodeKind::ZEROEXPR_LIR:
        case NodeKind::CALLEXPR_LIR:
        case NodeKind::MEMACCEXPR_LIR:
        case NodeKind::REINTEXPR_LIR:
        case NodeKind::SUBSCREXPR_LIR:
        case NodeKind::PFIXEXPR_LIR:
            return true;
        default:
            return false;
        }
    }
};

class FunctionLIR : public LIRVisitable<FunctionLIR, LIRNode> {
public:
    FunctionLIR(Location loc, LIRFuncSym *func)
        : LIRVisitable<FunctionLIR, LIRNode>(loc, NodeKind::FUNC_LIR), funcsym(func) {}

    LIRFuncSym *funcsym;
    bool has_definition = false;

    Vec<Chunk<VarDeclLIR>> locals;
    Vec<Chunk<ProgItemLIR>> body;

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::FUNC_LIR; }
};

class GotoStmtLIR : public LIRVisitable<GotoStmtLIR, TerminalLIR> {
public:
    GotoStmtLIR(std::string mangled_target)
        : LIRVisitable<GotoStmtLIR, TerminalLIR>(NodeKind::GOTOSTMT_LIR),
          mangled_target(std::move(mangled_target)) {}

    GotoStmtLIR(Location loc, std::string mangled_target, std::string target)
        : LIRVisitable<GotoStmtLIR, TerminalLIR>(loc, NodeKind::GOTOSTMT_LIR),
          mangled_target(std::move(mangled_target)), target(target) {}

    // The mangled target name.
    std::string mangled_target;
    // The original target name as defined in the source code.
    // Does not exist for compiler-generated targets.
    Optional<std::string> target;

    bool is_terminal() override { return true; }

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::GOTOSTMT_LIR; }
};

class ReturnStmtLIR : public LIRVisitable<ReturnStmtLIR, TerminalLIR> {
public:
    ReturnStmtLIR(Location loc, Chunk<ExprLIR> ret_value)
        : LIRVisitable<ReturnStmtLIR, TerminalLIR>(loc, NodeKind::RETSTMT_LIR),
          ret_value(std::move(ret_value)) {}

    ReturnStmtLIR(Location loc)
        : LIRVisitable<ReturnStmtLIR, TerminalLIR>(loc, NodeKind::RETSTMT_LIR) {}

    ReturnStmtLIR() : LIRVisitable<ReturnStmtLIR, TerminalLIR>(NodeKind::RETSTMT_LIR) {}

    Optional<Chunk<ExprLIR>> ret_value;

    bool is_terminal() override { return true; }

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::RETSTMT_LIR; }
};

class SwitchStmtLIR : public LIRVisitable<SwitchStmtLIR, TerminalLIR> {
public:
    SwitchStmtLIR(Location loc, Chunk<ExprLIR> condition)
        : LIRVisitable<SwitchStmtLIR, TerminalLIR>(loc, NodeKind::SWITCHSTMT_LIR),
          condition(std::move(condition)) {}

    Chunk<ExprLIR> condition;
    Vec<Chunk<ProgItemLIR>> body;

    bool is_terminal() override { return true; }

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::SWITCHSTMT_LIR; }
};

class CaseLIR : public LIRVisitable<CaseLIR, LabelLIR> {
public:
    CaseLIR(Location loc, eval::Value& case_value)
        : LIRVisitable<CaseLIR, LabelLIR>(loc, NodeKind::CASEDECL_LIR), case_value(case_value) {}

    eval::Value case_value;

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::CASEDECL_LIR; }
};

class DefaultLIR : public LIRVisitable<DefaultLIR, LabelLIR> {
public:
    DefaultLIR(Location loc) : LIRVisitable<DefaultLIR, LabelLIR>(loc, NodeKind::DEFDECL_LIR) {}

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::DEFDECL_LIR; }
};

class LabelDeclLIR : public LIRVisitable<LabelDeclLIR, LabelLIR> {
public:
    LabelDeclLIR(Location loc, std::string mangled_label, std::string label)
        : LIRVisitable<LabelDeclLIR, LabelLIR>(loc, NodeKind::LABDECL_LIR),
          mangled_label(std::move(mangled_label)), label(std::move(label)) {}

    std::string mangled_label;
    std::string label;

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::LABDECL_LIR; }
};

class BreakStmtLIR : public LIRVisitable<BreakStmtLIR, TerminalLIR> {
public:
    BreakStmtLIR(Location loc)
        : LIRVisitable<BreakStmtLIR, TerminalLIR>(loc, NodeKind::BREAKSTMT_LIR) {}

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::BREAKSTMT_LIR; }
};

class ContStmtLIR : public LIRVisitable<ContStmtLIR, TerminalLIR> {
public:
    ContStmtLIR(Location loc)
        : LIRVisitable<ContStmtLIR, TerminalLIR>(loc, NodeKind::CONTSTMT_LIR) {}

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::CONTSTMT_LIR; }
};

class ExprStmtLIR : public LIRVisitable<ExprStmtLIR, NonTerminalLIR> {
public:
    ExprStmtLIR(Location loc, Chunk<ExprLIR> expr)
        : LIRVisitable<ExprStmtLIR, NonTerminalLIR>(loc, NodeKind::EXPRSTMT_LIR),
          expr(std::move(expr)) {}

    Chunk<ExprLIR> expr;

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::EXPRSTMT_LIR; }
};

class MemcpyLIR : public LIRVisitable<MemcpyLIR, NonTerminalLIR> {
public:
    MemcpyLIR(Location loc, Chunk<ExprLIR> to, Chunk<ExprLIR> from, size_t n)
        : LIRVisitable<MemcpyLIR, NonTerminalLIR>(loc, NodeKind::MEMCPY_LIR), to(std::move(to)),
          from(std::move(from)), n(n) {}

    MemcpyLIR(Chunk<ExprLIR> to, Chunk<ExprLIR> from, size_t n)
        : LIRVisitable<MemcpyLIR, NonTerminalLIR>(NodeKind::MEMCPY_LIR), to(std::move(to)),
          from(std::move(from)), n(n) {}

    Chunk<ExprLIR> to, from;
    size_t n;

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::MEMCPY_LIR; }
};

class MemmoveLIR {};

class MemsetLIR {};

class IfStmtLIR : public LIRVisitable<IfStmtLIR, TerminalLIR> {
public:
    IfStmtLIR(Location loc, Chunk<ExprLIR> condition)
        : LIRVisitable<IfStmtLIR, TerminalLIR>(loc, NodeKind::IFSTMT_LIR),
          condition(std::move(condition)) {}

    Chunk<ExprLIR> condition;
    Vec<Chunk<ProgItemLIR>> then_br;
    Optional<Vec<Chunk<ProgItemLIR>>> else_br;

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::IFSTMT_LIR; }
};

class LoopStmtLIR : public LIRVisitable<LoopStmtLIR, TerminalLIR> {
public:
    LoopStmtLIR(Location loc)
        : LIRVisitable<LoopStmtLIR, TerminalLIR>(loc, NodeKind::LOOPSTMT_LIR) {}

    Optional<Vec<Chunk<ProgItemLIR>>> init;

    Optional<Chunk<ExprLIR>> condition;

    Optional<Vec<Chunk<ProgItemLIR>>> step;

    Vec<Chunk<ProgItemLIR>> body;

    bool is_dowhile = false;

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::LOOPSTMT_LIR; }
};

class PrintStmtLIR : public LIRVisitable<PrintStmtLIR, NonTerminalLIR> {
public:
    PrintStmtLIR(Location loc, std::string format_string, Vec<Chunk<ExprLIR>> args)
        : LIRVisitable<PrintStmtLIR, NonTerminalLIR>(loc, NodeKind::PRINTSTMT_LIR),
          format_string(std::move(format_string)), args(std::move(args)) {}

    std::string format_string;
    Vec<Chunk<ExprLIR>> args;

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::PRINTSTMT_LIR; }
};

class BinaryExprLIR : public LIRVisitable<BinaryExprLIR, ExprLIR> {
public:
    BinaryExprLIR(
        Location loc, sema::types::Type *type, Chunk<ExprLIR> left, Chunk<ExprLIR> right,
        tokens::BinaryOp op)
        : LIRVisitable<BinaryExprLIR, ExprLIR>(loc, NodeKind::BINEXPR_LIR, type),
          left(std::move(left)), right(std::move(right)), op(op) {}

    Chunk<ExprLIR> left;
    Chunk<ExprLIR> right;
    tokens::BinaryOp op;

    Chunk<ExprLIR> clone_chunk() override;

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::BINEXPR_LIR; }
};

class UnaryExprLIR : public LIRVisitable<UnaryExprLIR, ExprLIR> {
public:
    UnaryExprLIR(Location loc, sema::types::Type *type, Chunk<ExprLIR> operand, tokens::UnaryOp op)
        : LIRVisitable<UnaryExprLIR, ExprLIR>(loc, NodeKind::UNEXPR_LIR, type),
          operand(std::move(operand)), op(op) {}

    Chunk<ExprLIR> operand;
    tokens::UnaryOp op;

    Chunk<ExprLIR> clone_chunk() override;

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::UNEXPR_LIR; }
};

class CastExprLIR : public LIRVisitable<CastExprLIR, ExprLIR> {
public:
    enum class CastKind : uint8_t {
        Implicit,
        Explicit,
        ArrPtrDecay,
        FuncPtrDecay,
    };

    CastExprLIR(
        Location loc, sema::types::Type *type, Chunk<ExprLIR> inner, sema::types::Type *target,
        CastKind ck)
        : LIRVisitable<CastExprLIR, ExprLIR>(loc, NodeKind::CASTEXPR_LIR, type), castkind(ck),
          target(target), inner(std::move(inner)) {}

    CastKind castkind;
    sema::types::Type *target;
    Chunk<ExprLIR> inner;

    Chunk<ExprLIR> clone_chunk() override;

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::CASTEXPR_LIR; }
};

class AssignExprLIR : public LIRVisitable<AssignExprLIR, ExprLIR> {
public:
    AssignExprLIR(
        Location loc, sema::types::Type *type, Chunk<ExprLIR> left, Chunk<ExprLIR> right,
        tokens::AssignOp op)
        : LIRVisitable<AssignExprLIR, ExprLIR>(loc, NodeKind::ASSIGNEXPR_LIR, type),
          left(std::move(left)), right(std::move(right)), op(op) {}

    Chunk<ExprLIR> left;
    Chunk<ExprLIR> right;
    tokens::AssignOp op;

    Chunk<ExprLIR> clone_chunk() override;

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::ASSIGNEXPR_LIR; }
};

class CondExprLIR : public LIRVisitable<CondExprLIR, ExprLIR> {
public:
    CondExprLIR(
        Location loc, sema::types::Type *type, Chunk<ExprLIR> cond, Chunk<ExprLIR> true_val,
        Chunk<ExprLIR> false_val)
        : LIRVisitable<CondExprLIR, ExprLIR>(loc, NodeKind::CONDEXPR_LIR, type),
          condition(std::move(cond)), true_value(std::move(true_val)),
          false_value(std::move(false_val)) {}

    Chunk<ExprLIR> condition;
    Chunk<ExprLIR> true_value;
    Chunk<ExprLIR> false_value;

    Chunk<ExprLIR> clone_chunk() override;

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::CONDEXPR_LIR; }
};

class IdentExprLIR : public LIRVisitable<IdentExprLIR, ExprLIR> {
public:
    IdentExprLIR(Location loc, LIRSym *sym, sema::types::Type *type)
        : LIRVisitable<IdentExprLIR, ExprLIR>(loc, NodeKind::IDENTEXPR_LIR, type), sym(sym) {}

    IdentExprLIR(LIRSym *sym, sema::types::Type *type)
        : LIRVisitable<IdentExprLIR, ExprLIR>(NodeKind::IDENTEXPR_LIR, type), sym(sym) {}

    IdentExprLIR(Optional<Location> loc, LIRSym *sym, sema::types::Type *type)
        : LIRVisitable<IdentExprLIR, ExprLIR>(loc, NodeKind::IDENTEXPR_LIR, type), sym(sym) {}

    LIRSym *sym;

    Chunk<ExprLIR> clone_chunk() override;

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::IDENTEXPR_LIR; }
};

class LiteralExprLIR : public LIRVisitable<LiteralExprLIR, ExprLIR> {
public:
    using LitValueLIR = std::variant<eval::Value, std::string>;

    LiteralExprLIR(Location loc, eval::Value value, sema::types::Type *type)
        : LIRVisitable<LiteralExprLIR, ExprLIR>(loc, NodeKind::LITEXPR_LIR, type), value(value) {}

    LiteralExprLIR(eval::Value value, sema::types::Type *type)
        : LIRVisitable<LiteralExprLIR, ExprLIR>(NodeKind::LITEXPR_LIR, type), value(value) {}

    LiteralExprLIR(Location loc, std::string value, sema::types::Type *type)
        : LIRVisitable<LiteralExprLIR, ExprLIR>(loc, NodeKind::LITEXPR_LIR, type),
          value(std::move(value)) {}

    LiteralExprLIR(Location loc, LitValueLIR value, sema::types::Type *type)
        : LIRVisitable<LiteralExprLIR, ExprLIR>(loc, NodeKind::LITEXPR_LIR, type),
          value(std::move(value)) {}

    LiteralExprLIR(Optional<Location> loc, LitValueLIR value, sema::types::Type *type)
        : LIRVisitable<LiteralExprLIR, ExprLIR>(loc, NodeKind::LITEXPR_LIR, type),
          value(std::move(value)) {}

    LitValueLIR value;

    Chunk<ExprLIR> clone_chunk() override;

    bool is_str() const { return std::holds_alternative<std::string>(value); }
    bool is_val() const { return std::holds_alternative<eval::Value>(value); }

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::LITEXPR_LIR; }
};

/**
An expression to indicate a certain value needs to be zero-filled.

Used in initializers for fields or array indexes that were skipped.
*/
class ZeroExprLIR : public LIRVisitable<ZeroExprLIR, ExprLIR> {
public:
    ZeroExprLIR(Location loc, sema::types::Type *type)
        : LIRVisitable<ZeroExprLIR, ExprLIR>(loc, NodeKind::ZEROEXPR_LIR, type) {}

    Chunk<ExprLIR> clone_chunk() override;

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::ZEROEXPR_LIR; }
};

class CallExprLIR : public LIRVisitable<CallExprLIR, ExprLIR> {
public:
    CallExprLIR(Location loc, Chunk<ExprLIR> callee, Vec<Chunk<ExprLIR>> args, sema::types::Type *type)
        : LIRVisitable<CallExprLIR, ExprLIR>(loc, NodeKind::CALLEXPR_LIR, type),
          callee(std::move(callee)), args(std::move(args)) {}

    Chunk<ExprLIR> callee;
    Vec<Chunk<ExprLIR>> args;

    Chunk<ExprLIR> clone_chunk() override;

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::CALLEXPR_LIR; }
};

class MemberAccExprLIR : public LIRVisitable<MemberAccExprLIR, ExprLIR> {
public:
    MemberAccExprLIR(Location loc, Chunk<ExprLIR> object, size_t member_idx, sema::types::Type *type)
        : LIRVisitable<MemberAccExprLIR, ExprLIR>(loc, NodeKind::MEMACCEXPR_LIR, type),
          object(std::move(object)), member_idx(member_idx) {}

    MemberAccExprLIR(Chunk<ExprLIR> object, size_t member_idx, sema::types::Type *type)
        : LIRVisitable<MemberAccExprLIR, ExprLIR>(NodeKind::MEMACCEXPR_LIR, type),
          object(std::move(object)), member_idx(member_idx) {}

    MemberAccExprLIR(
        Optional<Location> loc, Chunk<ExprLIR> object, size_t member_idx, sema::types::Type *type)
        : LIRVisitable<MemberAccExprLIR, ExprLIR>(loc, NodeKind::MEMACCEXPR_LIR, type),
          object(std::move(object)), member_idx(member_idx) {}

    Chunk<ExprLIR> object;
    size_t member_idx;

    Chunk<ExprLIR> clone_chunk() override;

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::MEMACCEXPR_LIR; }
};

class ReintExprLIR : public LIRVisitable<ReintExprLIR, ExprLIR> {
public:
    ReintExprLIR(
        Location loc, Chunk<ExprLIR> object, tokens::PrimType target, sema::types::Type *type)
        : LIRVisitable<ReintExprLIR, ExprLIR>(loc, NodeKind::REINTEXPR_LIR, type),
          object(std::move(object)), target(target) {}

    ReintExprLIR(
        Optional<Location> loc, Chunk<ExprLIR> object, tokens::PrimType target,
        sema::types::Type *type)
        : LIRVisitable<ReintExprLIR, ExprLIR>(loc, NodeKind::REINTEXPR_LIR, type),
          object(std::move(object)), target(target) {}

    Chunk<ExprLIR> object;
    tokens::PrimType target;

    Chunk<ExprLIR> clone_chunk() override;

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::REINTEXPR_LIR; }
};

class SubscrExprLIR : public LIRVisitable<SubscrExprLIR, ExprLIR> {
public:
    SubscrExprLIR(Location loc, Chunk<ExprLIR> array, Chunk<ExprLIR> index, sema::types::Type *type)
        : LIRVisitable<SubscrExprLIR, ExprLIR>(loc, NodeKind::SUBSCREXPR_LIR, type),
          array(std::move(array)), index(std::move(index)) {}

    SubscrExprLIR(Chunk<ExprLIR> array, Chunk<ExprLIR> index, sema::types::Type *type)
        : LIRVisitable<SubscrExprLIR, ExprLIR>(NodeKind::SUBSCREXPR_LIR, type),
          array(std::move(array)), index(std::move(index)) {}

    SubscrExprLIR(
        Optional<Location> loc, Chunk<ExprLIR> array, Chunk<ExprLIR> index, sema::types::Type *type)
        : LIRVisitable<SubscrExprLIR, ExprLIR>(loc, NodeKind::SUBSCREXPR_LIR, type),
          array(std::move(array)), index(std::move(index)) {}

    Chunk<ExprLIR> array;
    Chunk<ExprLIR> index;

    Chunk<ExprLIR> clone_chunk() override;

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::SUBSCREXPR_LIR; }
};

class PostfixExprLIR : public LIRVisitable<PostfixExprLIR, ExprLIR> {
public:
    PostfixExprLIR(
        Location loc, Chunk<ExprLIR> operand, tokens::PostfixOp op, sema::types::Type *type)
        : LIRVisitable<PostfixExprLIR, ExprLIR>(loc, NodeKind::PFIXEXPR_LIR, type),
          operand(std::move(operand)), op(op) {}

    Chunk<ExprLIR> operand;
    tokens::PostfixOp op;

    Chunk<ExprLIR> clone_chunk() override;

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::PFIXEXPR_LIR; }
};

class ProgramLIR : public LIRVisitable<ProgramLIR, LIRNode> {
public:
    ProgramLIR() : LIRVisitable<ProgramLIR, LIRNode>(NodeKind::PROG_LIR) {}

    Vec<Chunk<FunctionLIR>> functions;
    /*
    In HolyC, statements can be declared in the global scope, and they will be executed
    as if they were in something like main().

    As such, we have to separate functions and progitems, so that if a function declaration
    comes in between two statements, the function is not generated between the code for these
    two statements, which is obviously a bug.

    Additionally, since we will be inserting an implicit main() but all variables declared
    outside a function are global, we also need to separate out declarations.
    */
    Vec<Chunk<VarDeclLIR>> globals;
    Vec<Chunk<ProgItemLIR>> progitems;

    static bool classof(const LIRNode *node) { return node->kind == NodeKind::PROG_LIR; }
};

} // namespace ecc::lower::lir

#endif
