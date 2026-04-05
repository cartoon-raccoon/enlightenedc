#include "codegen/lir/lir.hpp"

#include "codegen/lir/visitor.hpp"

using namespace codegen::lir;

void ProgramLIR::accept(LIRVisitor& visitor) {
    visitor.visit(*this);
}

void FunctionLIR::accept(LIRVisitor& visitor) {
    visitor.visit(*this);
}

void VarDeclLIR::accept(LIRVisitor& visitor) {
    visitor.visit(*this);
}

void GotoStmtLIR::accept(LIRVisitor& visitor) {
    visitor.visit(*this);
}

void ExprStmtLIR::accept(LIRVisitor& visitor) {
    visitor.visit(*this);
}

void SwitchStmtLIR::accept(LIRVisitor& visitor) {
    visitor.visit(*this);
}

void CaseLIR::accept(LIRVisitor& visitor) {
    visitor.visit(*this);
}

void DefaultLIR::accept(LIRVisitor& visitor) {
    visitor.visit(*this);
}

void BreakStmtLIR::accept(LIRVisitor& visitor) {
    visitor.visit(*this);
}

void ContStmtLIR::accept(LIRVisitor& visitor) {
    visitor.visit(*this);
}

void IfStmtLIR::accept(LIRVisitor& visitor) {
    visitor.visit(*this);
}

void LoopStmtLIR::accept(LIRVisitor& visitor) {
    visitor.visit(*this);
}

void LabelDeclLIR::accept(LIRVisitor& visitor) {
    visitor.visit(*this);
}

void PrintStmtLIR::accept(LIRVisitor& visitor) {
    visitor.visit(*this);
}

void ReturnStmtLIR::accept(LIRVisitor& visitor) {
    visitor.visit(*this);
}

void BinaryExprLIR::accept(LIRVisitor& visitor) {
    visitor.visit(*this);
}

void UnaryExprLIR::accept(LIRVisitor& visitor) {
    visitor.visit(*this);
}

void CastExprLIR::accept(LIRVisitor& visitor) {
    visitor.visit(*this);
}

void AssignExprLIR::accept(LIRVisitor& visitor) {
    visitor.visit(*this);
}

void CondExprLIR::accept(LIRVisitor& visitor) {
    visitor.visit(*this);
}

void IdentExprLIR::accept(LIRVisitor& visitor) {
    visitor.visit(*this);
}

void LiteralExprLIR::accept(LIRVisitor& visitor) {
    visitor.visit(*this);
}

void CallExprLIR::accept(LIRVisitor& visitor) {
    visitor.visit(*this);
}

void MemberAccExprLIR::accept(LIRVisitor& visitor) {
    visitor.visit(*this);
}

void SubscrExprLIR::accept(LIRVisitor& visitor) {
    visitor.visit(*this);
}

void PostfixExprLIR::accept(LIRVisitor& visitor) {
    visitor.visit(*this);
}

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