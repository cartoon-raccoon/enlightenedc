#include "semantics/mir/mir.hpp"
#include "semantics/mir/visitor.hpp"

using namespace sema::mir;

void ProgramMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void FunctionMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void InitializerMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void TypeDeclMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void VarDeclMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void CompoundStmtMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void ExprStmtMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void SwitchStmtMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void CaseRangeStmtMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void DefaultStmtMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void LabeledStmtMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void PrintStmtMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void IfStmtMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void LoopStmtMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void GotoStmtMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void BreakStmtMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void ReturnStmtMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void BinaryExprMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void UnaryExprMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void CastExprMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void AssignExprMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void CondExprMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void IdentExprMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void LiteralExprMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void StringExprMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void CallExprMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void MemberAccExprMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void SubscrExprMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void PostfixExprMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void SizeofExprMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void CompoundStmtMIR::add_item(Box<ProgramItemMIR> item) {
    items.push_back(std::move(item));
}

void ProgramMIR::add_item(Box<ProgramItemMIR> item) {
    items.push_back(std::move(item));
}
