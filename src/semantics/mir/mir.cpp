#include "semantics/mir/mir.hpp"

#include <algorithm>
#include <variant>

#include "eval/consteval.hpp"
#include "semantics/mir/visitor.hpp"
#include "util.hpp"

#define DO_EVAL(mirty, evalr)            \
    eval::Value mirty::eval(evalr& ev) { \
        return ev.eval(*this);           \
    }

using namespace sema::mir;
using namespace sema::sym;
using namespace tokens;

DO_ACCEPT(ProgramMIR, MIRVisitor)
DO_ACCEPT(FunctionMIR, MIRVisitor)
DO_ACCEPT(InitializerMIR, MIRVisitor)
DO_ACCEPT(TypeDeclMIR, MIRVisitor)
DO_ACCEPT(VarDeclMIR, MIRVisitor)
DO_ACCEPT(CompoundStmtMIR, MIRVisitor)
DO_ACCEPT(ExprStmtMIR, MIRVisitor)
DO_ACCEPT(SwitchStmtMIR, MIRVisitor)
DO_ACCEPT(CaseStmtMIR, MIRVisitor)
DO_ACCEPT(CaseRangeStmtMIR, MIRVisitor)
DO_ACCEPT(DefaultStmtMIR, MIRVisitor)
DO_ACCEPT(LabeledStmtMIR, MIRVisitor)
DO_ACCEPT(PrintStmtMIR, MIRVisitor)
DO_ACCEPT(IfStmtMIR, MIRVisitor)
DO_ACCEPT(LoopStmtMIR, MIRVisitor)
DO_ACCEPT(GotoStmtMIR, MIRVisitor)
DO_ACCEPT(BreakStmtMIR, MIRVisitor)
DO_ACCEPT(ContStmtMIR, MIRVisitor)
DO_ACCEPT(ReturnStmtMIR, MIRVisitor)
DO_ACCEPT(BinaryExprMIR, MIRVisitor)
DO_ACCEPT(UnaryExprMIR, MIRVisitor)
DO_ACCEPT(CastExprMIR, MIRVisitor)
DO_ACCEPT(AssignExprMIR, MIRVisitor)
DO_ACCEPT(CondExprMIR, MIRVisitor)
DO_ACCEPT(IdentExprMIR, MIRVisitor)
DO_ACCEPT(LiteralExprMIR, MIRVisitor)
DO_ACCEPT(CallExprMIR, MIRVisitor)
DO_ACCEPT(MemberAccExprMIR, MIRVisitor)
DO_ACCEPT(SubscrExprMIR, MIRVisitor)
DO_ACCEPT(PostfixExprMIR, MIRVisitor)
DO_ACCEPT(SizeofExprMIR, MIRVisitor)

DO_EVAL(BinaryExprMIR, eval::ExprEvaluator)
DO_EVAL(UnaryExprMIR, eval::ExprEvaluator)
DO_EVAL(CastExprMIR, eval::ExprEvaluator)
DO_EVAL(AssignExprMIR, eval::ExprEvaluator)
DO_EVAL(CondExprMIR, eval::ExprEvaluator)
DO_EVAL(IdentExprMIR, eval::ExprEvaluator)
DO_EVAL(LiteralExprMIR, eval::ExprEvaluator)
DO_EVAL(CallExprMIR, eval::ExprEvaluator)
DO_EVAL(MemberAccExprMIR, eval::ExprEvaluator)
DO_EVAL(SubscrExprMIR, eval::ExprEvaluator)
DO_EVAL(PostfixExprMIR, eval::ExprEvaluator)
DO_EVAL(SizeofExprMIR, eval::ExprEvaluator)

void CompoundStmtMIR::add_item(Box<ProgItemMIR> item) {
    items.push_back(std::move(item));
}

void ProgramMIR::add_item(Box<ProgItemMIR> item) {
    items.push_back(std::move(item));
}

bool InitializerMIR::is_all_literals() {
    return std::visit(
        match{
            [](Box<ExprMIR>& expr) { return expr->kind == MIRNode::NodeKind::LITEXPR_MIR; },
            [](Box<InitializerMIR::Member>& mem) { return mem->initializer->is_all_literals(); },
            [](Box<InitializerMIR::Index>& idx) { return idx->initializer->is_all_literals(); },
            [](Vec<Box<InitializerMIR>>& init) {
                return std::all_of(init.cbegin(), init.cend(), [](const Box<InitializerMIR>& init) {
                    return init->is_all_literals();
                });
            }},
        initializer);
}

void VarDeclMIR::add_decl(VarSymbol *sym) {
    decls.emplace_back(VarDecl{sym, {}});
}

void VarDeclMIR::add_decl(VarSymbol *sym, Box<InitializerMIR> init) {
    decls.emplace_back(VarDecl{sym, std::move(init)});
}
