#include "codegen/lir/lir.hpp"
#include "codegen/lir/visitor.hpp"

using namespace codegen::lir;

void ProgramLIR::accept(LIRVisitor& visitor) { visitor.visit(*this); }

void FunctionLIR::accept(LIRVisitor& visitor) { visitor.visit(*this); }

void VarDeclLIR::accept(LIRVisitor& visitor) { visitor.visit(*this); }

void GotoStmtLIR::accept(LIRVisitor& visitor) { visitor.visit(*this); }

void ExprStmtLIR::accept(LIRVisitor& visitor) { visitor.visit(*this); }

void SwitchStmtLIR::accept(LIRVisitor& visitor) { visitor.visit(*this); }

void BreakStmtLIR::accept(LIRVisitor& visitor) { visitor.visit(*this); }

void ContStmtLIR::accept(LIRVisitor& visitor) { visitor.visit(*this); }

void IfStmtLIR::accept(LIRVisitor& visitor) { visitor.visit(*this); }

void LoopStmtLIR::accept(LIRVisitor& visitor) { visitor.visit(*this); }

void LabelStmtLIR::accept(LIRVisitor& visitor) { visitor.visit(*this); }

void PrintStmtLIR::accept(LIRVisitor& visitor) { visitor.visit(*this); }

void ReturnStmtLIR::accept(LIRVisitor& visitor) { visitor.visit(*this); }

void BinaryExprLIR::accept(LIRVisitor& visitor) { visitor.visit(*this); }

void UnaryExprLIR::accept(LIRVisitor& visitor) { visitor.visit(*this); }

void CastExprLIR::accept(LIRVisitor& visitor) { visitor.visit(*this); }

void AssignExprLIR::accept(LIRVisitor& visitor) { visitor.visit(*this); }

void CondExprLIR::accept(LIRVisitor& visitor) { visitor.visit(*this); }

void IdentExprLIR::accept(LIRVisitor& visitor) { visitor.visit(*this); }

void LiteralExprLIR::accept(LIRVisitor& visitor) { visitor.visit(*this); }

void CallExprLIR::accept(LIRVisitor& visitor) { visitor.visit(*this); }

void MemberAccExprLIR::accept(LIRVisitor& visitor) { visitor.visit(*this); }

void SubscrExprLIR::accept(LIRVisitor& visitor) { visitor.visit(*this); }

void PostfixExprLIR::accept(LIRVisitor& visitor) { visitor.visit(*this); }