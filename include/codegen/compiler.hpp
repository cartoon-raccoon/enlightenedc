#pragma once

#ifndef ECC_COMPILER_H
#define ECC_COMPILER_H

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include "lowering/cfg/cfg.hpp"
#include "lowering/cfg/walker.hpp"
#include "codegen/llvm.hpp"
#include "util.hpp"

using namespace ecc;
using namespace util;

namespace ecc::codegen {
/*
LLVM IR generation functionality.
*/

class LLVMSynthesizer : public lower::cfg::RevPostorderCFGWalker, public NoMove {
    Ref<llvm::LLVMContext> ctxtref;
    Ref<llvm::Module> modref;
    Ref<llvm::IRBuilder<>> irbref;

protected:
    llvm::LLVMContext& ctxt() { return ctxtref; }
    llvm::Module& mod() { return modref; }
    llvm::IRBuilder<>& irb() { return irbref; }

public:
    LLVMSynthesizer(LLVMUnit& llvm);

    void compile(lower::cfg::ProgramCFG& prog);

    // Visitor method overrides
    
};

} // namespace ecc::codegen

#endif