#ifndef ECC_LLVM_H
#define ECC_LLVM_H

#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>

#include "util.hpp"

using namespace ecc;
using namespace util;

namespace ecc::codegen {

using LLVMType = llvm::Type;

/**
Global LLVM state.
*/
class LLVMCore {
    llvm::Triple target_triple;
    const llvm::Target *target;
    llvm::TargetMachine *target_machine;
public:
    LLVMCore();
    ~LLVMCore();

    friend class LLVMUnit;
};

/**
LLVM state for a single translation unit.
*/
class LLVMUnit {
    Box<llvm::LLVMContext> context;
    Box<llvm::Module> llvmmod;
    Box<llvm::IRBuilder<>> irbuilder;

public:
    LLVMUnit(std::string& module_name, LLVMCore& llvmcore);
    ~LLVMUnit();

    friend class LLVMCore;

    llvm::LLVMContext& ctx() { return *context; }
    llvm::Module& mod() { return *llvmmod; }
    llvm::IRBuilder<>& irb() { return *irbuilder; }

};

}

#endif