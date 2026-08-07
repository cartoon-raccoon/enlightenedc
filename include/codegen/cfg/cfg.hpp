#pragma once

#ifndef ECC_CFG_H
#define ECC_CFG_H

#include <concepts>
#include <utility>
#include <variant>

#include "codegen/lir/lir.hpp"
#include "codegen/lir/symbols.hpp"
#include "eval/value.hpp"
#include "semantics/types.hpp"
#include "tokens.hpp"
#include "util.hpp"

namespace ecc::codegen::cfg {

using EvalValue = eval::Value;

using namespace ecc;
using namespace util;

class Instruction;
class FuncRef;
class Literal;
class Zero;

class Value : public NoCopy {
public:
    enum class ValueKind : uint8_t {
        INST,
        FUNC,
        LIT,
        ZERO,
    };

    Value(ValueKind kind, sema::types::Type *type, Location loc)
        : valkind(kind), type(type), loc(loc) {}

    Value(ValueKind kind, Location loc) : valkind(kind), loc(loc) {}

    Value(ValueKind kind, sema::types::Type *type) : valkind(kind), type(type) {}

    Value(ValueKind kind) : valkind(kind) {}

    ValueKind valkind;
    sema::types::Type *type = nullptr;
    Optional<Location> loc;

    virtual ~Value() = default;

    virtual Instruction *as_instruction() { return nullptr; }
    virtual FuncRef *as_funcref() { return nullptr; }
    virtual Literal *as_literal() { return nullptr; }
    virtual Zero *as_zero() { return nullptr; }
};

class Instruction : public Value {
public:
    enum class InstKind : uint8_t {
        ALLOCA,
        LOAD,
        STORE,
        BINARY,
        UNARY,
        CAST,
        REINT,
        MEMBERACC,
        SUBSCR,
        CALL,
    };

    InstKind instkind;

    Instruction(InstKind kind, sema::types::Type *type, Location loc)
        : Value(ValueKind::INST, type, loc), instkind(kind) {}

    Instruction(InstKind kind, Location loc) : Value(ValueKind::INST, loc), instkind(kind) {}

    Instruction(InstKind kind) : Value(ValueKind::INST), instkind(kind) {}

    Instruction *as_instruction() override { return this; }
};

class AllocaInst : public Instruction {
public:
    AllocaInst(sema::types::Type *type, std::string name, lir::LIRVarSym *sym)
        : Instruction(InstKind::ALLOCA, type, sym->sym->loc), sym(sym), name(std::move(name)) {}

    AllocaInst(sema::types::Type *type, lir::LIRVarSym *sym)
        : Instruction(InstKind::ALLOCA, type, sym->sym->loc), sym(sym) {}

    lir::LIRVarSym *sym;
    Optional<std::string> name;
};

class LoadInst : public Instruction {
public:
    LoadInst(sema::types::Type *type, Value *value, Location loc)
        : Instruction(InstKind::LOAD, type, loc), value(value) {}

    Value *value;
};

class StoreInst : public Instruction {
public:
    StoreInst(sema::types::Type *type, Value *value, Location loc)
        : Instruction(InstKind::STORE, type, loc), value(value) {}

    Value *value;
};

class BinaryInst : public Instruction {
public:
    BinaryInst(
        sema::types::Type *type, tokens::BinaryOp op, Value *loperand, Value *roperand,
        Location loc)
        : Instruction(InstKind::BINARY, type, loc), op(op), loperand(loperand), roperand(roperand) {
    }

    tokens::BinaryOp op;
    Value *loperand, *roperand;
};

class UnaryInst : public Instruction {
public:
    UnaryInst(sema::types::Type *type, tokens::UnaryOp op, Value *operand, Location loc)
        : Instruction(InstKind::UNARY, type, loc), op(op), operand(operand) {}

    tokens::UnaryOp op;
    Value *operand;
};

class CastInst : public Instruction {
public:
    CastInst(sema::types::Type *type, sema::types::Type *target, Value *operand, Location loc)
        : Instruction(InstKind::CAST, type, loc), target(target), operand(operand) {}
    
    sema::types::Type *target;
    Value *operand;
};

class ReintInst : public Instruction {
public:
    ReintInst(sema::types::Type *type, tokens::PrimType target, Value *operand, Location loc)
        : Instruction(InstKind::REINT, type, loc), target(target), operand(operand) {}

    tokens::PrimType target;
    Value *operand;
};

class MemberAccInst : public Instruction {
public:
    MemberAccInst(sema::types::Type *type, size_t member_idx, Value *operand, Location loc)
        : Instruction(InstKind::MEMBERACC, type, loc), member_idx(member_idx), operand(operand) {}
    
    size_t member_idx;
    Value *operand;
};

class SubscrInst : public Instruction {
public:
    SubscrInst(sema::types::Type *type, Value *index, Value *operand, Location loc)
        : Instruction(InstKind::SUBSCR, type, loc), index(index), operand(operand) {}

    Value *index;
    Value *operand;
};

class CallInst : public Instruction {
public:
    CallInst(sema::types::Type *type, Value *operand, Vec<Value *> args, Location loc)
        : Instruction(InstKind::CALL, type, loc), operand(operand), args(std::move(args)) {}
    
    Value *operand;
    Vec<Value *> args;
};

/**
A value that holds a reference to a function symbol.

FuncRef holds a reference to a LIRFuncSym, to distinguish it from LIRVarSym,
which is the only symbol type Load can operate on.
*/
class FuncRef : public Value {
public:
    FuncRef(sema::types::FunctionType *sig, lir::LIRFuncSym *ref)
        : Value(ValueKind::FUNC, sig), func(ref) {}

    lir::LIRFuncSym *func;

    FuncRef *as_funcref() override { return this; }
};

class Literal : public Value {
public:
    using LiteralVariant = std::variant<eval::Value, std::string>;

    Literal(sema::types::Type *type, eval::Value& value)
        : Value(ValueKind::LIT, type), value(value) {}

    Literal(sema::types::Type *type, std::string value)
        : Value(ValueKind::LIT, type), value(std::move(value)) {}
    
    LiteralVariant value;

    bool is_string() const {
        return std::holds_alternative<std::string>(value);
    }

    bool is_value() const {
        return std::holds_alternative<eval::Value>(value);
    }

    Literal *as_literal() override { return this; }
};

class Zero : public Value {
public:
    Zero(sema::types::Type *type)
        : Value(ValueKind::ZERO, type) {}
    
    Zero *as_zero() override { return this; }
};

class BasicBlock;
class If;
class Goto;
class Return;
class Switch;

/**
A terminator of a basic block.

A terminator indicates how control flow moves out of a block.
For example, an `If` terminator creates a branch that links to two blocks,
contingent upon a condition.
*/
class Terminator {
public:
    enum class Kind : uint8_t {
        IF,
        GOTO,
        RETURN,
        SWITCH,
    };

    Terminator(Kind kind, BasicBlock *terminating) : kind(kind), terminating_blk(terminating) {}
    virtual ~Terminator() = default;

    Kind kind;
    BasicBlock *terminating_blk;

    virtual If *as_if() { return nullptr; }
    virtual Goto *as_goto() { return nullptr; }
    virtual Return *as_return() { return nullptr; }
    virtual Switch *as_switch() { return nullptr; }
};

class If : public Terminator {
public:
    If(BasicBlock *termng, Value *cond) : Terminator(Kind::IF, termng), cond(cond) {}

    Value *cond;

    BasicBlock *then_br = nullptr;
    BasicBlock *else_br = nullptr;

    If *as_if() override { return this; }
};

class Goto : public Terminator {
public:
    Goto(BasicBlock *termng) : Terminator(Kind::GOTO, termng) {}

    BasicBlock *target = nullptr;

    Goto *as_goto() override { return this; }
};

class Return : public Terminator {
public:
    Return(BasicBlock *termng) : Terminator(Kind::RETURN, termng) {}

    Optional<lir::ExprLIR *> expr;

    Return *as_return() override { return this; }
};

class SwitchCase {
public:
    SwitchCase(eval::Value val, BasicBlock *blk) : case_val(val), blk(blk) {}

    SwitchCase(BasicBlock *blk) : blk(blk) {}

    bool is_default() const { return !case_val.has_value(); }

    Optional<eval::Value> case_val;
    BasicBlock *blk;
};

class Switch : public Terminator {
public:
    Switch(BasicBlock *termng) : Terminator(Kind::SWITCH, termng) {}

    void add_case(eval::Value& val, BasicBlock *blk) { cases.emplace_back(val, blk); }

    void add_default(BasicBlock *blk) { cases.emplace_back(blk); }

    size_t num_cases() const { return cases.size(); }

    Vec<SwitchCase> cases;

    Switch *as_switch() override { return this; }
};

class FunctionCFG;

/**
The basic unit of the CFG.
*/
class BasicBlock : public NoCopy, public NoMove {
public:
    friend class FunctionCFG;
    
    BasicBlock(std::string& label, FunctionCFG *func) : func(func), label(label) {}

    static Box<BasicBlock> entry(std::string& func_name, FunctionCFG *func);

    void set_terminator(Box<Terminator> term) { this->term = std::move(term); }

    bool has_terminator() { return term != nullptr; }

    template <typename T, typename ...Args>
        requires std::derived_from<T, Instruction>
    void add_instruction(Args ... args) {
        Box<Instruction> inst = std::make_unique<T>(args...);
        push_instruction(std::move(inst));
    }

    template <typename T, typename ...Args>
        requires std::derived_from<T, Value>
    Value *add_value(Args ... args) {
        Box<Value> val = std::make_unique<T>(args ...);
        Value *ret = val.get();

        push_value(std::move(val));

        return ret;
    }

private:
    void push_instruction(Box<Instruction> inst) { instructions.push_back(std::move(inst)); }

    void push_value(Box<Value> val);

    // Whether the block is an entry block into a function.
    bool is_entry = false;
    // Whether the block is part of a loop structure.
    bool is_part_of_loop = false;

    FunctionCFG *func;
    std::string label;

    Vec<BasicBlock *> incoming;
    Vec<BasicBlock *> successors;
    // The instructions that make up the block.
    Vec<Box<Instruction>> instructions;

    Box<Terminator> term = nullptr;
};

/**
A single function, composed of linked blocks.
*/
class FunctionCFG {
public:
    friend class BasicBlock;

    FunctionCFG(lir::FunctionLIR *func) : lir(func) {}

    lir::FunctionLIR *lir;

    BasicBlock *create_block(std::string& label);

    void append_block(Box<BasicBlock> block) { blocks.push_back(std::move(block)); }

    BasicBlock *lookup_block();

    size_t num_blocks() { return blocks.size(); }

    void remove_block(BasicBlock *blk);

private:
    Vec<Box<BasicBlock>> blocks;
    // Bag of non-instruction values.
    Vec<Box<Value>> values;
};

class ProgramCFG {
public:
    ProgramCFG() {}

    FunctionCFG *add_function();

    Vec<Box<FunctionCFG>> functions;
};

} // end namespace ecc::codegen::cfg

#endif