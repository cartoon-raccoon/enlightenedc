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
    BasicBlock *current_block = nullptr;

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

    // variable declaration and expression nodes don't need to be visited.
    VISIT_NO_IMPL(lir::VarDeclLIR);
    VISIT_NO_IMPL(lir::BinaryExprLIR);
    VISIT_NO_IMPL(lir::UnaryExprLIR);
    VISIT_NO_IMPL(lir::CastExprLIR);
    VISIT_NO_IMPL(lir::AssignExprLIR);
    VISIT_NO_IMPL(lir::CondExprLIR);
    VISIT_NO_IMPL(lir::IdentExprLIR);
    VISIT_NO_IMPL(lir::LiteralExprLIR);
    VISIT_NO_IMPL(lir::CallExprLIR);
    VISIT_NO_IMPL(lir::MemberAccExprLIR);
    VISIT_NO_IMPL(lir::SubscrExprLIR);
    VISIT_NO_IMPL(lir::PostfixExprLIR);

private:
    Ref<ProgramCFG> prog_cfg;
    Vec<Box<NestedStmtInfo>> infostack;
    MonotonicCtr<uint64_t> ctr = 1;
};

} // namespace ecc::codegen::cfg

#endif