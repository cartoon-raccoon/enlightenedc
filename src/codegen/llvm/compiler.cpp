#include "codegen/llvm/compiler.hpp"
#include "config.hpp"

using namespace ecc::codegen;
using namespace ecc::lower::cfg;

LLVMGenerator::LLVMGenerator(LLVMUnit& llvm, RuntimeConfig& rtcfg)
    : ctxtref(llvm.ctx()), modref(llvm.mod()), irbref(llvm.irb()), rtcfg(rtcfg) {
}

void LLVMGenerator::compile(Program& prog) {
    for (auto& glob : prog.get_globals()) {
        glob->accept(*this);
    }
    for (auto& func : prog.get_functions()) {
        func->accept(*this);
    }

    // todo: validate
}

// todo: remove pragmas when done
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

void LLVMGenerator::visit(lower::cfg::Alloca& inst) {}

void LLVMGenerator::visit(lower::cfg::LoadInst& inst) {}

void LLVMGenerator::visit(lower::cfg::StoreInst& inst) {}

void LLVMGenerator::visit(lower::cfg::PhiInst& inst) {}

void LLVMGenerator::visit(lower::cfg::PrintInst& inst) {}

void LLVMGenerator::visit(lower::cfg::MemcpyInst& inst) {}

void LLVMGenerator::visit(lower::cfg::BinaryInst& inst) {}

void LLVMGenerator::visit(lower::cfg::UnaryInst& inst) {}

void LLVMGenerator::visit(lower::cfg::IncrInst& inst) {}

void LLVMGenerator::visit(lower::cfg::DecrInst& inst) {}

void LLVMGenerator::visit(lower::cfg::CastInst& inst) {}

void LLVMGenerator::visit(lower::cfg::MemberAccInst& inst) {}

void LLVMGenerator::visit(lower::cfg::SubscrInst& inst) {}

void LLVMGenerator::visit(lower::cfg::CallInst& inst) {}

void LLVMGenerator::visit(lower::cfg::Function& val) {}

void LLVMGenerator::visit(lower::cfg::ScalarConst& val) {}

void LLVMGenerator::visit(lower::cfg::PointerConst& val) {}

void LLVMGenerator::visit(lower::cfg::AggregateConst& val) {}

void LLVMGenerator::visit(lower::cfg::ZeroConst& val) {}

void LLVMGenerator::visit(lower::cfg::Global& val) {}

void LLVMGenerator::visit(lower::cfg::String& val) {}

void LLVMGenerator::visit(lower::cfg::FuncArg& val) {}

void LLVMGenerator::visit(lower::cfg::If& term) {}

void LLVMGenerator::visit(lower::cfg::Goto& term) {}

void LLVMGenerator::visit(lower::cfg::Switch& term) {}

void LLVMGenerator::visit(lower::cfg::Return& term) {}

#pragma clang diagnostic pop