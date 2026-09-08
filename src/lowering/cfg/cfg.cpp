#include "lowering/cfg/cfg.hpp"

#include "lowering/cfg/visitor.hpp"
#include "semantics/types.hpp"
#include "tokens.hpp"
#include "prelude.hpp"

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
        ECC_UNREACHABLE("BinaryInst::op_from_token: invalid operator");
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
        ECC_UNREACHABLE("UnaryInst::op_from_token: invalid operator");
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

Box<BasicBlock> BasicBlock::entry(StringRef func_name, Function *func) {
    auto ret      = std::make_unique<BasicBlock>(func_name, func);
    ret->is_entry = true;

    return ret;
}

Instruction *BasicBlock::first_non_phi_inst() const {
    Instruction *curr = &instructions.first();
    while (!isa<PhiInst>(curr)) {
        curr = curr->next();
    }

    return curr;
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

BasicBlock *Function::initialize() {
    if (is_initialized())
        return entry;

    for (size_t i = 0; i < signature->num_params(); ++i) {
        Type *paramtype = signature->param_at(i);
        add_arg(paramtype);
    }

    auto& entry_blk    = blocks.emplace_back(name, this);
    entry_blk.is_entry = true;
    entry              = &entry_blk;

    return entry;
}

FuncArg *Function::add_arg(Type *type) {
    auto arg = std::make_unique<FuncArg>(type);

    FuncArg *ret = arg.get();

    args.push_back(std::move(arg));

    return ret;
}

bool Function::is_plain_return() const {
    if (!entry->terminator()) {
        return false;
    }

    return num_blocks() == 1 && entry->is_empty() && isa<Return>(entry->terminator());
}

BasicBlock *Function::create_block() {
    return &blocks.emplace_back(this);
}

BasicBlock *Function::create_block(StringRef name, bool make_labeled) {
    auto& block = blocks.emplace_back(name, this);

    if (make_labeled) {
        labeled_blocks[name.str()] = &block;
    }

    return &block;
}

BasicBlock *Function::create_block_before(BasicBlock *succ) {
    auto& block = blocks.emplace_before(*succ, this);

    return &block;
}

BasicBlock *
Function::create_block_before(BasicBlock *succ, StringRef name, bool make_labeled) {
    auto& block = blocks.emplace_before(*succ, name, this);

    if (make_labeled) {
        labeled_blocks[name.str()] = &block;
    }

    return &block;
}

BasicBlock *Function::create_block_after(BasicBlock *prec) {
    auto& block = blocks.emplace_after(*prec, this);

    return &block;
}

BasicBlock *
Function::create_block_after(BasicBlock *prec, StringRef name, bool make_labeled) {
    auto& block = blocks.emplace_after(*prec, name, this);

    if (make_labeled) {
        labeled_blocks[name.str()] = &block;
    }

    return &block;
}

void Function::swap_blocks(BasicBlock *first, BasicBlock *second) {
    blocks.swap(*first, *second);
}

BasicBlock *Function::lookup_labeled_block(StringRef label) {
    auto it = labeled_blocks.find(label);
    return it == labeled_blocks.end() ? nullptr : it->second;
}

Span<Box<Alloca>> Function::get_allocas() {
    return allocas;
}

Alloca *Function::add_alloca(Type *type, StringRef name) {
    auto alloc = std::make_unique<Alloca>(type, name);
    auto *ret  = alloc.get();

    allocas.push_back(std::move(alloc));

    return ret;
}

Alloca *Function::add_alloca(Type *type) {
    auto alloc = std::make_unique<Alloca>(type);
    auto *ret  = alloc.get();

    allocas.push_back(std::move(alloc));

    return ret;
}

Global *Program::add_global(Type *type, StringRef name, Value *init) {

    Box<Global> new_global;
    if (init) {
        new_global = std::make_unique<Global>(type, name, init);
    } else {
        new_global = std::make_unique<Global>(type, name);
    }

    Global *ret = new_global.get();
    globals.push_back(std::move(new_global));

    return ret;
}

Span<Box<Global>> Program::get_globals() {
    return globals;
}

ScalarConst *Program::get_scalar(PrimitiveType *type, const eval::Value& val) {
    if (scalars.contains(val)) {
        return scalars.find(val)->second.get();
    }

    auto scl = make_box<ScalarConst>(type, val);
    auto *ret = scl.get();

    scalars[val] = std::move(scl);

    return ret;
}

ZeroConst *Program::get_zero(Type *type) {
    if (zeroes.contains(type)) {
        return zeroes.find(type)->second.get();
    }

    auto zero = make_box<ZeroConst>(type);
    auto *ret = zero.get();

    zeroes[type] = std::move(zero);

    return ret;
}

PointerConst *Program::get_pointer(PointerType *ptr, const eval::Value& val) {
    PointerKey key(ptr, val);

    if (pointers.contains(key)) {
        return pointers.find(key)->second.get();
    }

    auto pointer = make_box<PointerConst>(ptr, val);
    auto *ret = pointer.get();

    pointers[key] = std::move(pointer);

    return ret;
}

AggregateConst *Program::get_aggregate(Type *type, const Vec<Constant *>& structure) {
    AggregateKeyView key(type, structure);

    if (auto it = aggregates.find(key); it != aggregates.end()) {
        return it->second.get();
    }

    auto agg      = make_box<AggregateConst>(type);
    agg->elements = structure;
    auto *ret     = agg.get();

    aggregates.emplace(AggregateKey{type, structure}, std::move(agg));

    return ret;
}


String *Program::get_string(ArrayType *type, StringRef str) {
    if (auto it = strings.find(str); it != strings.end())
        return it->second.get();


    auto new_str = std::make_unique<String>(type, str.str());

    String *ret = new_str.get();

    strings[str] = std::move(new_str);

    return ret;
}

Function *Program::add_function(sema::types::FunctionType *sig, StringRef name) {

    auto funcfg = std::make_unique<Function>(sig, name.str());

    Function *ret = funcfg.get();

    functions.push_back(std::move(funcfg));

    return ret;
}

Span<Box<Function>> Program::get_functions() {
    return functions;
}