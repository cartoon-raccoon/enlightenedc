#include "lowering/cfg/cfg.hpp"

#include <stdexcept>

#include "lowering/cfg/visitor.hpp"
#include "semantics/types.hpp"
#include "tokens.hpp"
#include "util.hpp"

using namespace lower::cfg;
using namespace sema::types;
using namespace tokens;

BinaryInst::Operator BinaryInst::op_from_token(tokens::BinaryOp op) {
    using BinOp = tokens::BinaryOp;
    using Op    = Operator;

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
    using Op   = Operator;

    switch (op) {
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

DO_ACCEPT(If, CFGVisitor);
DO_ACCEPT(Goto, CFGVisitor);
DO_ACCEPT(Switch, CFGVisitor);
DO_ACCEPT(Return, CFGVisitor);

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
    if (is_initialized()) return entry;

    for (size_t i = 0; i < signature->num_params(); ++i) {
        Type *paramtype = signature->param_idx(i);
        add_arg(paramtype);
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

Span<Box<Alloca>> FunctionCFG::get_allocas() {
    return allocas;
}

Alloca *FunctionCFG::add_alloca(Type *type, std::string name) {
    auto alloc = std::make_unique<Alloca>(type, std::move(name));
    auto *ret  = alloc.get();

    allocas.push_back(std::move(alloc));

    return ret;
}

Global *ProgramCFG::add_global(Type *type, std::string name, Value *init) {

    Box<Global> new_global;
    if (init) {
        new_global = std::make_unique<Global>(type, std::move(name), init);
    } else {
        new_global = std::make_unique<Global>(type, std::move(name));
    }

    Global *ret = new_global.get();
    globals.push_back(std::move(new_global));

    return ret;
}

Span<Box<Global>> ProgramCFG::get_globals() {
    return globals;
}

String *ProgramCFG::add_or_get_string(ArrayType *type, const std::string& str) {
    if (strings.contains(str)) {
        return strings[str].get();
    }

    auto new_str = std::make_unique<String>(type, str);

    String *ret = new_str.get();

    strings[str] = std::move(new_str);

    return ret;
}

FunctionCFG *ProgramCFG::add_function(sema::types::FunctionType *sig, std::string name) {

    auto funcfg = std::make_unique<FunctionCFG>(sig, std::move(name));

    FunctionCFG *ret = funcfg.get();

    functions.push_back(std::move(funcfg));

    return ret;
}

Span<Box<FunctionCFG>> ProgramCFG::get_functions() {
    return functions;
}