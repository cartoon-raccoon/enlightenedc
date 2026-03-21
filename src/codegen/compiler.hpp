#ifndef ECC_COMPILER_H
#define ECC_COMPILER_H

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>

#include "semantics/types.hpp"
#include "codegen/lir/lir.hpp"
#include "codegen/lir/visitor.hpp"
#include "codegen/lir/symbols.hpp"
#include "util.hpp"

using namespace ecc;
using namespace util;

namespace ecc::codegen {
/*
LLVM IR generation functionality.
*/

using LLVMType = llvm::Type;

// todo: use LLVM DataLayout to handle alignment

class LLVMSynthesizer : public lir::LIRVisitor {
public:
    LLVMSynthesizer(lir::LIRSymbolMap& syms) : syms(syms) {}
    ~LLVMSynthesizer() = default;

    Box<llvm::Module> module;
    Box<llvm::LLVMContext> ctxt;
    Box<llvm::IRBuilder<>> builder;

    lir::LIRSymbolMap& syms;

    /*
    Converts the provided type to its corresponding LLVM type.
    */
    LLVMType *map_to_llvm_type(sema::types::Type *ty);

    // Visitor method overrides
};

} // namespace ecc::codegen

#endif