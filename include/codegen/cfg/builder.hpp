#pragma once

#ifndef ECC_CFG_BUILDER_H
#define ECC_CFG_BUILDER_H

#include "codegen/cfg/cfg.hpp"
#include "codegen/lir/lir.hpp"
#include "codegen/lir/visitor.hpp"
#include "util.hpp"

using namespace ecc;
using namespace util;

/*
CFG generation functionality.
*/
namespace ecc::codegen::cfg {

class IfStmtInfo;
class SwitchStmtInfo;
class LoopStmtInfo;
class Value;

class NestedStmtInfo {
public:
    enum class Kind : uint8_t {
        IF,
        SWITCH,
        LOOP,
    };

    NestedStmtInfo(Kind kind, BasicBlock *merge) : kind(kind), merge(merge) {}
    virtual ~NestedStmtInfo() = default;

    Kind kind;
    BasicBlock *merge;

    bool is_if() const { return kind == Kind::IF; }
    bool is_switch() const { return kind == Kind::SWITCH; }
    bool is_loop() const { return kind == Kind::LOOP; }

    virtual IfStmtInfo *as_if() { return nullptr; }
    virtual SwitchStmtInfo *as_switch() { return nullptr; }
    virtual LoopStmtInfo *as_loop() { return nullptr; }
};

class IfStmtInfo : public NestedStmtInfo {
public:
    IfStmtInfo(BasicBlock *merge) : NestedStmtInfo(Kind::IF, merge) {}

    IfStmtInfo *as_if() override { return this; }
};

class SwitchStmtInfo : public NestedStmtInfo {
public:
    SwitchStmtInfo(BasicBlock *merge) : NestedStmtInfo(Kind::SWITCH, merge) {}

    Vec<SwitchCase> cases;

    SwitchStmtInfo *as_switch() override { return this; }
};

class LoopStmtInfo : public NestedStmtInfo {
public:
    LoopStmtInfo(BasicBlock *merge) : NestedStmtInfo(Kind::LOOP, merge) {}

    BasicBlock *cond = nullptr;
    BasicBlock *body = nullptr;

    LoopStmtInfo *as_loop() override { return this; }
};

/**
Class that builds
*/
class CFGBuilder : public lir::LIRVisitor {
public:
    CFGBuilder(ProgramCFG& prog_cfg) : prog_cfg(prog_cfg) {}

    void build_cfg(lir::ProgramLIR& prog);

protected:
    FunctionCFG *current_func = nullptr;
    BasicBlock *current_block = nullptr;
    Value *last_value         = nullptr;

    using NestedStmtFilter = std::function<bool(NestedStmtInfo *)>;

    NestedStmtInfo *find_info(NestedStmtFilter& filter);

    void visit(lir::ProgramLIR& node) override;
    void visit(lir::FunctionLIR& node) override;

    void visit(lir::LabelDeclLIR& node) override;
    void visit(lir::CaseLIR& node) override;
    void visit(lir::DefaultLIR& node) override;
    void visit(lir::ExprStmtLIR& node) override;
    void visit(lir::GotoStmtLIR& node) override;
    void visit(lir::SwitchStmtLIR& node) override;
    void visit(lir::BreakStmtLIR& node) override;
    void visit(lir::ContStmtLIR& node) override;
    void visit(lir::IfStmtLIR& node) override;
    void visit(lir::LoopStmtLIR& node) override;
    void visit(lir::PrintStmtLIR& node) override;
    void visit(lir::ReturnStmtLIR& node) override;

    void visit(lir::VarDeclLIR& node) override;
    void visit(lir::BinaryExprLIR& node) override;
    void visit(lir::UnaryExprLIR& node) override;
    void visit(lir::CastExprLIR& node) override;
    void visit(lir::AssignExprLIR& node) override;
    void visit(lir::CondExprLIR& node) override;
    void visit(lir::IdentExprLIR& node) override;
    void visit(lir::LiteralExprLIR& node) override;
    void visit(lir::ZeroExprLIR& node) override;
    void visit(lir::CallExprLIR& node) override;
    void visit(lir::MemberAccExprLIR& node) override;
    void visit(lir::SubscrExprLIR& node) override;
    void visit(lir::PostfixExprLIR& node) override;

private:
    Ref<ProgramCFG> prog_cfg;
    Vec<Box<NestedStmtInfo>> infostack;
    MonotonicCtr<uint64_t> ctr = 1;
};

} // namespace ecc::codegen::cfg

#endif