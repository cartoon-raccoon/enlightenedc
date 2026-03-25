#include "codegen/llvm.hpp"

#include "error.hpp"
#include "util.hpp"
#include <llvm/TargetParser/Host.h>

using namespace ecc::codegen;
using namespace llvm;

LLVM::LLVM(std::string& module_name) : 
    context(), 
    llvmmod(), 
    irbuilder()
{
    if (!initialized) {
        dbprint("initializing LLVM");
        llvm::InitializeAllTargetInfos();
        llvm::InitializeAllTargets();
        llvm::InitializeAllTargetMCs();
        llvm::InitializeAllDisassemblers();
        llvm::InitializeAllAsmParsers();
        llvm::InitializeAllAsmPrinters();
        initialized = true;
    }

    context = std::make_unique<llvm::LLVMContext>();
    llvmmod = std::make_unique<llvm::Module>(module_name, *context);
    irbuilder = std::make_unique<llvm::IRBuilder<>>(*context);

    auto target_triple = sys::getDefaultTargetTriple();
    llvmmod->setTargetTriple(Triple(target_triple));

    std::string error;
    auto target = TargetRegistry::lookupTarget(llvmmod->getTargetTriple(), error);
    if (!target) {
        throw EccError(ErrorSource::LLVM, error);
    }

    auto cpu = sys::getHostCPUName();
    auto features = "";
    TargetOptions opt;

    auto target_machine = target->createTargetMachine(
        Triple(target_triple), cpu, features, opt, Reloc::PIC_);

    llvmmod->setDataLayout(target_machine->createDataLayout());

    this->target = target;
    this->target_machine = target_machine;

    dbprint("LLVM initialized");
}

LLVM::~LLVM() {
    
}