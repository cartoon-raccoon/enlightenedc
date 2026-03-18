#ifndef ECC_COMPILER_H
#define ECC_COMPILER_H

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>

#include "semantics/mir/mir.hpp"
#include "semantics/mir/visitor.hpp"
#include "util.hpp"
#include "semantics/types.hpp"
#include "semantics/symbols.hpp"

using namespace ecc;

namespace ecc::compiler {
/*
LLVM IR generation functionality.
*/

using LLVMType = llvm::Type;

// todo: use LLVM DataLayout to handle alignment

class LLVMVisitor : public sema::mir::MIRVisitor {
public:
    LLVMVisitor(sema::sym::SymbolTable& syms, sema::types::TypeContext& tys)
    : syms(syms), tys(tys) {}
    ~LLVMVisitor() = default;

    Box<llvm::Module> module;
    Box<llvm::LLVMContext> ctxt;
    Box<llvm::IRBuilder<>> builder;

    sema::sym::SymbolTable& syms;
    sema::types::TypeContext& tys;

    /*
    Converts the provided type to its corresponding LLVM type.
    */
    LLVMType *map_to_llvm_type(sema::types::Type *ty);

    // Visitor method overrides

    void visit(sema::mir::ProgramMIR& node) override;
    void visit(sema::mir::FunctionMIR& node) override;
    
    void visit(sema::mir::InitializerMIR& node) override;
    void visit(sema::mir::TypeDeclMIR& node) override;
    void visit(sema::mir::VarDeclMIR& node) override;

    void visit(sema::mir::CompoundStmtMIR& node) override;
    void visit(sema::mir::ExprStmtMIR& node) override;
    void visit(sema::mir::SwitchStmtMIR& node) override;
    void visit(sema::mir::CaseRangeStmtMIR& node) override;
    void visit(sema::mir::DefaultStmtMIR& node) override;
    void visit(sema::mir::LabeledStmtMIR& node) override;
    void visit(sema::mir::PrintStmtMIR& node) override;
    void visit(sema::mir::IfStmtMIR& node) override;
    void visit(sema::mir::LoopStmtMIR& node) override;
    void visit(sema::mir::GotoStmtMIR& node) override;
    void visit(sema::mir::BreakStmtMIR& node) override;
    void visit(sema::mir::ReturnStmtMIR& node) override;

    void visit(sema::mir::BinaryExprMIR& node) override;
    void visit(sema::mir::UnaryExprMIR& node) override;
    void visit(sema::mir::CastExprMIR& node) override;
    void visit(sema::mir::AssignExprMIR& node) override;
    void visit(sema::mir::CondExprMIR& node) override;
    void visit(sema::mir::IdentExprMIR& node) override;
    void visit(sema::mir::LiteralExprMIR& node) override;
    void visit(sema::mir::StringExprMIR& node) override;
    void visit(sema::mir::CallExprMIR& node) override;
    void visit(sema::mir::MemberAccExprMIR& node) override;
    void visit(sema::mir::SubscrExprMIR& node) override;
    void visit(sema::mir::PostfixExprMIR& node) override;
    void visit(sema::mir::SizeofExprMIR& node) override;
};

} // namespace ecc::compiler

#endif