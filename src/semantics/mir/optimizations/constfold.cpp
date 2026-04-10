#include "semantics/mir/optimizations/constfold.hpp"

#include "semantics/mir/mir.hpp"

using namespace ecc::opti;
using namespace ecc::eval;
using namespace ecc::sema::mir;

Box<LiteralExprMIR> ConstantFolder::eval_and_expr(Box<ExprMIR>& expr, Location loc) {
    Value val     = expr->eval(evalr);
    auto new_expr = std::make_unique<LiteralExprMIR>(loc, syms.current, std::move(val));

    return std::move(new_expr);
}

void ConstantFolder::do_visit(ExprStmtMIR& node) {
    if (!node.expr)
        return;

    if ((*node.expr)->is_const_foldable()) {
        node.expr = eval_and_expr(*node.expr, (*node.expr)->loc);
    }
}

void ConstantFolder::do_visit(BinaryExprMIR& node) {
    if (node.left->is_const_foldable()) {
        node.left = eval_and_expr(node.left, node.left->loc);
    }

    if (node.right->is_const_foldable()) {
        node.right = eval_and_expr(node.right, node.right->loc);
    }
}

void ConstantFolder::do_visit(UnaryExprMIR& node) {
    if (node.operand->is_const_foldable()) {
        node.operand = eval_and_expr(node.operand, node.operand->loc);
    }
}

void ConstantFolder::do_visit(CastExprMIR& node) {
    if (node.inner->is_const_foldable()) {
        node.inner = eval_and_expr(node.inner, node.inner->loc);
    }
}

void ConstantFolder::do_visit(CondExprMIR& node) {
    if (node.condition->is_const_foldable()) {
        node.condition = eval_and_expr(node.condition, node.condition->loc);
    }

    if (node.true_expr->is_const_foldable()) {
        node.true_expr = eval_and_expr(node.true_expr, node.true_expr->loc);
    }

    if (node.false_expr->is_const_foldable()) {
        node.false_expr = eval_and_expr(node.false_expr, node.false_expr->loc);
    }
}

void ConstantFolder::do_visit(ConstExprMIR& node) {
    if (node.inner->is_const_foldable()) {
        node.inner = eval_and_expr(node.inner, node.inner->loc);
    }
}

void ConstantFolder::do_visit(PostfixExprMIR& node) {
    if (node.operand->is_const_foldable()) {
        node.operand = eval_and_expr(node.operand, node.operand->loc);
    }
}