#include "codegen/lir/lir.hpp"

#include "codegen/lir/visitor.hpp"
#include "util.hpp"

using namespace codegen::lir;

DO_ACCEPT(ProgramLIR, LIRVisitor)
DO_ACCEPT(FunctionLIR, LIRVisitor)
DO_ACCEPT(VarDeclLIR, LIRVisitor)
DO_ACCEPT(GotoStmtLIR, LIRVisitor)
DO_ACCEPT(ExprStmtLIR, LIRVisitor)
DO_ACCEPT(SwitchStmtLIR, LIRVisitor)
DO_ACCEPT(CaseLIR, LIRVisitor)
DO_ACCEPT(DefaultLIR, LIRVisitor)
DO_ACCEPT(BreakStmtLIR, LIRVisitor)
DO_ACCEPT(ContStmtLIR, LIRVisitor)
DO_ACCEPT(IfStmtLIR, LIRVisitor)
DO_ACCEPT(LoopStmtLIR, LIRVisitor)
DO_ACCEPT(LabelDeclLIR, LIRVisitor)
DO_ACCEPT(PrintStmtLIR, LIRVisitor)
DO_ACCEPT(ReturnStmtLIR, LIRVisitor)
DO_ACCEPT(BinaryExprLIR, LIRVisitor)
DO_ACCEPT(UnaryExprLIR, LIRVisitor)
DO_ACCEPT(CastExprLIR, LIRVisitor)
DO_ACCEPT(AssignExprLIR, LIRVisitor)
DO_ACCEPT(CondExprLIR, LIRVisitor)
DO_ACCEPT(IdentExprLIR, LIRVisitor)
DO_ACCEPT(LiteralExprLIR, LIRVisitor)
DO_ACCEPT(CallExprLIR, LIRVisitor)
DO_ACCEPT(MemberAccExprLIR, LIRVisitor)
DO_ACCEPT(SubscrExprLIR, LIRVisitor)
DO_ACCEPT(PostfixExprLIR, LIRVisitor)

Vec<SwitchTarget *> IfStmtLIR::pull_switch_targets() {
    Vec<SwitchTarget *> targets{};

    for (auto& item : then_br) {
        StmtLIR *stmt = item->as_stmt();
        if (stmt) {
            auto inner_targets = stmt->pull_switch_targets();
            targets.insert(targets.end(), inner_targets.begin(), inner_targets.end());
        }
    }

    if (else_br) {
        for (auto& item : *else_br) {
            StmtLIR *stmt = item->as_stmt();
            if (stmt) {
                auto inner_targets = stmt->pull_switch_targets();
                targets.insert(targets.end(), inner_targets.begin(), inner_targets.end());
            }
        }
    }

    return std::move(targets);
}

Vec<SwitchTarget *> LoopStmtLIR::pull_switch_targets() {
    Vec<SwitchTarget *> targets{};

    // only search the body, as that is the only "block" that is user defined.
    for (auto& item : body) {
        StmtLIR *stmt = item->as_stmt();
        if (stmt) {
            auto inner_targets = stmt->pull_switch_targets();
            targets.insert(targets.end(), inner_targets.begin(), inner_targets.end());
        }
    }

    return std::move(targets);
}