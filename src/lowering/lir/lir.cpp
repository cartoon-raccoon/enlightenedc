#include "lowering/lir/lir.hpp"

using namespace lower::lir;

Box<ExprLIR> BinaryExprLIR::clone_box() {
    return make_box<BinaryExprLIR>(
        loc.value_or(Location{}), act_type, left->clone_box(), right->clone_box(), op);
}

Box<ExprLIR> UnaryExprLIR::clone_box() {
    return make_box<UnaryExprLIR>(loc.value_or(Location{}), act_type, operand->clone_box(), op);
}

Box<ExprLIR> CastExprLIR::clone_box() {
    return make_box<CastExprLIR>(
        loc.value_or(Location{}), act_type, inner->clone_box(), target, castkind);
}

Box<ExprLIR> AssignExprLIR::clone_box() {
    return make_box<AssignExprLIR>(
        loc.value_or(Location{}), act_type, left->clone_box(), right->clone_box(), op);
}

Box<ExprLIR> CondExprLIR::clone_box() {
    return make_box<CondExprLIR>(
        loc.value_or(Location{}), act_type, condition->clone_box(), true_value->clone_box(),
        false_value->clone_box());
}

Box<ExprLIR> IdentExprLIR::clone_box() {
    return make_box<IdentExprLIR>(loc, sym, act_type);
}

Box<ExprLIR> LiteralExprLIR::clone_box() {
    return make_box<LiteralExprLIR>(loc, value, act_type);
}

Box<ExprLIR> ZeroExprLIR::clone_box() {
    return make_box<ZeroExprLIR>(loc.value_or(Location{}), act_type);
}

Box<ExprLIR> CallExprLIR::clone_box() {
    Vec<Box<ExprLIR>> cloned_args;
    cloned_args.reserve(args.size());
    for (auto& arg : args) {
        cloned_args.push_back(arg->clone_box());
    }

    return make_box<CallExprLIR>(
        loc.value_or(Location{}), callee->clone_box(), std::move(cloned_args), act_type);
}

Box<ExprLIR> MemberAccExprLIR::clone_box() {
    return make_box<MemberAccExprLIR>(loc, object->clone_box(), member_idx, act_type);
}

Box<ExprLIR> ReintExprLIR::clone_box() {
    return make_box<ReintExprLIR>(loc, object->clone_box(), target, act_type);
}

Box<ExprLIR> SubscrExprLIR::clone_box() {
    return make_box<SubscrExprLIR>(loc, array->clone_box(), index->clone_box(), act_type);
}

Box<ExprLIR> PostfixExprLIR::clone_box() {
    return make_box<PostfixExprLIR>(loc.value_or(Location{}), operand->clone_box(), op, act_type);
}
