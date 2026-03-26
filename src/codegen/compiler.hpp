#ifndef ECC_COMPILER_H
#define ECC_COMPILER_H

#include "codegen/lir/lir.hpp"
#include "codegen/lir/visitor.hpp"
#include "codegen/llvm.hpp"
#include <llvm/IR/IRBuilder.h>

using namespace ecc;
using namespace util;

namespace ecc::codegen {
/*
LLVM IR generation functionality.
*/

// todo: use LLVM DataLayout to handle alignment

class LLVMSynthesizer : public lir::LIRVisitor {
public:
    LLVMSynthesizer(lir::LIRSymbolMap& syms, LLVMUnit& llvm);
    ~LLVMSynthesizer() = default;

    llvm::LLVMContext& ctxt;
    llvm::Module& mod;
    llvm::IRBuilder<>& irb;

    lir::LIRSymbolMap& syms;

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
    void visit(lir::CaseStmtLIR& node) override;
    void visit(lir::DefaultStmtLIR& node) override;
    void visit(lir::LoopStmtLIR& node) override;
    void visit(lir::LabelStmtLIR& node) override;
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