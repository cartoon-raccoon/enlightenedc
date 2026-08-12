#include "lowering/cfg/cfg.hpp"

#include "lowering/cfg/visitor.hpp"
#include "lowering/lir/symbols.hpp"
#include "util.hpp"

using namespace lower::cfg;
using namespace lower::lir;

DO_ACCEPT(AllocaInst, CFGValueVisitor);
DO_ACCEPT(LoadInst, CFGValueVisitor);
DO_ACCEPT(StoreInst, CFGValueVisitor);
DO_ACCEPT(PhiInst, CFGValueVisitor);
DO_ACCEPT(PrintInst, CFGValueVisitor);
DO_ACCEPT(BinaryInst, CFGValueVisitor);
DO_ACCEPT(UnaryInst, CFGValueVisitor);
DO_ACCEPT(IncrInst, CFGValueVisitor);
DO_ACCEPT(DecrInst, CFGValueVisitor);
DO_ACCEPT(CastInst, CFGValueVisitor);
DO_ACCEPT(ReintInst, CFGValueVisitor);
DO_ACCEPT(MemberAccInst, CFGValueVisitor);
DO_ACCEPT(SubscrInst, CFGValueVisitor);
DO_ACCEPT(CallInst, CFGValueVisitor);

DO_ACCEPT(FuncRef, CFGValueVisitor);
DO_ACCEPT(Literal, CFGValueVisitor);
DO_ACCEPT(Zero, CFGValueVisitor);

void If::set_then_target(BasicBlock *blk) {
    then_br = blk;
    terminating_blk->link_to(blk);
}

void If::set_else_target(BasicBlock *blk) {
    else_br = blk;
    terminating_blk->link_to(blk);
}

void Goto::set_target(BasicBlock *blk) {
    target = blk;
    terminating_blk->link_to(blk);
}

void Switch::add_case(eval::Value& val, BasicBlock *blk) {
    cases.emplace_back(blk, val);
    terminating_blk->link_to(blk);
}

void Switch::add_default(BasicBlock *blk) {
    cases.emplace_back(blk);
    terminating_blk->link_to(blk);
}

Box<BasicBlock> BasicBlock::entry(std::string& func_name, FunctionCFG *func) {
    auto ret      = std::make_unique<BasicBlock>(func_name, func);
    ret->is_entry = true;

    return ret;
}

void BasicBlock::push_value(Box<Value> val) {
    func->values.push_back(std::move(val));
}

void BasicBlock::link_to(BasicBlock *target) {
    succs.push_back(target);
    target->incoming.push_back(this);
}

BasicBlock *BasicBlockIfSuccIter::next() {
    BasicBlock *ret;
        switch (state) {
        case TRUE:
            ret = i->then_br;
            state = FALSE;
            break;
        case FALSE:
            ret = i->else_br;
            state = DONE;
            break;
        case DONE:
            ret = nullptr;
            break;
        }

        return ret;
}

BasicBlock *BasicBlockGotoSuccIter::next() {
    if (iterated) {
            return nullptr;
        } else {
            iterated = true;
            return g->target;
        }
}

BasicBlock *BasicBlockSwitchSuccIter::next() {
    if (idx >= sw->num_cases()) {
            return nullptr;
        }
        return sw->cases[idx++].blk;
}

BasicBlock *FunctionCFG::initialize() {
    assert(blocks.empty() && "attempted to initialize already initialized FunctionCFG");

    std::string name;
    if (lir) {
        name = lir->funcsym->mangled_name;
    } else {
        // fixme: come up with a good compiler-internal naming convention
        name = "__implicit_main";
    }

    auto& entry_blk    = blocks.emplace_back(name, this);
    entry_blk.is_entry = true;
    entry = &entry_blk;

    return entry;
}

BasicBlock *FunctionCFG::create_block() {
    return &blocks.emplace_back(this);
}

BasicBlock *FunctionCFG::create_block(std::string& name, bool make_labeled) {
    auto& block = blocks.emplace_back(name, this);

    if (make_labeled) {
        labeled_blocks[name] = &block;
    }

    return &block;
}

BasicBlock *FunctionCFG::create_block_before(BasicBlock *succ) {
    auto& block = blocks.emplace_before(*succ, this);

    return &block;
}

BasicBlock *
FunctionCFG::create_block_before(BasicBlock *succ, std::string& name, bool make_labeled) {
    auto& block = blocks.emplace_before(*succ, name, this);

    if (make_labeled) {
        labeled_blocks[name] = &block;
    }

    return &block;
}

BasicBlock *FunctionCFG::create_block_after(BasicBlock *prec) {
    auto& block = blocks.emplace_after(*prec, this);

    return &block;
}

BasicBlock *
FunctionCFG::create_block_after(BasicBlock *prec, std::string& name, bool make_labeled) {
    auto& block = blocks.emplace_after(*prec, name, this);

    if (make_labeled) {
        labeled_blocks[name] = &block;
    }

    return &block;
}

void FunctionCFG::swap_blocks(BasicBlock *first, BasicBlock *second) {
    blocks.swap(*first, *second);
}

BasicBlock *FunctionCFG::lookup_labeled_block(std::string& label) {
    if (labeled_blocks.contains(label)) {
        return labeled_blocks[label];
    } else {
        return nullptr;
    }
}

void FunctionCFG::add_pending_goto(std::string& label, Goto *g) {
    if (pending_gotos.contains(label)) {
        pending_gotos[label].push_back(g);
    } else {
        pending_gotos[label] = {};
        pending_gotos[label].push_back(g);
    }
}

size_t FunctionCFG::resolve_pending_gotos(std::string& label, BasicBlock *target) {
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


AllocaInst *FunctionCFG::lookup_alloca(LIRVarSym *sym) {
    return allocas.contains(sym) ? allocas[sym].get() : nullptr;
}

AllocaInst *FunctionCFG::add_alloca(BasicBlock *blk, LIRVarSym *sym) {
    auto alloc = std::make_unique<AllocaInst>(blk, sym->sym->type, sym);
    auto *ret  = alloc.get();

    alloca_order.push_back(sym);
    allocas[sym] = std::move(alloc);

    return ret;
}

FunctionCFG *ProgramCFG::add_function(FunctionLIR *func) {

    if (functions.contains(func)) {
        return functions[func].get();
    }

    auto funcfg = std::make_unique<FunctionCFG>(func);

    FunctionCFG *ret = funcfg.get();

    functions[func] = std::move(funcfg);

    return ret;
}

FuncRef *ProgramCFG::ref_function(FunctionLIR *func) {

    if (functions.contains(func)) {
        auto ref = std::make_unique<FuncRef>(func->funcsym->signature, functions[func].get());

        auto *ret = ref.get();
        funcrefs.push_back(std::move(ref));

        return ret;
    }

    // if lookup fails, add the function
    add_function(func);
    auto ref = std::make_unique<FuncRef>(func->funcsym->signature, functions[func].get());

    auto *ret = ref.get();
    funcrefs.push_back(std::move(ref));

    return ret;
}