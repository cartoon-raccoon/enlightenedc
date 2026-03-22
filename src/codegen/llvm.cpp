#include "codegen/llvm.hpp"
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/TargetParser/Host.h>

using namespace ecc::codegen;
using namespace llvm;

LLVM::LLVM() : module(), context(), irbuilder() {
    if (!initialized) {
        llvm::InitializeAllTargetInfos();
        llvm::InitializeAllTargets();
        llvm::InitializeAllTargetMCs();
        llvm::InitializeAllDisassemblers();
        llvm::InitializeAllAsmParsers();
        llvm::InitializeAllAsmPrinters();
        initialized = true;
    }

    auto target_triple = sys::getDefaultTargetTriple();
    module->setTargetTriple(Triple(target_triple));

    std::string error;
    auto target = TargetRegistry::lookupTarget(module->getTargetTriple(), error);
    if (!target) {
        // todo: throw error
    }


}