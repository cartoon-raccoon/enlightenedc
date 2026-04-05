#pragma once

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#ifndef ECC_COMPILER_H
#define ECC_COMPILER_H

#include <llvm/IR/IRBuilder.h>

#include "codegen/lir/lir.hpp"
#include "codegen/lir/visitor.hpp"
#include "codegen/llvm.hpp"
#include "util.hpp"

using namespace ecc;
using namespace util;

namespace ecc::codegen {
/*
LLVM IR generation functionality.
*/

class LLVMSynthesizer : public lir::LIRVisitor {
    Ref<llvm::LLVMContext> ctxtref;
    Ref<llvm::Module> modref;
    Ref<llvm::IRBuilder<>> irbref;

    Ref<lir::LIRSymbolMap> symsref;

protected:
    llvm::LLVMContext& ctxt() { return ctxtref; }
    llvm::Module& mod() { return modref; }
    llvm::IRBuilder<>& irb() { return irbref; }

public:
    LLVMSynthesizer(lir::LIRSymbolMap& syms, LLVMUnit& llvm);

    void compile(lir::ProgramLIR& prog);

    // Visitor method overrides
    void visit(lir::ProgramLIR& node) override;
    void visit(lir::FunctionLIR& node) override;

    void visit(lir::VarDeclLIR& node) override;

    void visit(lir::ExprStmtLIR& node) override;
    void visit(lir::GotoStmtLIR& node) override;
    void visit(lir::SwitchStmtLIR& node) override;
    void visit(lir::BreakStmtLIR& node) override;
    void visit(lir::ContStmtLIR& node) override;
    void visit(lir::IfStmtLIR& node) override;
    void visit(lir::CaseLIR& node) override;
    void visit(lir::DefaultLIR& node) override;
    void visit(lir::LoopStmtLIR& node) override;
    void visit(lir::LabelDeclLIR& node) override;
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

} // namespace ecc::codegen

#endif