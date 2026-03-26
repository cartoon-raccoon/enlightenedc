#include "codegen/llvm.hpp"

#include "error.hpp"
#include "util.hpp"
#include <llvm/TargetParser/Host.h>

using namespace ecc::codegen;
using namespace llvm;

LLVMCore::LLVMCore() {
    dbprint("Initializing LLVM");
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllDisassemblers();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    auto triple_str = sys::getDefaultTargetTriple();
    target_triple = Triple(triple_str);
    std::string error;
    target = TargetRegistry::lookupTarget(target_triple, error);
    if (!target) {
        throw EccError(ErrorSource::LLVM, error);
    }

    auto cpu = sys::getHostCPUName();
    auto features = "";
    TargetOptions opt;

    target_machine = target->createTargetMachine(
        Triple(target_triple), cpu, features, opt, Reloc::PIC_);
    
    dbprint("LLVM initialized");
}

LLVMCore::~LLVMCore() {
    dbprint("LLVM: Shutting down");
    llvm::llvm_shutdown();
}

LLVMUnit::LLVMUnit(std::string& module_name, LLVMCore& llvmcore) : context(), llvmmod(), irbuilder() {
    dbprint("LLVM: Creating LLVMUnit with module name '", module_name, "'");
    context = std::make_unique<llvm::LLVMContext>();
    llvmmod = std::make_unique<llvm::Module>(module_name, *context);
    irbuilder = std::make_unique<llvm::IRBuilder<>>(*context);

    llvmmod->setTargetTriple(llvmcore.target_triple);

    llvmmod->setDataLayout(llvmcore.target_machine->createDataLayout());

    dbprint("LLVM: LLVMUnit created");
}

LLVMUnit::~LLVMUnit() {
    
}