#include "codegen/compiler.hpp"

#include "codegen/lir/lir.hpp"

using namespace ecc::codegen;
using namespace ecc::codegen::lir;

LLVMSynthesizer::LLVMSynthesizer(lir::LIRSymbolMap& syms, LLVMUnit& llvm)
    : symsref(syms), ctxtref(llvm.ctx()), modref(llvm.mod()), irbref(llvm.irb()) {
}

void LLVMSynthesizer::compile(ProgramLIR& prog) {
    prog.accept(*this);

    // todo: validate
}

void LLVMSynthesizer::visit(ProgramLIR& node) {
}

void LLVMSynthesizer::visit(FunctionLIR& node) {
}

void LLVMSynthesizer::visit(VarDeclLIR& node) {
}

void LLVMSynthesizer::visit(ExprStmtLIR& node) {
}

void LLVMSynthesizer::visit(GotoStmtLIR& node) {
}

void LLVMSynthesizer::visit(SwitchStmtLIR& node) {
}

void LLVMSynthesizer::visit(BreakStmtLIR& node) {
}

void LLVMSynthesizer::visit(ContStmtLIR& node) {
}

void LLVMSynthesizer::visit(IfStmtLIR& node) {
}

void LLVMSynthesizer::visit(CaseLIR& node) {
}

void LLVMSynthesizer::visit(DefaultLIR& node) {
}

void LLVMSynthesizer::visit(LoopStmtLIR& node) {
}

void LLVMSynthesizer::visit(LabelDeclLIR& node) {
}

void LLVMSynthesizer::visit(PrintStmtLIR& node) {
}

void LLVMSynthesizer::visit(ReturnStmtLIR& node) {
}

void LLVMSynthesizer::visit(BinaryExprLIR& node) {
}

void LLVMSynthesizer::visit(UnaryExprLIR& node) {
}

void LLVMSynthesizer::visit(CastExprLIR& node) {
}

void LLVMSynthesizer::visit(AssignExprLIR& node) {
}

void LLVMSynthesizer::visit(CondExprLIR& node) {
}

void LLVMSynthesizer::visit(IdentExprLIR& node) {
}

void LLVMSynthesizer::visit(LiteralExprLIR& node) {
}

void LLVMSynthesizer::visit(CallExprLIR& node) {
}

void LLVMSynthesizer::visit(MemberAccExprLIR& node) {
}

void LLVMSynthesizer::visit(SubscrExprLIR& node) {
}

void LLVMSynthesizer::visit(PostfixExprLIR& node) {
}
