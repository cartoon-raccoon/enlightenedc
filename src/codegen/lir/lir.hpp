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
    enum class LIRNodeKind {
        PROG_LIR,
        FUNC_LIR,
        VARDECL_LIR,
        GOTOSTMT_LIR,
        EXPRSTMT_LIR,
        SWITCHSTMT_LIR,
        LABELSTMT_LIR,

    };

    LIRNode(LIRNodeKind kind) : kind(kind), loc() {}
    LIRNode(Location loc, LIRNodeKind kind) : kind(kind), loc(loc) {}
    virtual ~LIRNode() = default;

    LIRNodeKind kind;
    // Location is now optional because a lot of nodes in LIR will be compiler generated
    // and so will not have an intrinsic source code location.
    std::optional<Location> loc;

    virtual void accept(LIRVisitor& visitor) = 0;
};

class ProgItemLIR : public LIRNode {
public:
    ProgItemLIR(LIRNodeKind kind) : LIRNode(kind) {}
    ProgItemLIR(Location loc, LIRNodeKind kind) : LIRNode(loc, kind) {}

    virtual void accept(LIRVisitor& visitor) = 0;
};

class DeclLIR : public ProgItemLIR {
public:
    DeclLIR(LIRNodeKind kind) : ProgItemLIR(kind) {}
    DeclLIR(Location loc, LIRNodeKind kind) : ProgItemLIR(loc, kind) {}

    virtual void accept(LIRVisitor& visitor) = 0;
};

class StmtLIR : public ProgItemLIR {
public:
    StmtLIR(LIRNodeKind kind) : ProgItemLIR(kind) {}
    StmtLIR(Location loc, LIRNodeKind kind) : ProgItemLIR(loc, kind) {}

    virtual void accept(LIRVisitor& visitor) = 0;
};

class ExprLIR : public LIRNode {
public:
    ExprLIR(LIRNodeKind kind) : LIRNode(kind) {}
    ExprLIR(Location loc, LIRNodeKind kind) : LIRNode(loc, kind) {}\

    virtual void accept(LIRVisitor& visitor) = 0;
};

class FunctionLIR : public ProgItemLIR {
public:

    std::string name;

    sema::types::Type *returnty;

    Vec<LIRVar *> locals;
    Vec<Box<StmtLIR>> body;

    void accept(LIRVisitor& visitor) override;
};

class VarDeclLIR : public DeclLIR {
public:
    VarDeclLIR(Location loc, LIRVar *var)
        : DeclLIR(loc, LIRNodeKind::VARDECL_LIR), var(var) {}
    VarDeclLIR(LIRVar *var)
        : DeclLIR(LIRNodeKind::VARDECL_LIR), var(var) {}
    
    LIRVar *var;

    void accept(LIRVisitor& visitor) override;
};

class GotoStmtLIR : public StmtLIR {
public:
    GotoStmtLIR(std::string mangled_target)
        : StmtLIR(LIRNodeKind::GOTOSTMT_LIR), mangled_target(mangled_target) {}
    
    GotoStmtLIR(Location loc, std::string mangled_target, std::string target)
        : StmtLIR(loc, LIRNodeKind::GOTOSTMT_LIR), 
        mangled_target(mangled_target), target(target) {}

    // The mangled target name.
    std::string mangled_target;
    // The original target name as defined in the source code.
    // Does not exist for compiler-generated targets.
    std::optional<std::string> target;

    void accept(LIRVisitor& visitor) override;
};

class ExprStmtLIR : public StmtLIR {
public:
    Box<ExprLIR> expr;

    void accept(LIRVisitor& visitor) override;
};

class SwitchStmtLIR : public StmtLIR {
public:

    void accept(LIRVisitor& visitor) override;
};

class LabelStmtLIR : public StmtLIR {
public:
    std::string mangled_label;
    std::string label;

    void accept(LIRVisitor& visitor) override;
};

class PrintStmtLIR : public StmtLIR {
public:
    std::string format_string;
    Vec<Box<ExprLIR>> args;

    void accept(LIRVisitor& visitor) override;
};

class ReturnStmtLIR : public StmtLIR {
public:
    std::optional<Box<ExprLIR>> ret_value;

    void accept(LIRVisitor& visitor) override;
};

class BinaryExprLIR : public ExprLIR {
public:
    Box<ExprLIR> left;
    Box<ExprLIR> right;
    tokens::BinaryOp op;

    void accept(LIRVisitor& visitor) override;
};

class UnaryExprLIR : public ExprLIR {
public:
    Box<ExprLIR> operand;
    tokens::UnaryOp op;

    void accept(LIRVisitor& visitor) override;
};

class CastExprLIR : public ExprLIR {
public:
    sema::types::Type *target;
    Box<ExprLIR> inner;

    void accept(LIRVisitor& visitor) override;
};

class AssignExprLIR : public ExprLIR {
public:
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
    bool is_arrow;

    void accept(LIRVisitor& visitor) override;
};

class SubscrExprLIR : public ExprLIR {
public:
    void accept(LIRVisitor& visitor) override;
};

class PostfixExprLIR : public ExprLIR {
public:

    void accept(LIRVisitor& visitor) override;
};

class ProgramLIR : public LIRNode {
public:
    ProgramLIR() : LIRNode(LIRNodeKind::PROG_LIR), functions(), progitems() {}
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