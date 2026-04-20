#include "semantics/mir/passes/constfold.hpp"

#include "semantics/mir/mir.hpp"

using namespace ecc::opti;
using namespace ecc::eval;
using namespace ecc::sema::mir;

Box<LiteralExprMIR> ConstantFolder::eval_and_expr(Box<ExprMIR>& expr, Location loc) {

    if (expr->kind == MIRNode::NodeKind::CONDEXPR_MIR) {
        // todo: implement checking for cond exprs
        // cast expr to CondExprMIR, eliminate branch
    }

    Value val     = expr->eval(evalr);
    auto new_expr = std::make_unique<LiteralExprMIR>(loc, syms.current, std::move(val));

    return new_expr;
}

void ConstantFolder::do_visit(sema::mir::InitializerMIR& node) {
    // todo
}

void ConstantFolder::do_visit(ExprStmtMIR& node) {
    if (!node.expr)
        return;

    if ((*node.expr)->is_const_foldable()) {
        node.expr = eval_and_expr(*node.expr, (*node.expr)->loc);
    } else {
        (*node.expr)->accept(*this);
    }
}

void ConstantFolder::do_visit(BinaryExprMIR& node) {
    if (node.left->is_const_foldable()) {
        node.left = eval_and_expr(node.left, node.left->loc);
    } else {
        node.left->accept(*this);
    }

    if (node.right->is_const_foldable()) {
        node.right = eval_and_expr(node.right, node.right->loc);
    } else {
        node.right->accept(*this);
    }
}

void ConstantFolder::do_visit(UnaryExprMIR& node) {
    if (node.operand->is_const_foldable()) {
        node.operand = eval_and_expr(node.operand, node.operand->loc);
    } else {
        node.operand->accept(*this);
    }
}

void ConstantFolder::do_visit(CastExprMIR& node) {
    if (node.inner->is_const_foldable()) {
        node.inner = eval_and_expr(node.inner, node.inner->loc);
    } else {
        node.inner->accept(*this);
    }
}

void ConstantFolder::do_visit(CondExprMIR& node) {
    // fixme:
    // if condition is foldable, but children are not, it can be still be
    // optimized by returning the branch that gets chosen every time
    if (node.condition->is_const_foldable()) {
        node.condition = eval_and_expr(node.condition, node.condition->loc);
    } else {
        node.condition->accept(*this);
    }

    if (node.true_expr->is_const_foldable()) {
        node.true_expr = eval_and_expr(node.true_expr, node.true_expr->loc);
    } else {
        node.true_expr->accept(*this);
    }

    if (node.false_expr->is_const_foldable()) {
        node.false_expr = eval_and_expr(node.false_expr, node.false_expr->loc);
    } else {
        node.false_expr->accept(*this);
    }
}