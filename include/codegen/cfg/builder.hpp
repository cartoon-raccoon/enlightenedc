#pragma once

#ifndef ECC_CFG_BUILDER_H
#define ECC_CFG_BUILDER_H

#include "codegen/lir/visitor.hpp"

namespace ecc::codegen::cfg {

class CFGBuilder : public lir::LIRVisitor {
public:


    void visit(lir::ProgramLIR& node) override;
    void visit(lir::FunctionLIR& node) override;

    void visit(lir::VarDeclLIR& node) override;

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

    void visit(lir::BinaryExprLIR& node) override;
    void visit(lir::UnaryExprLIR& node) override;
    void visit(lir::CastExprLIR& node) override;
    void visit(lir::AssignExprLIR& node) override;
    void visit(lir::CondExprLIR& node) override;
    void visit(lir::IdentExprLIR& node) override;
    void visit(lir::LiteralExprLIR& node) override;
    void visit(lir::CallExprLIR& node) override;
    void visit(lir::MemberAccExprLIR& node) override;
    void visit(lir::SubscrExprLIR& node) override;
    void visit(lir::PostfixExprLIR& node) override;
};

}

#endif