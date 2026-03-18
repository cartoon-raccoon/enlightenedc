#include "compiler.hpp"

using namespace ecc::compiler;
using namespace ecc::compiler::mir;
using namespace ecc::sema::types;
using namespace ecc::ast;

void LLVMVisitor::visit(mir::ProgramMIR& node) {}
void LLVMVisitor::visit(mir::FunctionMIR& node) {}

void LLVMVisitor::visit(mir::InitializerMIR& node) {}
void LLVMVisitor::visit(mir::TypeDeclMIR& node) {}
void LLVMVisitor::visit(mir::VarDeclMIR& node) {}

void LLVMVisitor::visit(mir::CompoundStmtMIR& node) {}
void LLVMVisitor::visit(mir::ExprStmtMIR& node) {}
void LLVMVisitor::visit(mir::SwitchStmtMIR& node) {}
void LLVMVisitor::visit(mir::CaseRangeStmtMIR& node) {}
void LLVMVisitor::visit(mir::DefaultStmtMIR& node) {}
void LLVMVisitor::visit(mir::LabeledStmtMIR& node) {}
void LLVMVisitor::visit(mir::PrintStmtMIR& node) {}
void LLVMVisitor::visit(mir::IfStmtMIR& node) {}
void LLVMVisitor::visit(mir::LoopStmtMIR& node) {}
void LLVMVisitor::visit(mir::GotoStmtMIR& node) {}
void LLVMVisitor::visit(mir::BreakStmtMIR& node) {}
void LLVMVisitor::visit(mir::ReturnStmtMIR& node) {}

void LLVMVisitor::visit(mir::BinaryExprMIR& node) {}
void LLVMVisitor::visit(mir::UnaryExprMIR& node) {}
void LLVMVisitor::visit(mir::CastExprMIR& node) {}
void LLVMVisitor::visit(mir::AssignExprMIR& node) {}
void LLVMVisitor::visit(mir::CondExprMIR& node) {}
void LLVMVisitor::visit(mir::IdentExprMIR& node) {}
void LLVMVisitor::visit(mir::LiteralExprMIR& node) {}
void LLVMVisitor::visit(mir::StringExprMIR& node) {}
void LLVMVisitor::visit(mir::CallExprMIR& node) {}
void LLVMVisitor::visit(mir::MemberAccExprMIR& node) {}
void LLVMVisitor::visit(mir::SubscrExprMIR& node) {}
void LLVMVisitor::visit(mir::PostfixExprMIR& node) {}
void LLVMVisitor::visit(mir::SizeofExprMIR& node) {}