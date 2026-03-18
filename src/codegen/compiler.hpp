#ifndef ECC_COMPILER_H
#define ECC_COMPILER_H

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>

#include "semantics/mir/mir.hpp"
#include "semantics/mir/visitor.hpp"
#include "semantics/types.hpp"
#include "semantics/symbols.hpp"
#include "util.hpp"

using namespace ecc;
using namespace util;

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
};

} // namespace ecc::compiler

#endif