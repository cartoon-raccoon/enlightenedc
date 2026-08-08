#include "codegen/cfg/builder.hpp"

#include <stdexcept>

#include "codegen/cfg/cfg.hpp"
#include "codegen/lir/lir.hpp"
#include "tokens.hpp"

using namespace codegen::lir;
using namespace codegen::cfg;
using namespace tokens;

void CFGBuilder::build_cfg(lir::ProgramLIR& prog) {
    prog.accept(*this);
}

NestedStmtInfo *CFGBuilder::find_info(const NestedStmtFilter& filter) {
    for (auto stmt = infostack.rbegin(); stmt != infostack.rend(); stmt++) {
        if (filter(stmt->get())) {
            return stmt->get();
        }
    }

    return nullptr;
}

Value *CFGBuilder::eval(ExprLIR& node) {
    node.accept(*this);
    Value *ret = last_value;
    last_value = nullptr; // poison the last value to prevent silent incorrectness
    return ret;
}

Value *CFGBuilder::eval_lvalue(ExprLIR& node) {
    if (auto *ident = dyncast<IdentExprLIR>(&node)) {
        return curr_func->lookup_alloca(ident->sym->as_varsym());
    }
    if (auto *member = dyncast<MemberAccExprLIR>(&node)) {
        Value *base =
            eval_lvalue(*member->object); // object is a pointer *value*, same as the rvalue path
        return curr_blk->add_instruction<MemberAccInst>(
            member->act_type, member->member_idx, base, *member->loc);
    }
    if (auto *subscr = dyncast<SubscrExprLIR>(&node)) {
        Value *index = eval(*subscr->index);
        Value *base  = eval_lvalue(*subscr->array);
        return curr_blk->add_instruction<SubscrInst>(
            subscr->act_type, index, base, *subscr->loc);
    }
    if (auto *unary = dyncast<UnaryExprLIR>(&node); unary && unary->op == tokens::UnaryOp::DEREF) {
        return eval_lvalue(*unary->operand); // *p's address is just p's value
    }

    throw std::runtime_error("invalid ExprLIR type for eval_lvalue");
}

static tokens::BinaryOp assign_op_to_binop(tokens::AssignOp op) {
    using namespace tokens;
    switch (op) {
    case AssignOp::MULEQ:
        return BinaryOp::MUL;
    case AssignOp::DIVEQ:
        return BinaryOp::DIV;
    case AssignOp::MODEQ:
        return BinaryOp::MOD;
    case AssignOp::PLUSEQ:
        return BinaryOp::PLUS;
    case AssignOp::MINUSEQ:
        return BinaryOp::MINUS;
    case AssignOp::LSHIFTEQ:
        return BinaryOp::LSHIFT;
    case AssignOp::RSHIFTEQ:
        return BinaryOp::RSHIFT;
    case AssignOp::ANDEQ:
        return BinaryOp::AND;
    case AssignOp::XOREQ:
        return BinaryOp::XOR;
    case AssignOp::OREQ:
        return BinaryOp::OR;
    default:
        throw std::runtime_error("got assign when mapping assignop to binop");
    }
}

void CFGBuilder::visit(ProgramLIR& node) {
    curr_func  = prog_cfg.implicit_main.get();
    curr_blk = curr_func->initialize();

    for (auto& item : node.progitems) {
        item->accept(*this);
    }

    for (auto& func : node.functions) {
        func->accept(*this);
    }
}

void CFGBuilder::visit(FunctionLIR& node) {
    curr_func  = prog_cfg.add_function(&node);
    curr_blk = curr_func->initialize();

    for (auto& local : node.locals) {
        local->accept(*this);
    }

    for (auto& item : node.body) {
        item->accept(*this);
    }
}

void CFGBuilder::visit(LabelDeclLIR& node) {
    BasicBlock *newblock = curr_func->create_block(node.mangled_label, true);

    if (!curr_blk->is_terminated()) {
        curr_blk->terminate<Goto>()->set_target(newblock);
    }

    curr_func->resolve_pending_gotos(node.mangled_label, newblock);
    curr_blk = newblock;
}

void CFGBuilder::visit(CaseLIR& node) {
    BasicBlock *caseblk = curr_func->create_block();
    if (!curr_blk->is_terminated()) {
        curr_blk->terminate<Goto>()->set_target(caseblk);
    }

    auto *info = find_info([&](NestedStmtInfo *info) { return info->is_switch(); });
    if (!info) {
        throw std::runtime_error(
            "unable to find switch NestedStmtInfo while visiting CaseLIR");
    }

    assert(info->is_switch());
    SwitchStmtInfo *swinfo = info->as_switch();

    swinfo->swtch->add_case(node.case_value, caseblk);
    curr_blk = caseblk;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

void CFGBuilder::visit(DefaultLIR& node) {
    BasicBlock *caseblk = curr_func->create_block();
    if (!curr_blk->is_terminated()) {
        curr_blk->terminate<Goto>()->set_target(caseblk);
    }

    auto *info = find_info([&](NestedStmtInfo *info) { return info->is_switch(); });
    if (!info) {
        throw std::runtime_error(
            "unable to find switch NestedStmtInfo while visiting DefaultLIR");
    }
    assert(info->is_switch());
    SwitchStmtInfo *swinfo = info->as_switch();

    swinfo->swtch->add_default(caseblk);
    curr_blk = caseblk;
}

#pragma clang diagnostic pop

void CFGBuilder::visit(ExprStmtLIR& node) {
    if (curr_blk->is_terminated()) {
        curr_blk = curr_func->create_block();
    }
    node.expr->accept(*this);
}

void CFGBuilder::visit(PrintStmtLIR& node) {
    if (curr_blk->is_terminated()) {
        curr_blk = curr_func->create_block();
    }

    Vec<Value *> args;
    for (auto& arg : node.args) {
        args.push_back(eval(*arg));
    }

    curr_blk->add_instruction<PrintInst>(
        types, node.format_string, std::move(args), *node.loc);
}

void CFGBuilder::visit(GotoStmtLIR& node) {
    if (curr_blk->is_terminated()) {
        curr_blk = curr_func->create_block();
    }

    Goto *g = curr_blk->terminate<Goto>();

    if (auto *targ = curr_func->lookup_labeled_block(node.mangled_target)) {
        g->set_target(targ);
    } else {
        curr_func->add_pending_goto(node.mangled_target, g);
    }
}

void CFGBuilder::visit(SwitchStmtLIR& node) {
    if (curr_blk->is_terminated()) {
        curr_blk = curr_func->create_block();
    }
    Value *control = eval(*node.condition);
    /*
    1. check if current block was terminated
        a. if so, create a new block and set it as current
    2. terminate current block with Switch, control as eval(node.condition)
    3. push SwitchStmtInfo
    4. 
    */
    for (auto& item : node.body) {
        item->accept(*this);
    }
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

void CFGBuilder::visit(BreakStmtLIR& node) {
    if (curr_blk->is_terminated()) {
        curr_blk = curr_func->create_block();
    }

    auto *info =
        find_info([&](NestedStmtInfo *info) { return info->is_loop() || info->is_switch(); });

    if (!info) {
        throw std::runtime_error(
            "unable to find loop or switch NestedStmtInfo while visiting BreakStmtLIR");
    }

    Goto *g = curr_blk->terminate<Goto>();
    info->pending_merges.push_back(g);
}

void CFGBuilder::visit(ContStmtLIR& node) {
    if (curr_blk->is_terminated()) {
        curr_blk = curr_func->create_block();
    }

    auto *info = find_info([&](NestedStmtInfo *info) { return info->is_loop(); });

    if (!info) {
        throw std::runtime_error("unable to find loop NestedStmtInfo while visiting ContStmtLIR");
    }

    assert(info->is_loop());
    auto *loopinfo = info->as_loop();

    assert(loopinfo->step && "no loopinfo.step when visiting ContStmtLIR");

    curr_blk->terminate<Goto>()->set_target(loopinfo->step);
}

#pragma clang diagnostic pop

void CFGBuilder::visit(IfStmtLIR& node) {
    // if current block is already terminated, create a new block and set as current
    if (curr_blk->is_terminated()) {
        curr_blk = curr_func->create_block();
    }

    // evaluate the condition
    Value *cond = eval(*node.condition);

    // terminate the current block with the if
    If *term = curr_blk->terminate<If>(cond);

    // create the then branch, set term then target accordingly
    BasicBlock *then_blk = curr_func->create_block();
    term->set_then_target(then_blk);

    // visit the then branch
    curr_blk = then_blk;
    for (auto& item : node.then_br) {
        item->accept(*this);
    }
    BasicBlock *then_exit = curr_blk;

    // else branch exit point; remains null if no else branch exists
    BasicBlock *else_exit = nullptr;

    // create the else block, if an else branch exists
    if (node.else_br) {
        BasicBlock *else_blk = curr_func->create_block();
        term->set_else_target(else_blk);

        curr_blk = else_blk;
        for (auto& item : *node.else_br) {
            item->accept(*this);
        }
        else_exit = curr_blk;
    }

    // create the merge block and link it to the then branch exit point
    BasicBlock *merge_blk = curr_func->create_block();
    if (!then_exit->is_terminated()) {
        then_exit->terminate<Goto>()->set_target(merge_blk);
    }

    if (else_exit) {
        if (!else_exit->is_terminated()) {
            else_exit->terminate<Goto>()->set_target(merge_blk);
        }
    } else {
        assert(term->else_br == nullptr);
        term->set_else_target(merge_blk);
    }

    curr_blk = merge_blk;
}

void CFGBuilder::visit(LoopStmtLIR& node) {
    /*
    1. check if current block is terminated
       a. if so, create new block and set it as current
    

    */
}

void CFGBuilder::visit(ReturnStmtLIR& node) {

    if (curr_blk->is_terminated()) {
        curr_blk = curr_func->create_block();
    }

    if (node.ret_value) {
        Value *ret = eval(**node.ret_value);
        curr_blk->terminate<Return>(ret);
    } else {
        curr_blk->terminate<Return>();
    }
}

void CFGBuilder::visit(VarDeclLIR& node) {
    curr_func->add_alloca(curr_blk, node.lirsym);
}

void CFGBuilder::visit(BinaryExprLIR& node) {
    switch (node.op) {
    case BinaryOp::ANDAND:
    case BinaryOp::OROR: {
        bool is_and = node.op == BinaryOp::ANDAND;

        Value *lhs = eval(*node.left);
        BasicBlock *lhs_exit =
            curr_blk; // capture *after eval*, since left may branch by itself
        BasicBlock *rhs_block   = curr_func->create_block();
        BasicBlock *merge_block = curr_func->create_block();

        If *if_term = lhs_exit->terminate<If>(lhs);
        if (is_and) {
            // a && b: only evaluate b if a was true, false short-circuits

            // if lhs is true, we run the rhs
            if_term->set_then_target(rhs_block);

            // otherwise, we merge (short-circuit)
            if_term->set_else_target(merge_block);
        } else {
            // a || b: only evaluate b is a was false, true short-circuits

            // if lhs is true, we merge (short-circuit)
            if_term->set_then_target(merge_block);

            // otherwise, run the rhs
            if_term->set_else_target(rhs_block);
        }

        // evaluate the RHS, set the target of the rhs block to the merge block
        curr_blk        = rhs_block;
        Value *rhs           = eval(*node.right);
        BasicBlock *rhs_exit = curr_blk;

        rhs_exit->terminate<Goto>()->set_target(merge_block);

        // on the merge block, add a phi with all the incoming edges
        curr_blk = merge_block;
        PhiInst *phi  = curr_blk->add_instruction<PhiInst>(node.act_type, *node.loc);

        // create the short circuit value
        Value *short_circuit_val =
            curr_blk->add_value<Literal>(node.act_type, eval::Value(!is_and));

        phi->add_incoming(short_circuit_val, lhs_exit);
        phi->add_incoming(rhs, rhs_exit);
        last_value = phi;

        break;
    }

    case BinaryOp::BINCOMMA: {
        todo();
        break;
    }

    default: {
        Value *left  = eval(*node.left);
        Value *right = eval(*node.right);

        last_value = curr_blk->add_instruction<BinaryInst>(
            node.act_type, node.op, left, right, *node.loc);
    }
    }

    assert(last_value && "last_value is nullptr at end of expr visit");
}

void CFGBuilder::visit(UnaryExprLIR& node) {
    switch (node.op) {
    case UnaryOp::INC:
    case UnaryOp::DEC: {
        Value *address = eval_lvalue(*node.operand);
        Value *cur = curr_blk->add_instruction<LoadInst>(node.act_type, address, *node.loc);

        Value *updated;
        if (node.op == UnaryOp::INC) {
            updated = curr_blk->add_instruction<IncrInst>(node.act_type, cur, *node.loc);
        } else {
            updated = curr_blk->add_instruction<DecrInst>(node.act_type, cur, *node.loc);
        }

        curr_blk->add_instruction<StoreInst>(types, address, updated, *node.loc);
        last_value = updated;
        break;
    }
    default: {
        Value *operand = eval(*node.operand);
        last_value =
            curr_blk->add_instruction<UnaryInst>(node.act_type, node.op, operand, *node.loc);
    }
    }
    assert(last_value && "last_value is nullptr at end of expr visit");
}

void CFGBuilder::visit(CastExprLIR& node) {
    Value *operand = eval(*node.inner);

    last_value =
        curr_blk->add_instruction<CastInst>(node.act_type, node.target, operand, *node.loc);

    assert(last_value && "last_value is nullptr at end of expr visit");
}

void CFGBuilder::visit(AssignExprLIR& node) {
    Value *lvalue = eval_lvalue(*node.left);
    Value *right  = eval(*node.right);

    Value *to_store = right;
    if (node.op != AssignOp::ASSIGN) {
        Value *cur = curr_blk->add_instruction<LoadInst>(node.act_type, lvalue, *node.loc);
        to_store   = curr_blk->add_instruction<BinaryInst>(
            node.act_type, assign_op_to_binop(node.op), cur, right, *node.loc);
    }

    curr_blk->add_instruction<StoreInst>(types, lvalue, to_store, *node.loc);
    last_value = to_store;
}

void CFGBuilder::visit(CondExprLIR& node) {
    // evaluate the condition
    Value *cond = eval(*node.condition);

    // create our blocks
    BasicBlock *true_blk  = curr_func->create_block();
    BasicBlock *false_blk = curr_func->create_block();
    BasicBlock *merge_blk = curr_func->create_block();

    // terminate the current block with an if, set targets
    If *branch = curr_blk->terminate<If>(cond);
    branch->set_then_target(true_blk);
    branch->set_else_target(false_blk);

    // evaluate the true block
    curr_blk         = true_blk;
    Value *true_val       = eval(*node.true_value);
    BasicBlock *true_exit = curr_blk;
    true_exit->terminate<Goto>()->set_target(merge_blk);

    // evaluate the false block
    curr_blk          = false_blk;
    Value *false_val       = eval(*node.false_value);
    BasicBlock *false_exit = curr_blk;
    false_exit->terminate<Goto>()->set_target(merge_blk);

    curr_blk = merge_blk;
    PhiInst *phi  = curr_blk->add_instruction<PhiInst>(node.act_type, *node.loc);

    phi->add_incoming(true_val, true_exit);
    phi->add_incoming(false_val, false_exit);

    last_value = phi;

    assert(last_value && "last_value is nullptr at end of expr visit");
}

void CFGBuilder::visit(IdentExprLIR& node) {
    if (node.sym->is_var()) {
        last_value = curr_blk->add_instruction<LoadInst>(
            node.act_type, curr_func->lookup_alloca(node.sym->as_varsym()), *node.loc);
    } else if (node.sym->is_func()) {
        last_value = prog_cfg.ref_function(node.sym->as_funcsym()->lir);
    }

    assert(last_value && "last_value is nullptr at end of expr visit");
}

void CFGBuilder::visit(LiteralExprLIR& node) {
    last_value = curr_blk->add_value<Literal>(node.act_type, node.value);

    assert(last_value && "last_value is nullptr at end of expr visit");
}

void CFGBuilder::visit(ZeroExprLIR& node) {
    last_value = curr_blk->add_value<Zero>(node.act_type);

    assert(last_value && "last_value is nullptr at end of expr visit");
}

void CFGBuilder::visit(CallExprLIR& node) {
    Vec<Value *> args;
    for (auto& arg : node.args) {
        args.push_back(eval(*arg));
    }
    last_value = curr_blk->add_instruction<CallInst>(
        node.act_type, eval(*node.callee), std::move(args), *node.loc);

    assert(last_value && "last_value is nullptr at end of expr visit");
}

void CFGBuilder::visit(MemberAccExprLIR& node) {
    last_value = eval_lvalue(node);

    assert(last_value && "last_value is nullptr at end of expr visit");
}

void CFGBuilder::visit(ReintExprLIR& node) {
    Value *operand = eval(*node.object);

    last_value =
        curr_blk->add_instruction<ReintInst>(node.act_type, node.target, operand, *node.loc);

    assert(last_value && "last_value is nullptr at end of expr visit");
}

void CFGBuilder::visit(SubscrExprLIR& node) {
    last_value = eval_lvalue(node);

    assert(last_value && "last_value is nullptr at end of expr visit");
}

void CFGBuilder::visit(PostfixExprLIR& node) {
    Value *address = eval_lvalue(*node.operand);
    Value *old_val = curr_blk->add_instruction<LoadInst>(node.act_type, address, *node.loc);

    Value *new_val;
    if (node.op == PostfixOp::POSTINC) {
        new_val = curr_blk->add_instruction<IncrInst>(node.act_type, old_val, *node.loc);
    } else {
        new_val = curr_blk->add_instruction<DecrInst>(node.act_type, old_val, *node.loc);
    }

    curr_blk->add_instruction<StoreInst>(types, address, new_val, *node.loc);
    last_value = old_val; // postfix yields the *old* value
}