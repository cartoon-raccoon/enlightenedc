#pragma once

#ifndef ECC_CFG_BUILDER_H
#define ECC_CFG_BUILDER_H

#include <concepts>
#include <utility>

#include "lowering/cfg/cfg.hpp"
#include "lowering/lir/lir.hpp"
#include "lowering/lir/visitor.hpp"
#include "util.hpp"

using namespace ecc;
using namespace util;

/*
CFG generation functionality.
*/
namespace ecc::lower::cfg {

class SwitchStmtInfo;
class LoopStmtInfo;
class Value;

class NestedStmtInfo {
public:
    enum class Kind : uint8_t {
        SWITCH,
        LOOP,
    };

    NestedStmtInfo(Kind kind) : kind(kind) {}
    virtual ~NestedStmtInfo() = default;

    Kind kind;
    // All gotos pending a set_target to a merge block.
    Vec<Goto *> pending_merges;

    bool is_switch() const { return kind == Kind::SWITCH; }
    bool is_loop() const { return kind == Kind::LOOP; }

    virtual SwitchStmtInfo *as_switch() { return nullptr; }
    virtual LoopStmtInfo *as_loop() { return nullptr; }
};

class SwitchStmtInfo : public NestedStmtInfo {
public:
    SwitchStmtInfo(Switch *swtch) : NestedStmtInfo(Kind::SWITCH), swtch(swtch) {}

    Switch *swtch;

    SwitchStmtInfo *as_switch() override { return this; }
};

class LoopStmtInfo : public NestedStmtInfo {
public:
    LoopStmtInfo() : NestedStmtInfo(Kind::LOOP) {}

    BasicBlock *cond = nullptr;
    BasicBlock *step = nullptr;
    BasicBlock *body = nullptr;

    LoopStmtInfo *as_loop() override { return this; }
};

/**
Class that builds the Control Flow Graph.
*/
class CFGBuilder : public lir::LIRVisitor {
public:
    CFGBuilder(sema::types::TypeContext& types, ProgramCFG& prog_cfg)
        : types(types), prog_cfg(prog_cfg) {}

    void build_cfg(lir::ProgramLIR& prog);

protected:
    FunctionCFG *curr_func = nullptr;
    BasicBlock *curr_blk   = nullptr;
    Value *last_value      = nullptr;

    using NestedStmtFilter = std::function<bool(NestedStmtInfo *)>;

    template <typename Info, typename... Args>
        requires std::derived_from<Info, NestedStmtInfo>
    NestedStmtInfo *push_info(Args... args) {
        auto info = std::make_unique<Info>(std::forward<Args>(args)...);

        NestedStmtInfo *ret = info.get();
        infostack.push_back(std::move(info));

        return ret;
    }

    Box<NestedStmtInfo> pop_info() {
        auto popped = std::move(infostack.back());

        infostack.pop_back();

        return popped;
    }

    NestedStmtInfo *find_info(const NestedStmtFilter& filter);

    Value *eval(lir::ExprLIR& node);

    /**
    Evaluate the expression when appearing in an lvalue position.
    */
    Value *eval_lvalue(lir::ExprLIR& node);

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
    void visit(lir::ReintExprLIR& node) override;
    void visit(lir::SubscrExprLIR& node) override;
    void visit(lir::PostfixExprLIR& node) override;

private:
    sema::types::TypeContext& types;
    ProgramCFG& prog_cfg;
    Vec<Box<NestedStmtInfo>> infostack;
    MonotonicCtr<uint64_t> ctr = 1;
};

} // namespace ecc::lower::cfg

#endif