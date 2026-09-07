#pragma once

#ifndef ECC_COMPILER_H
#define ECC_COMPILER_H

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include "codegen/llvm/llvm.hpp"
#include "config.hpp"
#include "lowering/cfg/cfg.hpp"
#include "lowering/cfg/walker.hpp"
#include "util.hpp"

using namespace ecc;
using namespace util;

namespace ecc::codegen {

using RPOrderCFGW = lower::cfg::RevPostorderCFGWalker;

class LLVMGenerator : public RPOrderCFGW, public lower::cfg::CFGVisitor, public NoMove {
    llvm::LLVMContext& ctxtref;
    llvm::Module& modref;
    llvm::IRBuilder<>& irbref;
    RuntimeConfig& rtcfg;

protected:
    llvm::LLVMContext& ctxt() { return ctxtref; }
    llvm::Module& mod() { return modref; }
    llvm::IRBuilder<>& irb() { return irbref; }

public:
    LLVMGenerator(LLVMUnit& llvm, RuntimeConfig& rtcfg);

    void compile(lower::cfg::ProgramCFG& prog);

    // Visitor method overrides
    void visit(lower::cfg::Alloca& inst) override;
    void visit(lower::cfg::LoadInst& inst) override;
    void visit(lower::cfg::StoreInst& inst) override;
    void visit(lower::cfg::PhiInst& inst) override;
    void visit(lower::cfg::PrintInst& inst) override;
    void visit(lower::cfg::MemcpyInst& inst) override;
    void visit(lower::cfg::BinaryInst& inst) override;
    void visit(lower::cfg::UnaryInst& inst) override;
    void visit(lower::cfg::IncrInst& inst) override;
    void visit(lower::cfg::DecrInst& inst) override;
    void visit(lower::cfg::CastInst& inst) override;
    void visit(lower::cfg::MemberAccInst& inst) override;
    void visit(lower::cfg::SubscrInst& inst) override;
    void visit(lower::cfg::CallInst& inst) override;

    void visit(lower::cfg::FunctionCFG& val) override;
    void visit(lower::cfg::ScalarConst& val) override;
    void visit(lower::cfg::PointerConst& val) override;
    void visit(lower::cfg::AggregateConst& val) override;
    void visit(lower::cfg::ZeroConst& val) override;
    void visit(lower::cfg::Global& val) override;
    void visit(lower::cfg::String& val) override;
    void visit(lower::cfg::FuncArg& val) override;

    void visit(lower::cfg::If& term) override;
    void visit(lower::cfg::Goto& term) override;
    void visit(lower::cfg::Switch& term) override;
    void visit(lower::cfg::Return& term) override;
};

} // namespace ecc::codegen

#endif