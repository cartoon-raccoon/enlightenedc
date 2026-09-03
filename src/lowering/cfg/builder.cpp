#include "lowering/cfg/builder.hpp"

#include <ranges>
#include <stdexcept>

#include "lowering/cfg/cfg.hpp"
#include "lowering/lir/lir.hpp"
#include "lowering/lir/symbols.hpp"
#include "tokens.hpp"

using namespace lower::lir;
using namespace lower::cfg;
using namespace sema::types;
using namespace tokens;

const char *const IMPLICIT_MAIN_NAME = "__ec_implicit_main";

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

        Value *ret = nullptr;
        if (ident->sym->is_var()) {
            if (auto *alloc = lookup_alloca(ident->sym->as_varsym())) {
                ret = alloc;
            } else {
                ret = lookup_global(ident->sym->as_varsym());
            }
        } else {
            ret = add_or_get_function(ident->sym->as_funcsym()->lir);
        }

        assert(ret);

        return ret;
    }
    if (auto *member = dyncast<MemberAccExprLIR>(&node)) {
        Value *base =
            materialize(*member->object); // object is a pointer *value*, same as the rvalue path
        if (member->object->act_type->is_union()) {
            // if object is a union, return the base directly
            return base;
        } else {
            return curr_blk->add_instruction<MemberAccInst>(
                member->act_type, member->member_idx, base, member->loc);
        }
    }
    if (auto *reint = dyncast<ReintExprLIR>(&node)) {
        Value *base = materialize(*reint->object);
        return base;
    }
    if (auto *subscr = dyncast<SubscrExprLIR>(&node)) {
        Value *index = eval(*subscr->index);
        Value *base  = materialize(*subscr->array);
        return curr_blk->add_instruction<SubscrInst>(subscr->act_type, index, base, subscr->loc);
    }
    if (auto *unary = dyncast<UnaryExprLIR>(&node); unary && unary->op == tokens::UnaryOp::DEREF) {
        return eval(*unary->operand); // *p's address is just p's value
    }
    if (auto *literal = dyncast<LiteralExprLIR>(&node); literal && literal->is_str()) {
        auto& str = std::get<std::string>(literal->value);
        assert(node.act_type->is_array());
        String *ret = prog_cfg.add_or_get_string(node.act_type->as_array(), str);
        ret->set_type(literal->act_type);

        return ret;
    }

    return nullptr;
}

Value *CFGBuilder::materialize(ExprLIR& node) {
    if (auto *val = eval_lvalue(node); val) {
        return val;
    }
    return spill_to_temp(eval(node), node.act_type, node.loc);
}

Value *CFGBuilder::spill_to_temp(Value *val, Type *type, Optional<Location> loc) {
    Alloca *alloca = curr_func->add_alloca(type->unqual());
    curr_blk->add_instruction<StoreInst>(types, alloca, val, loc);

    return alloca;
}

Global *CFGBuilder::add_or_get_global(LIRVarSym *sym, Value *init) {
    if (globals.contains(sym))
        return globals[sym];

    Global *ret  = prog_cfg.add_global(sym->get_type(), sym->symdata->get_mangled_name(), init);
    globals[sym] = ret;

    return ret;
}

Global *CFGBuilder::lookup_global(LIRVarSym *sym) {
    return globals.contains(sym) ? globals[sym] : nullptr;
}

FunctionCFG *CFGBuilder::add_or_get_function(lir::FunctionLIR *func) {
    if (functions.contains(func))
        return functions[func];

    FunctionCFG *ret = prog_cfg.add_function(
        func->funcsym->get_symdata()->get_signature(), func->funcsym->symdata->get_mangled_name());
    functions[func] = ret;

    return ret;
}

FunctionCFG *CFGBuilder::lookup_function(lir::FunctionLIR *func) {
    return functions.contains(func) ? functions[func] : nullptr;
}

Alloca *CFGBuilder::add_or_get_alloca(lir::LIRVarSym *sym) {
    if (allocas.contains(sym)) {
        return allocas[sym];
    }

    Alloca *ret  = curr_func->add_alloca(sym->get_type(), sym->symdata->get_mangled_name());
    allocas[sym] = ret;

    return ret;
}

Alloca *CFGBuilder::lookup_alloca(lir::LIRVarSym *sym) {
    return allocas.contains(sym) ? allocas[sym] : nullptr;
}

void CFGBuilder::add_pending_goto(std::string& label, Goto *g) {
    if (pending_gotos.contains(label)) {
        pending_gotos[label].push_back(g);
    } else {
        pending_gotos[label] = {};
        pending_gotos[label].push_back(g);
    }
}

size_t CFGBuilder::resolve_pending_gotos(std::string& label, BasicBlock *target) {
    auto it = pending_gotos.find(label);
    if (it == pending_gotos.end()) {
        return 0;
    }

    for (Goto *g : it->second) {
        g->set_target(target);
    }

    size_t count = it->second.size();
    pending_gotos.erase(it);
    return count;
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
    dbprint("visiting ProgramLIR node ", node.loc ? *node.loc : Location{});

    FunctionType *implicit_main_sig = types.get_function({}, types.get_void(), {}, false);
    curr_func                       = prog_cfg.add_function(implicit_main_sig, IMPLICIT_MAIN_NAME);
    curr_func->initialize();

    for (auto& item : node.globals) {
        if (item->init) {
            Constant *init = build_constant(*item->init);
            add_or_get_global(item->lirsym, init);
        } else {
            add_or_get_global(item->lirsym);
        }
    }

    curr_blk = curr_func->initialize();

    for (auto& item : node.progitems) {
        item->accept(*this);
    }

    for (auto& func : node.functions) {
        func->accept(*this);
    }
}

void CFGBuilder::visit(FunctionLIR& node) {
    dbprint("visiting FunctionLIR node ", node.loc ? *node.loc : Location{});

    curr_func = add_or_get_function(&node);

    if (!node.has_definition) {
        return;
    }

    curr_blk = curr_func->initialize();

    // emit instructions for allocating and copying in the parameters
    for (auto [idx, param] : std::views::enumerate(node.funcsym->params)) {
        auto *addr = add_or_get_alloca(param);

        auto *value = curr_func->arg_idx(idx);
        assert(value && "got null arg");

        curr_blk->add_instruction<StoreInst>(types, addr, value);
    }

    for (auto& local : node.locals) {
        local->accept(*this);
    }

    for (auto& item : node.body) {
        item->accept(*this);
    }
}

void CFGBuilder::visit(LabelDeclLIR& node) {
    dbprint("visiting LabelDeclLIR node ", node.loc ? *node.loc : Location{});

    BasicBlock *newblock = curr_func->create_block(node.mangled_label, true);

    if (!curr_blk->is_terminated()) {
        curr_blk->terminate<Goto>()->set_target(newblock);
    }

    resolve_pending_gotos(node.mangled_label, newblock);
    assert(num_pending_gotos() == 0);
    curr_blk = newblock;
}

void CFGBuilder::visit(CaseLIR& node) {
    dbprint("visiting CaseLIR node ", node.loc ? *node.loc : Location{});

    BasicBlock *caseblk = curr_func->create_block();
    if (!curr_blk->is_terminated()) {
        curr_blk->terminate<Goto>()->set_target(caseblk);
    }

    auto *info = find_info([&](NestedStmtInfo *info) { return info->is_switch(); });
    if (!info) {
        throw std::runtime_error("unable to find switch NestedStmtInfo while visiting CaseLIR");
    }

    assert(info->is_switch());
    SwitchStmtInfo *swinfo = info->as_switch();

    swinfo->swtch->add_case(node.case_value, caseblk);
    curr_blk = caseblk;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

void CFGBuilder::visit(DefaultLIR& node) {
    dbprint("visiting DefaultLIR node ", node.loc ? *node.loc : Location{});

    BasicBlock *caseblk = curr_func->create_block();
    if (!curr_blk->is_terminated()) {
        curr_blk->terminate<Goto>()->set_target(caseblk);
    }

    auto *info = find_info([&](NestedStmtInfo *info) { return info->is_switch(); });
    if (!info) {
        throw std::runtime_error("unable to find switch NestedStmtInfo while visiting DefaultLIR");
    }
    assert(info->is_switch());
    SwitchStmtInfo *swinfo = info->as_switch();

    swinfo->swtch->add_default(caseblk);
    curr_blk = caseblk;
}

#pragma clang diagnostic pop

void CFGBuilder::visit(ExprStmtLIR& node) {
    dbprint("visiting ExprStmtLIR node ", node.loc ? *node.loc : Location{});

    if (curr_blk->is_terminated()) {
        curr_blk = curr_func->create_block();
    }
    node.expr->accept(*this);
}

void CFGBuilder::visit(MemcpyLIR& node) {
    dbprint("visiting MemcpyLIR node ", node.loc ? *node.loc : Location{});

    if (curr_blk->is_terminated()) {
        curr_blk = curr_func->create_block();
    }
    Value *to   = eval_lvalue(*node.to);
    Value *from = eval_lvalue(*node.from);

    assert(to && from);

    curr_blk->add_instruction<MemcpyInst>(types, to, from, node.n);
}

void CFGBuilder::visit(PrintStmtLIR& node) {
    dbprint("visiting PrintStmtLIR node ", node.loc ? *node.loc : Location{});

    if (curr_blk->is_terminated()) {
        curr_blk = curr_func->create_block();
    }

    ArrayType *str_type = types.get_array(types.get_i8(), node.format_string.size() + 1);
    String *format      = prog_cfg.add_or_get_string(str_type, node.format_string);

    Vec<Value *> args;
    for (auto& arg : node.args) {
        args.push_back(eval(*arg));
    }

    curr_blk->add_instruction<PrintInst>(types, format, std::move(args), node.loc);
}

void CFGBuilder::visit(GotoStmtLIR& node) {
    dbprint("visiting GotoStmtLIR node ", node.loc ? *node.loc : Location{});

    if (curr_blk->is_terminated()) {
        curr_blk = curr_func->create_block();
    }

    Goto *g = curr_blk->terminate<Goto>();

    if (auto *targ = curr_func->lookup_labeled_block(node.mangled_target)) {
        g->set_target(targ);
    } else {
        add_pending_goto(node.mangled_target, g);
    }
}

void CFGBuilder::visit(SwitchStmtLIR& node) {
    dbprint("visiting SwitchStmtLIR node ", node.loc ? *node.loc : Location{});

    if (curr_blk->is_terminated()) {
        curr_blk = curr_func->create_block();
    }
    Value *control = eval(*node.condition);

    assert(!curr_blk->is_terminated());
    Switch *swtch        = curr_blk->terminate<Switch>(control);
    SwitchStmtInfo *info = push_info<SwitchStmtInfo>(swtch)->as_switch();

    for (auto& item : node.body) {
        item->accept(*this);
    }

    BasicBlock *merge = curr_func->create_block();

    for (auto *g : info->pending_merges) {
        g->set_target(merge);
    }

    pop_info();

    curr_blk = merge;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

void CFGBuilder::visit(BreakStmtLIR& node) {
    dbprint("visiting BreakStmtLIR node ", node.loc ? *node.loc : Location{});

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
    dbprint("visiting ContStmtLIR node ", node.loc ? *node.loc : Location{});

    if (curr_blk->is_terminated()) {
        curr_blk = curr_func->create_block();
    }

    auto *info = find_info([&](NestedStmtInfo *info) { return info->is_loop(); });

    if (!info) {
        throw std::runtime_error("unable to find loop NestedStmtInfo while visiting ContStmtLIR");
    }

    assert(info->is_loop());
    auto *loopinfo = info->as_loop();

    // Try, in order, step, cond, and then body to link to
    if (loopinfo->step) {
        curr_blk->terminate<Goto>()->set_target(loopinfo->step);
    } else if (loopinfo->cond) {
        curr_blk->terminate<Goto>()->set_target(loopinfo->cond);
    } else if (loopinfo->body) {
        curr_blk->terminate<Goto>()->set_target(loopinfo->body);
    } else {
        throw std::runtime_error("no loop construct to link to when visiting contstmt");
    }
}

#pragma clang diagnostic pop

void CFGBuilder::visit(IfStmtLIR& node) {
    dbprint("visiting IfStmtLIR node ", node.loc ? *node.loc : Location{});

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
    dbprint("visiting LoopStmtLIR node ", node.loc ? *node.loc : Location{});

    if (curr_blk->is_terminated()) {
        curr_blk = curr_func->create_block();
    }
    auto *info = push_info<LoopStmtInfo>()->as_loop();

    Goto *entry           = curr_blk->terminate<Goto>();
    BasicBlock *init_blk  = nullptr;
    BasicBlock *init_exit = nullptr;

    BasicBlock *cond_blk  = nullptr;
    BasicBlock *cond_exit = nullptr;
    If *condition         = nullptr;

    BasicBlock *body_blk  = nullptr;
    BasicBlock *body_exit = nullptr;

    BasicBlock *step_blk  = nullptr;
    BasicBlock *step_exit = nullptr;

    // Create the blocks

    if (node.init) {
        init_blk = curr_func->create_block();

        curr_blk = init_blk;
        for (auto& item : *node.init) {
            item->accept(*this);
        }
        init_exit = curr_blk;
    }

    if (node.condition) {
        cond_blk   = curr_func->create_block();
        info->cond = cond_blk;

        curr_blk    = cond_blk;
        Value *cond = eval(**node.condition);
        cond_exit   = curr_blk;
        condition   = curr_blk->terminate<If>(cond);
    }

    if (condition) {
        assert(cond_blk && cond_exit && "no cond blk with non-null condition");
    }

    if (node.step) {
        step_blk   = curr_func->create_block();
        info->step = step_blk;
        curr_blk   = step_blk;
        for (auto& item : *node.step) {
            item->accept(*this);
        }
        step_exit = curr_blk;
    }

    body_blk   = step_blk ? curr_func->create_block_before(step_blk) : curr_func->create_block();
    info->body = body_blk;
    curr_blk   = body_blk;
    for (auto& item : node.body) {
        item->accept(*this);
    }
    body_exit = curr_blk;

    BasicBlock *merge = curr_func->create_block();

    // Wire up the blocks

    if (init_blk) {
        // If there is an init block, set the entry target to that
        entry->set_target(init_blk);
        if (!init_exit->is_terminated()) {
            init_exit->terminate<Goto>()->set_target(cond_blk ? cond_blk : body_blk);
        }
    } else if (node.is_dowhile || !cond_blk) {
        // Else, if the loop is dowhile or there is no condition, set the entry
        // directly to the body block
        entry->set_target(body_blk);
    } else {
        // All other cases, set the entry to the condition block
        entry->set_target(cond_blk);
    }

    if (node.is_dowhile) {
        // If the loop is dowhile, the body always runs first; the condition
        // (which always exists for do-while) then decides whether to loop back.
        if (!body_exit->is_terminated()) {
            body_exit->terminate<Goto>()->set_target(cond_blk);
        }
        condition->set_then_target(body_blk);
    } else {
        // Every non-dowhile shape with a condition takes the same then-target:
        // the body always runs next -- never the step.
        if (condition) {
            condition->set_then_target(body_blk);
        }

        // What runs after the body: recheck the condition if there is one,
        // otherwise loop straight back to the body. Step, if present, always
        // lands on this same target afterwards.
        BasicBlock *backedge_target = cond_blk ? cond_blk : body_blk;

        if (step_blk) {
            if (!body_exit->is_terminated()) {
                body_exit->terminate<Goto>()->set_target(step_blk);
            }
            if (!step_exit->is_terminated()) {
                step_exit->terminate<Goto>()->set_target(backedge_target);
            }
        } else if (!body_exit->is_terminated()) {
            body_exit->terminate<Goto>()->set_target(backedge_target);
        }
    }

    // In all cases, if the condition fails, exit to the merge block;
    // loops with no condition (`for(;;)`) can only be left via break/return/goto.
    if (condition) {
        condition->set_else_target(merge);
    }

    for (auto *g : info->pending_merges) {
        g->set_target(merge);
    }

    pop_info();

    curr_blk = merge;
}

void CFGBuilder::visit(ReturnStmtLIR& node) {
    dbprint("visiting ReturnStmtLIR node ", node.loc ? *node.loc : Location{});

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
    dbprint("visiting VarDeclLIR node ", node.loc ? *node.loc : Location{});

    // Parameters are hoisted into FunctionLIR::locals alongside real locals (both flow
    // through the same synthesis queue), but visit(FunctionLIR&) already allocas + stores
    // every parameter explicitly before walking locals. Reprocessing one here would
    // allocate a second Alloca for the same symbol and clobber the first, leaving the
    // parameter's incoming-argument store pointing at a freed instruction.
    if (node.lirsym->is_param) {
        return;
    }

    Alloca *addr = add_or_get_alloca(node.lirsym);
    if (node.init) {
        Value *store_operand = build_constant(*node.init);
        curr_blk->add_instruction<StoreInst>(types, addr, store_operand, node.loc);
    }
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

void CFGBuilder::visit(ScalarInitLIR& node) {
    throw std::runtime_error("visit ScalarInitLIR called");
}

void CFGBuilder::visit(PointerInitLIR& node) {
    throw std::runtime_error("visit PointerInitLIR called");
}

void CFGBuilder::visit(AggregateInitLIR& node) {
    throw std::runtime_error("visit AggregateInitLIR called");
}

void CFGBuilder::visit(StringInitLIR& node) {
    throw std::runtime_error("visit StringInitLIR called");
}

void CFGBuilder::visit(FuncInitLIR& node) {
    throw std::runtime_error("visit FuncInitLIR called");
}

void CFGBuilder::visit(ZeroInitLIR& node) {
    throw std::runtime_error("visit ZeroInitLIR called");
}

#pragma clang diagnostic pop

void CFGBuilder::visit(BinaryExprLIR& node) {
    dbprint("visiting BinaryExprLIR node ", node.loc ? *node.loc : Location{});

    switch (node.op) {
    case BinaryOp::ANDAND:
    case BinaryOp::OROR: {
        bool is_and = node.op == BinaryOp::ANDAND;

        Value *lhs              = eval(*node.left);
        BasicBlock *lhs_exit    = curr_blk; // capture *after eval*, since left may branch by itself
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
        curr_blk             = rhs_block;
        Value *rhs           = eval(*node.right);
        BasicBlock *rhs_exit = curr_blk;

        rhs_exit->terminate<Goto>()->set_target(merge_block);

        // on the merge block, add a phi with all the incoming edges
        curr_blk     = merge_block;
        PhiInst *phi = curr_blk->add_instruction<PhiInst>(node.act_type, node.loc);

        // create the short circuit value
        Value *short_circuit_val =
            curr_blk->add_value<ScalarConst>(node.act_type->as_primitive(), eval::Value(!is_and));

        phi->add_incoming(short_circuit_val, lhs_exit);
        phi->add_incoming(rhs, rhs_exit);
        last_value = phi;

        break;
    }

    case BinaryOp::BINCOMMA: {
        // a, b: evaluate a for side effects and discard it, result is b
        eval(*node.left);
        last_value = eval(*node.right);
        break;
    }

    default: {
        Value *left  = eval(*node.left);
        Value *right = eval(*node.right);

        BinaryInst::Operator op = BinaryInst::op_from_token(node.op);
        last_value =
            curr_blk->add_instruction<BinaryInst>(node.act_type, op, left, right, node.loc);
    }
    }

    assert(last_value && "last_value is nullptr at end of expr visit");
}

void CFGBuilder::visit(UnaryExprLIR& node) {
    dbprint("visiting UnaryExprLIR node ", node.loc ? *node.loc : Location{});

    switch (node.op) {
    case UnaryOp::INC:
    case UnaryOp::DEC: {
        Value *address = eval_lvalue(*node.operand);
        Value *cur     = curr_blk->add_instruction<LoadInst>(node.act_type, address, node.loc);

        Value *updated;
        if (node.op == UnaryOp::INC) {
            updated = curr_blk->add_instruction<IncrInst>(node.act_type, cur, node.loc);
        } else {
            updated = curr_blk->add_instruction<DecrInst>(node.act_type, cur, node.loc);
        }

        curr_blk->add_instruction<StoreInst>(types, address, updated, node.loc);
        last_value = updated;
        break;
    }
    case UnaryOp::REF:
        last_value = eval_lvalue(*node.operand);
        break;
    case UnaryOp::DEREF: {
        Value *address = eval(*node.operand);
        last_value     = curr_blk->add_instruction<LoadInst>(node.act_type, address, node.loc);
        break;
    }
    default: {
        Value *operand         = eval(*node.operand);
        UnaryInst::Operator op = UnaryInst::op_from_token(node.op);
        last_value = curr_blk->add_instruction<UnaryInst>(node.act_type, op, operand, node.loc);
    }
    }
    assert(last_value && "last_value is nullptr at end of expr visit");
}

void CFGBuilder::visit(CastExprLIR& node) {
    dbprint("visiting CastExprLIR node ", node.loc ? *node.loc : Location{});

    using CK = CastExprLIR::CastKind;
    switch (node.castkind) {
    case CK::Explicit:
    case CK::Implicit: {
        dbprint("    rvalue cast, evaluating on rvalue eval");
        Value *operand = eval(*node.inner);
        last_value =
            curr_blk->add_instruction<CastInst>(node.act_type, node.target, operand, node.loc);
    } break;

    case CK::ArrPtrDecay: {
        dbprint("    array-pointer decay, evaluating on lvalue eval");
        last_value = eval_lvalue(*node.inner);
    } break;
    case CK::FuncPtrDecay: {
        dbprint("    function-pointer decay, evaluating on rvalue eval");
        last_value = eval(*node.inner);
    } break;
    }

    assert(last_value && "last_value is nullptr at end of expr visit");
}

void CFGBuilder::visit(AssignExprLIR& node) {
    dbprint("visiting AssignExprLIR node ", node.loc ? *node.loc : Location{});

    Value *lvalue = eval_lvalue(*node.left);
    Value *right  = eval(*node.right);

    Value *to_store = right;
    if (node.op != AssignOp::ASSIGN) {
        Value *cur = curr_blk->add_instruction<LoadInst>(node.act_type, lvalue, node.loc);
        BinaryInst::Operator op = BinaryInst::op_from_token(assign_op_to_binop(node.op));
        to_store = curr_blk->add_instruction<BinaryInst>(node.act_type, op, cur, right, node.loc);
    }

    curr_blk->add_instruction<StoreInst>(types, lvalue, to_store, node.loc);
    last_value = to_store;
}

void CFGBuilder::visit(CondExprLIR& node) {
    dbprint("visiting CondExprLIR node ", node.loc ? *node.loc : Location{});

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
    curr_blk              = true_blk;
    Value *true_val       = eval(*node.true_value);
    BasicBlock *true_exit = curr_blk;
    true_exit->terminate<Goto>()->set_target(merge_blk);

    // evaluate the false block
    curr_blk               = false_blk;
    Value *false_val       = eval(*node.false_value);
    BasicBlock *false_exit = curr_blk;
    false_exit->terminate<Goto>()->set_target(merge_blk);

    curr_blk     = merge_blk;
    PhiInst *phi = curr_blk->add_instruction<PhiInst>(node.act_type, node.loc);

    phi->add_incoming(true_val, true_exit);
    phi->add_incoming(false_val, false_exit);

    last_value = phi;

    assert(last_value && "last_value is nullptr at end of expr visit");
}

void CFGBuilder::visit(IdentExprLIR& node) {
    dbprint("visiting IdentExprLIR node ", node.loc ? *node.loc : Location{});

    if (node.sym->is_var()) {
        Value *val;
        if (auto *alloc = lookup_alloca(node.sym->as_varsym())) {
            val = alloc;
        } else {
            val = lookup_global(node.sym->as_varsym());
        }
        last_value = curr_blk->add_instruction<LoadInst>(node.act_type, val, node.loc);
    } else if (node.sym->is_func()) {
        last_value = add_or_get_function(node.sym->as_funcsym()->lir);
    }

    assert(last_value && "last_value is nullptr at end of expr visit");
}

void CFGBuilder::visit(LiteralExprLIR& node) {
    dbprint("visiting LiteralExprLIR node ", node.loc ? *node.loc : Location{});

    std::visit(
        match{
            [&](eval::Value& val) {
                dbprint("    Literal is Value, creating Literal value");
                last_value = curr_blk->add_value<ScalarConst>(node.act_type->as_primitive(), val);
            },
            [&](std::string& str) {
                dbprint("    Literal is string, creating String value");
                // string dedup happens here.
                assert(node.act_type->is_array());
                last_value = prog_cfg.add_or_get_string(node.act_type->as_array(), str);
                last_value->set_type(node.act_type);
            }},
        node.value);

    assert(last_value && "last_value is nullptr at end of expr visit");
}

void CFGBuilder::visit(ZeroExprLIR& node) {
    dbprint("visiting ZeroExprLIR node ", node.loc ? *node.loc : Location{});

    last_value = curr_blk->add_value<ZeroConst>(node.act_type);

    assert(last_value && "last_value is nullptr at end of expr visit");
}

void CFGBuilder::visit(CallExprLIR& node) {
    dbprint("visiting CallExprLIR node ", node.loc ? *node.loc : Location{});

    Vec<Value *> args;
    for (auto& arg : node.args) {
        args.push_back(eval(*arg));
    }
    last_value = curr_blk->add_instruction<CallInst>(
        node.act_type, eval(*node.callee), std::move(args), node.loc);

    assert(last_value && "last_value is nullptr at end of expr visit");
}

void CFGBuilder::visit(MemberAccExprLIR& node) {
    dbprint("visiting MemberAccExprLIR node ", node.loc ? *node.loc : Location{});

    Value *addr = eval_lvalue(node);

    last_value = curr_blk->add_instruction<LoadInst>(node.act_type, addr, node.loc);

    assert(last_value && "last_value is nullptr at end of expr visit");
}

void CFGBuilder::visit(ReintExprLIR& node) {
    dbprint("visiting ReintExprLIR node ", node.loc ? *node.loc : Location{});

    Value *operand = eval_lvalue(node);

    last_value = curr_blk->add_instruction<LoadInst>(node.act_type, operand, node.loc);

    assert(last_value && "last_value is nullptr at end of expr visit");
}

void CFGBuilder::visit(SubscrExprLIR& node) {
    dbprint("visiting SubscrExprLIR node ", node.loc ? *node.loc : Location{});

    Value *addr = eval_lvalue(node);

    last_value = curr_blk->add_instruction<LoadInst>(node.act_type, addr, node.loc);

    assert(last_value && "last_value is nullptr at end of expr visit");
}

void CFGBuilder::visit(PostfixExprLIR& node) {
    dbprint("visiting PostfixExprLIR node ", node.loc ? *node.loc : Location{});

    Value *address = eval_lvalue(*node.operand);
    assert(address && "non-lvalue operand for PostfixExprLIR");
    Value *old_val = curr_blk->add_instruction<LoadInst>(node.act_type, address, node.loc);

    Value *new_val;
    if (node.op == PostfixOp::POSTINC) {
        new_val = curr_blk->add_instruction<IncrInst>(node.act_type, old_val, node.loc);
    } else {
        new_val = curr_blk->add_instruction<DecrInst>(node.act_type, old_val, node.loc);
    }

    curr_blk->add_instruction<StoreInst>(types, address, new_val, node.loc);
    last_value = old_val; // postfix yields the *old* value
}

Constant *CFGBuilder::build_constant(ConstInitLIR& init) {
    if (isa<ScalarInitLIR>(&init)) {
        return build_constant(*dyncast<ScalarInitLIR>(&init));
    } else if (isa<PointerInitLIR>(&init)) {
        return build_constant(*dyncast<PointerInitLIR>(&init));
    } else if (isa<AggregateInitLIR>(&init)) {
        return build_constant(*dyncast<AggregateInitLIR>(&init));
    } else if (isa<StringInitLIR>(&init)) {
        return build_constant(*dyncast<StringInitLIR>(&init));
    } else if (isa<FuncInitLIR>(&init)) {
        return build_constant(*dyncast<FuncInitLIR>(&init));
    } else if (isa<ZeroInitLIR>(&init)) {
        return build_constant(*dyncast<ZeroInitLIR>(&init));
    } else {
        throw std::runtime_error("build constant got invalid LIRNode");
    }
}

Constant *CFGBuilder::build_constant(ScalarInitLIR& init) {
    if (curr_func->get_name() == IMPLICIT_MAIN_NAME) {
        return prog_cfg.add_constant<ScalarConst>(init.type->as_primitive(), init.val);
    } else {
        return curr_blk->add_value<ScalarConst>(init.type->as_primitive(), init.val);
    }
}

Constant *CFGBuilder::build_constant(PointerInitLIR& init) {
    if (curr_func->get_name() == IMPLICIT_MAIN_NAME) {
        return prog_cfg.add_constant<PointerConst>(init.type->as_pointer(), init.val);
    } else {
        return curr_blk->add_value<PointerConst>(init.type->as_pointer(), init.val);
    }
}

Constant *CFGBuilder::build_constant(AggregateInitLIR& init) {
    AggregateConst *aggreg;
    if (curr_func->get_name() == IMPLICIT_MAIN_NAME) {
        aggreg = prog_cfg.add_constant<AggregateConst>(init.type);
    } else {
        aggreg = curr_blk->add_value<AggregateConst>(init.type);
    }

    for (auto& elem : init.elements) {
        aggreg->elements.push_back(build_constant(*elem));
    }

    return aggreg;
}

Constant *CFGBuilder::build_constant(StringInitLIR& init) {

    ArrayType *str_type = types.get_array(types.get_i8(), init.str.size() + 1);

    return prog_cfg.add_or_get_string(str_type, init.str);
}

Constant *CFGBuilder::build_constant(FuncInitLIR& init) {

    return add_or_get_function(init.func);
}

Constant *CFGBuilder::build_constant(ZeroInitLIR& init) {
    if (curr_func->get_name() == IMPLICIT_MAIN_NAME) {
        return prog_cfg.add_constant<ZeroConst>(init.type);
    } else {
        return curr_blk->add_value<ZeroConst>(init.type);
    }
}