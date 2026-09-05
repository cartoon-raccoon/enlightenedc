#include "semantics/mir/passes/constfold.hpp"

#include "ds/arenavec.hpp"
#include "semantics/mir/mir.hpp"
#include "semantics/types.hpp"

using namespace ecc::opti;
using namespace ecc::eval;
using namespace ecc::sema::mir;
using namespace ecc::ds;

Chunk<ExprMIR> ConstantFolder::eval_and_expr(Chunk<ExprMIR>& expr, Location loc) {

    if (isa<CondExprMIR>(expr)) {
        // A CondExpr only reaches here when its condition is constant-foldable
        // (that is the whole of CondExprMIR::is_const_foldable). Fold just the
        // condition and keep the branch it selects; the discarded branch does
        // not have to be foldable itself.
        auto *condexpr = dyncast<CondExprMIR>(expr);

        Value cond = condexpr->condition->eval(evalr);

        Chunk<ExprMIR>& taken =
            static_cast<bool>(cond) ? condexpr->true_expr : condexpr->false_expr;

        if (taken->is_const_foldable()) {
            return eval_and_expr(taken, taken->loc);
        }

        // Surviving branch is not constant: fold within it and hoist it up.
        taken->accept(*this);
        return std::move(taken);
    }

    Value val = expr->eval(evalr);

    // The folded literal must keep the type of the expression it replaces
    sema::types::Type *result_type = expr->eff_type;
    if (result_type != nullptr && result_type->is_primitive()) {
        val = val.pr_cast(result_type->as_primitive()->get_primkind());
    } else {
        result_type = types.get().get_primitive(val.primtype());
    }

    auto new_expr = make_chunk<LiteralExprMIR>(loc, syms.current, std::move(val));
    new_expr->set_type(result_type);

    return new_expr;
}

void ConstantFolder::fold_operand(Chunk<ExprMIR>& operand) {
    if (operand->is_const_foldable()) {
        operand = eval_and_expr(operand, operand->loc);
    } else {
        operand->accept(*this);
    }
}

void ConstantFolder::do_visit(InitializerMIR& node) {
    std::visit(
        match{
            [&](Chunk<ExprMIR>& expr) { fold_operand(expr); },
            [&](Chunk<InitializerMIR::Member>& member) { member->initializer->accept(*this); },
            [&](Chunk<InitializerMIR::Index>& index) { index->initializer->accept(*this); },
            [&](ArenaVec<Chunk<InitializerMIR>>& inits) {
                for (auto& init : inits) {
                    init->accept(*this);
                }
            }},
        node.initializer);
}

void ConstantFolder::do_visit(ExprStmtMIR& node) {
    if (node.expr) {
        fold_operand(*node.expr);
    }
}

void ConstantFolder::do_visit(ReturnStmtMIR& node) {
    if (node.ret_expr) {
        fold_operand(*node.ret_expr);
    }
}

void ConstantFolder::do_visit(IfStmtMIR& node) {
    fold_operand(node.condition);
    node.then_branch->accept(*this);
    if (node.else_branch) {
        (*node.else_branch)->accept(*this);
    }
}

void ConstantFolder::do_visit(LoopStmtMIR& node) {
    if (node.init) {
        (*node.init)->accept(*this);
    }
    if (node.condition) {
        fold_operand(*node.condition);
    }
    if (node.step) {
        (*node.step)->accept(*this);
    }
    node.body->accept(*this);
}

void ConstantFolder::do_visit(SwitchStmtMIR& node) {
    fold_operand(node.control_val);
    node.body->accept(*this);
}

void ConstantFolder::do_visit(PrintStmtMIR& node) {
    for (auto& arg : node.arguments) {
        fold_operand(arg);
    }
}

void ConstantFolder::do_visit(BinaryExprMIR& node) {
    fold_operand(node.left);
    fold_operand(node.right);
}

void ConstantFolder::do_visit(UnaryExprMIR& node) {
    fold_operand(node.operand);
}

void ConstantFolder::do_visit(CastExprMIR& node) {
    fold_operand(node.inner);
}

void ConstantFolder::do_visit(CondExprMIR& node) {
    // Reached only when the condition is not constant (a constant condition is
    // handled, with branch elimination, in eval_and_expr). Fold within each part
    // and keep the CondExpr.
    fold_operand(node.condition);
    fold_operand(node.true_expr);
    fold_operand(node.false_expr);
}

void ConstantFolder::do_visit(CallExprMIR& node) {
    node.callee->accept(*this);
    for (auto& arg : node.args) {
        fold_operand(arg);
    }
}

void ConstantFolder::do_visit(AssignExprMIR& node) {
    node.left->accept(*this);
    fold_operand(node.right);
}
