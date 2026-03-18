#ifndef ECC_COMPILER_H
#define ECC_COMPILER_H

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>

#include "codegen/mir.hpp"
#include "codegen/visitor.hpp"
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

class LLVMVisitor : public mir::MIRVisitor {
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

    void visit(mir::ProgramMIR& node) override;
    void visit(mir::FunctionMIR& node) override;
    
    void visit(mir::InitializerMIR& node) override;
    void visit(mir::TypeDeclMIR& node) override;
    void visit(mir::VarDeclMIR& node) override;

    void visit(mir::CompoundStmtMIR& node) override;
    void visit(mir::ExprStmtMIR& node) override;
    void visit(mir::SwitchStmtMIR& node) override;
    void visit(mir::CaseRangeStmtMIR& node) override;
    void visit(mir::DefaultStmtMIR& node) override;
    void visit(mir::LabeledStmtMIR& node) override;
    void visit(mir::PrintStmtMIR& node) override;
    void visit(mir::IfStmtMIR& node) override;
    void visit(mir::LoopStmtMIR& node) override;
    void visit(mir::GotoStmtMIR& node) override;
    void visit(mir::BreakStmtMIR& node) override;
    void visit(mir::ReturnStmtMIR& node) override;

    void visit(mir::BinaryExprMIR& node) override;
    void visit(mir::UnaryExprMIR& node) override;
    void visit(mir::CastExprMIR& node) override;
    void visit(mir::AssignExprMIR& node) override;
    void visit(mir::CondExprMIR& node) override;
    void visit(mir::IdentExprMIR& node) override;
    void visit(mir::LiteralExprMIR& node) override;
    void visit(mir::StringExprMIR& node) override;
    void visit(mir::CallExprMIR& node) override;
    void visit(mir::MemberAccExprMIR& node) override;
    void visit(mir::SubscrExprMIR& node) override;
    void visit(mir::PostfixExprMIR& node) override;
    void visit(mir::SizeofExprMIR& node) override;
};

} // namespace ecc::compiler

#endif