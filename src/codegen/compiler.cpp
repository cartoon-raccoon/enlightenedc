#include "codegen/compiler.hpp"

using namespace ecc::codegen;
using namespace ecc::lower::cfg;

LLVMSynthesizer::LLVMSynthesizer(LLVMUnit& llvm)
    : ctxtref(llvm.ctx()), modref(llvm.mod()), irbref(llvm.irb()) {
}

void LLVMSynthesizer::compile(ProgramCFG& prog) {

    // todo: validate
}