#pragma once

#ifndef ECC_COMPILER_H
#define ECC_COMPILER_H

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include "lowering/lir/lir.hpp"
#include "lowering/lir/visitor.hpp"
#include "codegen/llvm.hpp"
#include "util.hpp"

using namespace ecc;
using namespace util;

namespace ecc::codegen {
/*
LLVM IR generation functionality.
*/

class LLVMSynthesizer : public lower::lir::LIRVisitor, public NoMove {
    Ref<llvm::LLVMContext> ctxtref;
    Ref<llvm::Module> modref;
    Ref<llvm::IRBuilder<>> irbref;

    Ref<lower::lir::LIRSymbolMap> symsref;

protected:
    llvm::LLVMContext& ctxt() { return ctxtref; }
    llvm::Module& mod() { return modref; }
    llvm::IRBuilder<>& irb() { return irbref; }

public:
    LLVMSynthesizer(lower::lir::LIRSymbolMap& syms, LLVMUnit& llvm);

    void compile(lower::lir::ProgramLIR& prog);

    // Visitor method overrides
    void visit(lower::lir::ProgramLIR& node) override;
    void visit(lower::lir::FunctionLIR& node) override;

    void visit(lower::lir::VarDeclLIR& node) override;

    void visit(lower::lir::ExprStmtLIR& node) override;
    void visit(lower::lir::GotoStmtLIR& node) override;
    void visit(lower::lir::SwitchStmtLIR& node) override;
    void visit(lower::lir::BreakStmtLIR& node) override;
    void visit(lower::lir::ContStmtLIR& node) override;
    void visit(lower::lir::IfStmtLIR& node) override;
    void visit(lower::lir::CaseLIR& node) override;
    void visit(lower::lir::DefaultLIR& node) override;
    void visit(lower::lir::LoopStmtLIR& node) override;
    void visit(lower::lir::LabelDeclLIR& node) override;
    void visit(lower::lir::PrintStmtLIR& node) override;
    void visit(lower::lir::ReturnStmtLIR& node) override;

    void visit(lower::lir::BinaryExprLIR& node) override;
    void visit(lower::lir::UnaryExprLIR& node) override;
    void visit(lower::lir::CastExprLIR& node) override;
    void visit(lower::lir::AssignExprLIR& node) override;
    void visit(lower::lir::CondExprLIR& node) override;
    void visit(lower::lir::IdentExprLIR& node) override;
    void visit(lower::lir::LiteralExprLIR& node) override;
    void visit(lower::lir::ZeroExprLIR& node) override;
    void visit(lower::lir::CallExprLIR& node) override;
    void visit(lower::lir::MemberAccExprLIR& node) override;
    void visit(lower::lir::ReintExprLIR& node) override;
    void visit(lower::lir::SubscrExprLIR& node) override;
    void visit(lower::lir::PostfixExprLIR& node) override;
};

} // namespace ecc::codegen

#endif