#include "semantics/mir/mir.hpp"
#include <algorithm>
#include <variant>

#include "eval/consteval.hpp"
#include "semantics/mir/visitor.hpp"

using namespace sema::mir;
using namespace sema::sym;
using namespace tokens;

void ProgramMIR::accept(MIRVisitor& visitor) {
    visitor.visit(*this);
}

void FunctionMIR::accept(MIRVisitor& visitor) {
    visitor.visit(*this);
}

void InitializerMIR::accept(MIRVisitor& visitor) {
    visitor.visit(*this);
}

void TypeDeclMIR::accept(MIRVisitor& visitor) {
    visitor.visit(*this);
}

void VarDeclMIR::accept(MIRVisitor& visitor) {
    visitor.visit(*this);
}

void CompoundStmtMIR::accept(MIRVisitor& visitor) {
    visitor.visit(*this);
}

void ExprStmtMIR::accept(MIRVisitor& visitor) {
    visitor.visit(*this);
}

void SwitchStmtMIR::accept(MIRVisitor& visitor) {
    visitor.visit(*this);
}

void CaseStmtMIR::accept(MIRVisitor& visitor) {
    visitor.visit(*this);
}

void CaseRangeStmtMIR::accept(MIRVisitor& visitor) {
    visitor.visit(*this);
}

void DefaultStmtMIR::accept(MIRVisitor& visitor) {
    visitor.visit(*this);
}

void LabeledStmtMIR::accept(MIRVisitor& visitor) {
    visitor.visit(*this);
}

void PrintStmtMIR::accept(MIRVisitor& visitor) {
    visitor.visit(*this);
}

void IfStmtMIR::accept(MIRVisitor& visitor) {
    visitor.visit(*this);
}

void LoopStmtMIR::accept(MIRVisitor& visitor) {
    visitor.visit(*this);
}

void GotoStmtMIR::accept(MIRVisitor& visitor) {
    visitor.visit(*this);
}

void BreakStmtMIR::accept(MIRVisitor& visitor) {
    visitor.visit(*this);
}

void ContStmtMIR::accept(MIRVisitor& visitor) {
    visitor.visit(*this);
}

void ReturnStmtMIR::accept(MIRVisitor& visitor) {
    visitor.visit(*this);
}

void BinaryExprMIR::accept(MIRVisitor& visitor) {
    visitor.visit(*this);
}

void UnaryExprMIR::accept(MIRVisitor& visitor) {
    visitor.visit(*this);
}

void CastExprMIR::accept(MIRVisitor& visitor) {
    visitor.visit(*this);
}

void AssignExprMIR::accept(MIRVisitor& visitor) {
    visitor.visit(*this);
}

void CondExprMIR::accept(MIRVisitor& visitor) {
    visitor.visit(*this);
}

void IdentExprMIR::accept(MIRVisitor& visitor) {
    visitor.visit(*this);
}

void ConstExprMIR::accept(MIRVisitor& visitor) {
    visitor.visit(*this);
}

void LiteralExprMIR::accept(MIRVisitor& visitor) {
    visitor.visit(*this);
}

void CallExprMIR::accept(MIRVisitor& visitor) {
    visitor.visit(*this);
}

void MemberAccExprMIR::accept(MIRVisitor& visitor) {
    visitor.visit(*this);
}

void SubscrExprMIR::accept(MIRVisitor& visitor) {
    visitor.visit(*this);
}

void PostfixExprMIR::accept(MIRVisitor& visitor) {
    visitor.visit(*this);
}

void SizeofExprMIR::accept(MIRVisitor& visitor) {
    visitor.visit(*this);
}

eval::Value BinaryExprMIR::eval(eval::ExprEvaluator& ev) {
    return ev.eval(*this);
}

eval::Value UnaryExprMIR::eval(eval::ExprEvaluator& ev) {
    return ev.eval(*this);
}

eval::Value CastExprMIR::eval(eval::ExprEvaluator& ev) {
    return ev.eval(*this);
}

eval::Value AssignExprMIR::eval(eval::ExprEvaluator& ev) {
    return ev.eval(*this);
}

eval::Value CondExprMIR::eval(eval::ExprEvaluator& ev) {
    return ev.eval(*this);
}

eval::Value IdentExprMIR::eval(eval::ExprEvaluator& ev) {
    return ev.eval(*this);
}

eval::Value ConstExprMIR::eval(eval::ExprEvaluator& ev) {
    return ev.eval(*this);
}

eval::Value LiteralExprMIR::eval(eval::ExprEvaluator& ev) {
    return ev.eval(*this);
}

eval::Value CallExprMIR::eval(eval::ExprEvaluator& ev) {
    return ev.eval(*this);
}

eval::Value MemberAccExprMIR::eval(eval::ExprEvaluator& ev) {
    return ev.eval(*this);
}

eval::Value SubscrExprMIR::eval(eval::ExprEvaluator& ev) {
    return ev.eval(*this);
}

eval::Value PostfixExprMIR::eval(eval::ExprEvaluator& ev) {
    return ev.eval(*this);
}

eval::Value SizeofExprMIR::eval(eval::ExprEvaluator& ev) {
    return ev.eval(*this);
}

void CompoundStmtMIR::add_item(Box<ProgItemMIR> item) {
    items.push_back(std::move(item));
}

void ProgramMIR::add_item(Box<ProgItemMIR> item) {
    items.push_back(std::move(item));
}

bool InitializerMIR::is_all_literals() {
    return std::visit(match {
        [] (Box<ExprMIR>& expr) {
            return expr->kind == MIRNode::NodeKind::LITEXPR_MIR;
        },
        [] (Vec<Box<InitializerMIR>>& init) {
            return std::all_of(
                init.cbegin(), init.cend(), 
                [](const Box<InitializerMIR>& init) {return init->is_all_literals();});
        }
    }, initializer);
}

void VarDeclMIR::add_decl(VarSymbol *sym) {
    decls.emplace_back(VarDecl{sym, {}});
}

void VarDeclMIR::add_decl(VarSymbol *sym, Box<InitializerMIR> init) {
    decls.emplace_back(VarDecl{sym, std::move(init)});
}
