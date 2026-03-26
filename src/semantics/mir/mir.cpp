#include "semantics/mir/mir.hpp"
#include "eval/exec.hpp"
#include "semantics/mir/visitor.hpp"

using namespace sema::mir;
using namespace sema::sym;
using namespace tokens;

void ProgramMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void FunctionMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void InitializerMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void TypeDeclMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void VarDeclMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void CompoundStmtMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void ExprStmtMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void SwitchStmtMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void CaseStmtMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void CaseRangeStmtMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void DefaultStmtMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void LabeledStmtMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void PrintStmtMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void IfStmtMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void LoopStmtMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void GotoStmtMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void BreakStmtMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void ContStmtMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void ReturnStmtMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void BinaryExprMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void UnaryExprMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void CastExprMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void AssignExprMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void CondExprMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void IdentExprMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void ConstExprMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void LiteralExprMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void CallExprMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void MemberAccExprMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void SubscrExprMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void PostfixExprMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

void SizeofExprMIR::accept(MIRVisitor& visitor) { visitor.visit(*this); }

exec::Value BinaryExprMIR::eval(exec::Evaluator& ev) { return ev.eval(*this); }

exec::Value UnaryExprMIR::eval(exec::Evaluator& ev) { return ev.eval(*this); }

exec::Value CastExprMIR::eval(exec::Evaluator& ev) { return ev.eval(*this); }

exec::Value AssignExprMIR::eval(exec::Evaluator& ev) { return ev.eval(*this); }

exec::Value CondExprMIR::eval(exec::Evaluator& ev) { return ev.eval(*this); }

exec::Value IdentExprMIR::eval(exec::Evaluator& ev) { return ev.eval(*this); }

exec::Value ConstExprMIR::eval(exec::Evaluator& ev) { return ev.eval(*this); }

exec::Value LiteralExprMIR::eval(exec::Evaluator& ev) { return ev.eval(*this); }

exec::Value CallExprMIR::eval(exec::Evaluator& ev) { return ev.eval(*this); }

exec::Value MemberAccExprMIR::eval(exec::Evaluator& ev) { return ev.eval(*this); }

exec::Value SubscrExprMIR::eval(exec::Evaluator& ev) { return ev.eval(*this); }

exec::Value PostfixExprMIR::eval(exec::Evaluator& ev) { return ev.eval(*this); }

exec::Value SizeofExprMIR::eval(exec::Evaluator& ev) { return ev.eval(*this); }

void CompoundStmtMIR::add_item(Box<ProgItemMIR> item) {
    items.push_back(std::move(item));
}

void ProgramMIR::add_item(Box<ProgItemMIR> item) {
    items.push_back(std::move(item));
}

void VarDeclMIR::add_decl(VarSymbol *sym) {
    decls.emplace_back(VarDecl {sym, {}});
}

void VarDeclMIR::add_decl(VarSymbol *sym, Box<InitializerMIR> init) {
    decls.emplace_back(VarDecl {sym, std::move(init)});
}
