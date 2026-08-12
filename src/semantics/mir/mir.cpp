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

DO_EVAL(BinaryExprMIR, eval::ExprEvaluator);
DO_EVAL(UnaryExprMIR, eval::ExprEvaluator);
DO_EVAL(CastExprMIR, eval::ExprEvaluator);
DO_EVAL(AssignExprMIR, eval::ExprEvaluator);
DO_EVAL(CondExprMIR, eval::ExprEvaluator);
DO_EVAL(IdentExprMIR, eval::ExprEvaluator);
DO_EVAL(LiteralExprMIR, eval::ExprEvaluator);
DO_EVAL(CallExprMIR, eval::ExprEvaluator);
DO_EVAL(MemberAccExprMIR, eval::ExprEvaluator);
DO_EVAL(ReintExprMIR, eval::ExprEvaluator);
DO_EVAL(SubscrExprMIR, eval::ExprEvaluator);
DO_EVAL(PostfixExprMIR, eval::ExprEvaluator);
DO_EVAL(SizeofExprMIR, eval::ExprEvaluator);

void CompoundStmtMIR::add_item(Box<ProgItemMIR> item) {
    items.push_back(std::move(item));
}

void ProgramMIR::add_item(Box<ProgItemMIR> item) {
    items.push_back(std::move(item));
}

bool InitializerMIR::is_all_literals() {
    return std::visit(
        match{
            [&](Box<ExprMIR>& expr) { return expr->kind == MIRNode::NodeKind::LITEXPR_MIR; },
            [&](Box<InitializerMIR::Member>& mem) { return mem->initializer->is_all_literals(); },
            [&](Box<InitializerMIR::Index>& idx) { return idx->initializer->is_all_literals(); },
            [&](Vec<Box<InitializerMIR>>& init) {
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
