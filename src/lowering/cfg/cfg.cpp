#include "lowering/cfg/cfg.hpp"

#include <stdexcept>

#include "lowering/cfg/visitor.hpp"
#include "lowering/lir/symbols.hpp"
#include "semantics/types.hpp"
#include "tokens.hpp"
#include "util.hpp"

using namespace lower::cfg;
using namespace lower::lir;
using namespace sema::types;
using namespace tokens;

BinaryInst::Operator BinaryInst::op_from_token(tokens::BinaryOp op) {
    using BinOp = tokens::BinaryOp;
    using Op = Operator;

    switch (op) {
    case BinOp::OR:
        return Op::OR;
    case BinOp::XOR:
        return Op::XOR;
    case BinOp::AND:
        return Op::AND;
    case BinOp::EQ:
        return Op::EQ;
    case BinOp::NE:
        return Op::NE;
    case BinOp::LT:
        return Op::LT;
    case BinOp::GT:
        return Op::GT;
    case BinOp::LE:
        return Op::LE;
    case BinOp::GE:
        return Op::GE;
    case BinOp::LSHIFT:
        return Op::SHL;
    case BinOp::RSHIFT:
        return Op::SHR;
    case BinOp::PLUS:
        return Op::ADD;
    case BinOp::MINUS:
        return Op::SUB;
    case BinOp::MUL:
        return Op::MUL;
    case BinOp::DIV:
        return Op::DIV;
    case BinOp::MOD:
        return Op::MOD;
    default:
        throw std::runtime_error("BinaryInst::op_from_token: invalid operator");
    }
}

UnaryInst::Operator UnaryInst::op_from_token(tokens::UnaryOp op) {
    using UnOp = tokens::UnaryOp;
    using Op = Operator;

    switch (op) {
    case UnOp::REF:
        return Op::REF;
    case UnOp::DEREF:
        return Op::DEREF;
    case UnOp::POS:
        return Op::POS;
    case UnOp::NEG:
        return Op::NEG;
    case UnOp::TILDE:
        return Op::TILDE;
    case UnOp::NOT:
        return Op::NOT;
    default:
        throw std::runtime_error("UnaryInst::op_from_token: invalid operator");
    }
}

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
    parent->values.push_back(std::move(val));
}

void BasicBlock::link_to(BasicBlock *target) {
    succs.push_back(target);
    target->incoming.push_back(this);
}

BasicBlock *BasicBlockIfSuccIter::next() {
    BasicBlock *ret;
    switch (state) {
    case TRUE:
        ret   = i->then_br;
        state = FALSE;
        break;
    case FALSE:
        ret   = i->else_br;
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

        FunctionType *signature = lir->funcsym->signature;
        for (size_t i = 0; i < signature->num_params(); ++i) {
            Type *paramtype = signature->param_idx(i);
            add_arg(paramtype);
        }

    } else {
        // fixme: come up with a good compiler-internal naming convention
        name = "__implicit_main";
    }

    auto& entry_blk    = blocks.emplace_back(name, this);
    entry_blk.is_entry = true;
    entry              = &entry_blk;

    return entry;
}

FuncArg *FunctionCFG::add_arg(Type *type) {
    auto arg = std::make_unique<FuncArg>(type);

    FuncArg *ret = arg.get();

    args.push_back(std::move(arg));

    return ret;
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

FunctionCFGAllocas FunctionCFG::get_allocas() {
    return FunctionCFGAllocas(this);
}

AllocaInst *FunctionCFG::add_alloca(BasicBlock *blk, LIRVarSym *sym) {
    auto alloc = std::make_unique<AllocaInst>(blk, sym->type, sym);
    auto *ret  = alloc.get();

    alloca_order.push_back(sym);
    allocas[sym] = std::move(alloc);

    return ret;
}

Global *ProgramCFG::add_or_get_global(LIRVarSym *var) {
    if (globals.contains(var)) {
        return globals[var].get();
    }

    auto new_global = std::make_unique<Global>(var);

    Global *ret = new_global.get();

    globals[var] = std::move(new_global);
    global_order.push_back(var);

    return ret;
}

Global *ProgramCFG::lookup_global(LIRVarSym *sym) {
    return globals.contains(sym) ? globals[sym].get() : nullptr;
}

ProgramCFGGlobals ProgramCFG::get_globals() {
    return ProgramCFGGlobals(this);
}

String *ProgramCFG::add_or_get_string(const std::string& str) {
    if (strings.contains(str)) {
        return strings[str].get();
    }

    auto new_str = std::make_unique<String>(str);

    String *ret = new_str.get();

    strings[str] = std::move(new_str);

    return ret;
}

FunctionCFG *ProgramCFG::add_or_get_function(FunctionLIR *func) {

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
    add_or_get_function(func);
    auto ref = std::make_unique<FuncRef>(func->funcsym->signature, functions[func].get());

    auto *ret = ref.get();
    funcrefs.push_back(std::move(ref));

    return ret;
}