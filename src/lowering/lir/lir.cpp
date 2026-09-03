#include "lowering/lir/lir.hpp"

using namespace lower::lir;

Chunk<ExprLIR> BinaryExprLIR::clone_chunk() {
    return make_chunk<BinaryExprLIR>(
        loc.value_or(Location{}), act_type, left->clone_chunk(), right->clone_chunk(), op);
}

Chunk<ExprLIR> UnaryExprLIR::clone_chunk() {
    return make_chunk<UnaryExprLIR>(loc.value_or(Location{}), act_type, operand->clone_chunk(), op);
}

Chunk<ExprLIR> CastExprLIR::clone_chunk() {
    return make_chunk<CastExprLIR>(
        loc.value_or(Location{}), act_type, inner->clone_chunk(), target, castkind);
}

Chunk<ExprLIR> AssignExprLIR::clone_chunk() {
    return make_chunk<AssignExprLIR>(
        loc.value_or(Location{}), act_type, left->clone_chunk(), right->clone_chunk(), op);
}

Chunk<ExprLIR> CondExprLIR::clone_chunk() {
    return make_chunk<CondExprLIR>(
        loc.value_or(Location{}), act_type, condition->clone_chunk(), true_value->clone_chunk(),
        false_value->clone_chunk());
}

Chunk<ExprLIR> IdentExprLIR::clone_chunk() {
    return make_chunk<IdentExprLIR>(loc, sym, act_type);
}

Chunk<ExprLIR> LiteralExprLIR::clone_chunk() {
    return make_chunk<LiteralExprLIR>(loc, value, act_type);
}

Chunk<ExprLIR> ZeroExprLIR::clone_chunk() {
    return make_chunk<ZeroExprLIR>(loc.value_or(Location{}), act_type);
}

Chunk<ExprLIR> CallExprLIR::clone_chunk() {
    Vec<Chunk<ExprLIR>> cloned_args;
    cloned_args.reserve(args.size());
    for (auto& arg : args) {
        cloned_args.push_back(arg->clone_chunk());
    }

    return make_chunk<CallExprLIR>(
        loc.value_or(Location{}), callee->clone_chunk(), std::move(cloned_args), act_type);
}

Chunk<ExprLIR> MemberAccExprLIR::clone_chunk() {
    return make_chunk<MemberAccExprLIR>(loc, object->clone_chunk(), member_idx, act_type);
}

Chunk<ExprLIR> ReintExprLIR::clone_chunk() {
    return make_chunk<ReintExprLIR>(loc, object->clone_chunk(), target, act_type);
}

Chunk<ExprLIR> SubscrExprLIR::clone_chunk() {
    return make_chunk<SubscrExprLIR>(loc, array->clone_chunk(), index->clone_chunk(), act_type);
}

Chunk<ExprLIR> PostfixExprLIR::clone_chunk() {
    return make_chunk<PostfixExprLIR>(loc.value_or(Location{}), operand->clone_chunk(), op, act_type);
}
