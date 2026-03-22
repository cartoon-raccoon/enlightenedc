#ifndef ECC_COMPILER_H
#define ECC_COMPILER_H

#include "semantics/types.hpp"
#include "codegen/lir/lir.hpp"
#include "codegen/lir/visitor.hpp"
#include "codegen/llvm.hpp"

using namespace ecc;
using namespace util;

namespace ecc::codegen {
/*
LLVM IR generation functionality.
*/

// todo: use LLVM DataLayout to handle alignment

class LLVMSynthesizer : public lir::LIRVisitor {
public:
    LLVMSynthesizer(lir::LIRSymbolMap& syms, LLVM& llvm) : syms(syms), llvm(llvm) {}
    ~LLVMSynthesizer() = default;

    LLVM& llvm;

    lir::LIRSymbolMap& syms;

    /*
    Converts the provided type to its corresponding LLVM type.
    */
    LLVMType *map_to_llvm_type(sema::types::Type *ty);

    // Visitor method overrides
};

} // namespace ecc::codegen

#endif