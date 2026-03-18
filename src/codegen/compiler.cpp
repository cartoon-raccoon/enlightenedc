#include "semantics/mir/mir.hpp"
#include "codegen/compiler.hpp"

using namespace ecc::compiler;
using namespace ecc::sema::mir;
using namespace ecc::sema::types;
using namespace ecc::ast;

void LLVMVisitor::visit(ProgramMIR& node) {}
void LLVMVisitor::visit(FunctionMIR& node) {}

void LLVMVisitor::visit(InitializerMIR& node) {}
void LLVMVisitor::visit(TypeDeclMIR& node) {}
void LLVMVisitor::visit(VarDeclMIR& node) {}

void LLVMVisitor::visit(CompoundStmtMIR& node) {}
void LLVMVisitor::visit(ExprStmtMIR& node) {}
void LLVMVisitor::visit(SwitchStmtMIR& node) {}
void LLVMVisitor::visit(CaseRangeStmtMIR& node) {}
void LLVMVisitor::visit(DefaultStmtMIR& node) {}
void LLVMVisitor::visit(LabeledStmtMIR& node) {}
void LLVMVisitor::visit(PrintStmtMIR& node) {}
void LLVMVisitor::visit(IfStmtMIR& node) {}
void LLVMVisitor::visit(LoopStmtMIR& node) {}
void LLVMVisitor::visit(GotoStmtMIR& node) {}
void LLVMVisitor::visit(BreakStmtMIR& node) {}
void LLVMVisitor::visit(ReturnStmtMIR& node) {}

void LLVMVisitor::visit(BinaryExprMIR& node) {}
void LLVMVisitor::visit(UnaryExprMIR& node) {}
void LLVMVisitor::visit(CastExprMIR& node) {}
void LLVMVisitor::visit(AssignExprMIR& node) {}
void LLVMVisitor::visit(CondExprMIR& node) {}
void LLVMVisitor::visit(IdentExprMIR& node) {}
void LLVMVisitor::visit(LiteralExprMIR& node) {}
void LLVMVisitor::visit(StringExprMIR& node) {}
void LLVMVisitor::visit(CallExprMIR& node) {}
void LLVMVisitor::visit(MemberAccExprMIR& node) {}
void LLVMVisitor::visit(SubscrExprMIR& node) {}
void LLVMVisitor::visit(PostfixExprMIR& node) {}
void LLVMVisitor::visit(SizeofExprMIR& node) {}