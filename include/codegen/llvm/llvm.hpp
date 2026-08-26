#pragma once

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
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>

#include "codegen/codegen.hpp"
#include "semantics/types.hpp"
#include "util.hpp"

using namespace ecc;
using namespace util;

namespace ecc::codegen {

using LLVMType = llvm::Type;

/**
Global LLVM state.
*/
class LLVMCore : public CodeGenCore {
    llvm::Triple target_triple;
    const llvm::Target *target;
    llvm::TargetMachine *target_machine;

public:
    LLVMCore();
    ~LLVMCore() override;

    friend class LLVMUnit;

    Box<CodeGenUnit> make_unit(const std::string& unit_name) override;
};

/**
LLVM state for a single translation unit.
*/
class LLVMUnit : public CodeGenUnit {
    Box<llvm::LLVMContext> context;
    Box<llvm::Module> llvmmod;
    Box<llvm::IRBuilder<>> irbuilder;

    HashMap<sema::types::Type *, LLVMType *> typemap;

public:
    LLVMUnit(const std::string& module_name, LLVMCore& llvmcore);

    friend class LLVMCore;

    llvm::LLVMContext& ctx() { return *context; }
    llvm::Module& mod() { return *llvmmod; }
    llvm::IRBuilder<>& irb() { return *irbuilder; }

    LLVMType *get_llvm_type(sema::types::Type *type);

    // Without this, declaring any of the finalize(Xxx *) overrides below hides the entire
    // base-class finalize overload set, including CodeGenUnit::finalize(Type *) -- the
    // non-virtual dispatcher get_llvm_type() below calls.
    using CodeGenUnit::finalize;

    void finalize(sema::types::VoidType *type) override;
    void finalize(sema::types::PrimitiveType *type) override;
    void finalize(sema::types::ClassType *type) override;
    void finalize(sema::types::UnionType *type) override;
    void finalize(sema::types::EnumType *type) override;
    void finalize(sema::types::PointerType *type) override;
    void finalize(sema::types::ArrayType *type) override;
    void finalize(sema::types::FunctionType *type) override;
    void finalize(sema::types::ConstType *type) override;

    size_t get_pointer_size() override;
    size_t get_pointer_size_bits() override;

    size_t alloc_size(sema::types::Type *type) override;

    void compile(lower::cfg::ProgramCFG& prog) override;

protected:
    bool is_finalized(sema::types::Type *type);
};

} // namespace ecc::codegen

#endif