#pragma once

#ifndef ECC_CFG_H
#define ECC_CFG_H

#include <concepts>
#include <stdexcept>
#include <utility>

#include "ds/linkedlist.hpp"
#include "eval/value.hpp"
#include "lowering/cfg/visitor.hpp"
#include "semantics/types.hpp"
#include "tokens.hpp"
#include "util.hpp"

namespace ecc::lower::cfg {

using EvalValue = eval::Value;

using namespace ecc;
using namespace util;

class Instruction;

class BasicBlock;
class If;
class Goto;
class Return;
class Switch;

class FunctionCFG;

template <typename DerivedT, typename BaseT>
using CFGVisitable = Visitable<DerivedT, BaseT, CFGVisitor>;

/**
A CFG value.
*/
class Value : public NoCopy {
public:
    enum class ValueKind : uint8_t {
        INST,
        SCALAR,
        POINTER,
        AGGREG,
        ZERO,
        FUNC,
        GLOBAL,
        ALLOCA,
        ARG,
        STR,
    };

    Value(ValueKind kind, sema::types::Type *type, Location loc)
        : valkind(kind), type(type), eff_type(type->effective_type()), loc(loc) {}

    Value(ValueKind kind, sema::types::Type *type, Optional<Location> loc)
        : valkind(kind), type(type), eff_type(type->effective_type()), loc(loc) {}

    Value(ValueKind kind, sema::types::Type *type, std::string name, Location loc)
        : valkind(kind), name(std::move(name)), type(type), eff_type(type->effective_type()),
          loc(loc) {}

    Value(ValueKind kind, sema::types::Type *type, std::string name)
        : valkind(kind), name(std::move(name)), type(type), eff_type(type->effective_type()) {}

    Value(ValueKind kind, sema::types::Type *type, std::string name, Optional<Location> loc)
        : valkind(kind), name(std::move(name)), type(type), eff_type(type->effective_type()),
          loc(loc) {}

    Value(ValueKind kind, Location loc) : valkind(kind), loc(loc) {}

    Value(ValueKind kind, sema::types::Type *type)
        : valkind(kind), type(type), eff_type(type->effective_type()) {}

    Value(ValueKind kind) : valkind(kind) {}

    ValueKind valkind;

    /**
    The name of the value.

    An empty string means that the Value has no name.
    */
    std::string name;
    sema::types::Type *type     = nullptr;
    sema::types::Type *eff_type = nullptr;
    Optional<Location> loc;

    virtual ~Value() = default;

    bool named() const { return !this->name.empty(); }

    void set_name(std::string&& name) { this->name = std::move(name); }

    void set_name(const std::string& name) { this->name = name; }

    void set_type(sema::types::Type *type) {
        this->type     = type;
        this->eff_type = type->effective_type();
    }

    virtual Instruction *as_instruction() { return nullptr; }
    virtual ScalarConst *as_scalar() { return nullptr; }
    virtual PointerConst *as_pointer() { return nullptr; }
    virtual AggregateConst *as_aggregate() { return nullptr; }
    virtual ZeroConst *as_zero() { return nullptr; }
    virtual Global *as_global() { return nullptr; }
    virtual Alloca *as_alloca() { return nullptr; }
    virtual FuncArg *as_funcarg() { return nullptr; }
    virtual String *as_string() { return nullptr; }

    virtual void accept(CFGVisitor& visitor) = 0;
};

class Constant : public Value {
public:
    Constant(ValueKind kind, sema::types::Type *type) : Value(kind, type) {}

    Constant(ValueKind kind, sema::types::Type *type, Location loc) : Value(kind, type, loc) {}

    Constant(ValueKind kind, sema::types::Type *type, std::string name)
        : Value(kind, type, std::move(name)) {}

    static bool classof(const Value *node) {
        switch (node->valkind) {
        case ValueKind::SCALAR:
        case ValueKind::AGGREG:
        case ValueKind::ZERO:
        case ValueKind::FUNC:
        case ValueKind::STR:
            return true;
        default:
            return false;
        }
    }
};

class ScalarConst : public CFGVisitable<ScalarConst, Constant> {
public:
    ScalarConst(sema::types::PrimitiveType *type, eval::Value& value)
        : CFGVisitable<ScalarConst, Constant>(ValueKind::SCALAR, type), value(value) {}

    eval::Value value;

    ScalarConst *as_scalar() override { return this; }

    static bool classof(const Value *node) { return node->valkind == ValueKind::SCALAR; }
};

class PointerConst : public CFGVisitable<PointerConst, Constant> {
public:
    PointerConst(sema::types::PointerType *type, eval::Value& value)
        : CFGVisitable<PointerConst, Constant>(ValueKind::POINTER, type), value(value) {}

    eval::Value value;

    PointerConst *as_pointer() override { return this; }

    static bool classof(const Value *node) { return node->valkind == ValueKind::POINTER; }
};

class AggregateConst : public CFGVisitable<AggregateConst, Constant> {
public:
    AggregateConst(sema::types::Type *type)
        : CFGVisitable<AggregateConst, Constant>(ValueKind::AGGREG, type) {}

    Vec<Constant *> elements;

    AggregateConst *as_aggregate() override { return this; }

    static bool classof(const Value *node) { return node->valkind == ValueKind::AGGREG; }
};

class ZeroConst : public CFGVisitable<ZeroConst, Constant> {
public:
    ZeroConst(sema::types::Type *type) : CFGVisitable<ZeroConst, Constant>(ValueKind::ZERO, type) {}

    ZeroConst *as_zero() override { return this; }

    static bool classof(const Value *node) { return node->valkind == ValueKind::ZERO; }
};

class String : public CFGVisitable<String, Constant> {
public:
    String(sema::types::Type *type, std::string data)
        : CFGVisitable<String, Constant>(ValueKind::STR, type), data(std::move(data)) {}

    std::string data;

    String *as_string() override { return this; }

    static bool classof(const Value *node) { return node->valkind == ValueKind::STR; }
};

class Global : public CFGVisitable<Global, Value> {
public:
    Global(sema::types::Type *type, std::string name)
        : CFGVisitable<Global, Value>(ValueKind::GLOBAL, type, std::move(name)) {}

    Global(sema::types::Type *type, std::string name, Value *initializer)
        : CFGVisitable<Global, Value>(ValueKind::GLOBAL, type, std::move(name)),
          initializer(initializer) {}

    /**
    The initial value of the global, if it has one.
    */
    Value *initializer = nullptr;

    Global *as_global() override { return this; }

    static bool classof(const Value *node) { return node->valkind == ValueKind::GLOBAL; }
};

class Alloca : public CFGVisitable<Alloca, Value> {
public:
    Alloca(sema::types::Type *type, std::string name)
        : CFGVisitable<Alloca, Value>(ValueKind::ALLOCA, type, std::move(name)), type(type) {}

    Alloca(sema::types::Type *type)
        : CFGVisitable<Alloca, Value>(ValueKind::ALLOCA, type), type(type) {}

    sema::types::Type *type;

    Alloca *as_alloca() override { return this; }

    static bool classof(const Value *node) { return node->valkind == ValueKind::ALLOCA; }
};

/**
An argument to a function.

FunctionCFG stores these as the values to be stored into the allocations.
*/
class FuncArg : public CFGVisitable<FuncArg, Value> {
public:
    FuncArg(sema::types::Type *type) : CFGVisitable<FuncArg, Value>(ValueKind::ARG, type) {}

    FuncArg *as_funcarg() override { return this; }

    static bool classof(const Value *node) { return node->valkind == ValueKind::ARG; }
};

/**
A unit of execution in the CFG IR.
*/
class Instruction : public Value, public ds::LinkedListNode<Instruction> {
public:
    enum class InstKind : uint8_t {
        LOAD,
        STORE,
        PHI,
        PRINT,
        MEMCPY,
        BINARY,
        UNARY,
        INC,
        DEC,
        CAST,
        MEMBERACC,
        SUBSCR,
        CALL,
    };

    Instruction(BasicBlock *containing, InstKind kind, sema::types::Type *type, Location loc)
        : Value(ValueKind::INST, type, loc), containing(containing), instkind(kind) {}

    Instruction(
        BasicBlock *containing, InstKind kind, sema::types::Type *type, Optional<Location> loc)
        : Value(ValueKind::INST, type, loc), containing(containing), instkind(kind) {}

    Instruction(BasicBlock *containing, InstKind kind, sema::types::Type *type)
        : Value(ValueKind::INST, type), containing(containing), instkind(kind) {}

    Instruction(
        BasicBlock *containing, InstKind kind, sema::types::Type *type, std::string name,
        Location loc)
        : Value(ValueKind::INST, type, std::move(name), loc), containing(containing),
          instkind(kind) {}

    Instruction(
        BasicBlock *containing, InstKind kind, sema::types::Type *type, std::string name,
        Optional<Location> loc)
        : Value(ValueKind::INST, type, std::move(name), loc), containing(containing),
          instkind(kind) {}

    Instruction(BasicBlock *containing, InstKind kind, Location loc)
        : Value(ValueKind::INST, loc), containing(containing), instkind(kind) {}

    Instruction(BasicBlock *containing, InstKind kind)
        : Value(ValueKind::INST), containing(containing), instkind(kind) {}

    Instruction *as_instruction() override { return this; }

    /**
    The basic block that contains the instruction.
    */
    BasicBlock *containing;
    InstKind instkind;

    static bool classof(const Value *node) { return node->valkind == ValueKind::INST; }
};

class LoadInst : public CFGVisitable<LoadInst, Instruction> {
public:
    LoadInst(BasicBlock *containing, sema::types::Type *type, Value *addr, Location loc)
        : CFGVisitable<LoadInst, Instruction>(containing, InstKind::LOAD, type, loc),
          address(addr) {}

    LoadInst(BasicBlock *containing, sema::types::Type *type, Value *addr, Optional<Location> loc)
        : CFGVisitable<LoadInst, Instruction>(containing, InstKind::LOAD, type, loc),
          address(addr) {}

    Value *address;

    static bool classof(const Value *node) {
        if (!Instruction::classof(node)) {
            return false;
        }
        const auto *inst = static_cast<const Instruction *>(node); // NOLINT(*-static-cast-downcast)
        return inst->instkind == InstKind::LOAD;
    }
};

class StoreInst : public CFGVisitable<StoreInst, Instruction> {
public:
    StoreInst(
        BasicBlock *containing, sema::types::TypeContext& tyctxt, Value *address, Value *value,
        Location loc)
        : CFGVisitable<StoreInst, Instruction>(containing, InstKind::STORE, tyctxt.get_void(), loc),
          address(address), value(value) {}

    StoreInst(
        BasicBlock *containing, sema::types::TypeContext& tyctxt, Value *address, Value *value,
        Optional<Location> loc)
        : CFGVisitable<StoreInst, Instruction>(containing, InstKind::STORE, tyctxt.get_void(), loc),
          address(address), value(value) {}

    StoreInst(
        BasicBlock *containing, sema::types::TypeContext& tyctxt, Value *address, Value *value)
        : CFGVisitable<StoreInst, Instruction>(containing, InstKind::STORE, tyctxt.get_void()),
          address(address), value(value) {}

    Value *address;
    Value *value;

    static bool classof(const Value *node) {
        if (!Instruction::classof(node)) {
            return false;
        }
        const auto *inst = static_cast<const Instruction *>(node); // NOLINT(*-static-cast-downcast)
        return inst->instkind == InstKind::STORE;
    }
};

/**
The SSA phi function instruction.
*/
class PhiInst : public CFGVisitable<PhiInst, Instruction> {
public:
    PhiInst(BasicBlock *containing, sema::types::Type *type, Location loc)
        : CFGVisitable<PhiInst, Instruction>(containing, InstKind::PHI, type, loc) {}

    PhiInst(BasicBlock *containing, sema::types::Type *type, Optional<Location> loc)
        : CFGVisitable<PhiInst, Instruction>(containing, InstKind::PHI, type, loc) {}

    void add_incoming(Value *value, BasicBlock *block) { incoming.emplace_back(value, block); }

    Vec<Pair<Value *, BasicBlock *>> incoming;

    static bool classof(const Value *node) {
        if (!Instruction::classof(node)) {
            return false;
        }
        const auto *inst = static_cast<const Instruction *>(node); // NOLINT(*-static-cast-downcast)
        return inst->instkind == InstKind::PHI;
    }
};

class MemcpyInst : public CFGVisitable<MemcpyInst, Instruction> {
public:
    MemcpyInst(
        BasicBlock *containing, sema::types::TypeContext& tyctxt, Value *to, Value *from, size_t n)
        : CFGVisitable<MemcpyInst, Instruction>(containing, InstKind::MEMCPY, tyctxt.get_void()),
          to(to), from(from), n(n) {}

    Value *to, *from;
    size_t n;

    static bool classof(const Value *node) {
        if (!Instruction::classof(node)) {
            return false;
        }
        const auto *inst = static_cast<const Instruction *>(node); // NOLINT(*-static-cast-downcast)
        return inst->instkind == InstKind::MEMCPY;
    }
};

class PrintInst : public CFGVisitable<PrintInst, Instruction> {
public:
    PrintInst(
        BasicBlock *containing, sema::types::TypeContext& tyctxt, String *format, Vec<Value *> args,
        Location loc)
        : CFGVisitable<PrintInst, Instruction>(containing, InstKind::PRINT, tyctxt.get_void(), loc),
          format_string(format), args(std::move(args)) {}

    PrintInst(
        BasicBlock *containing, sema::types::TypeContext& tyctxt, String *format, Vec<Value *> args,
        Optional<Location> loc)
        : CFGVisitable<PrintInst, Instruction>(containing, InstKind::PRINT, tyctxt.get_void(), loc),
          format_string(format), args(std::move(args)) {}

    String *format_string;
    Vec<Value *> args;

    void add_arg(Value *arg) { args.push_back(arg); }

    static bool classof(const Value *node) {
        if (!Instruction::classof(node)) {
            return false;
        }
        const auto *inst = static_cast<const Instruction *>(node); // NOLINT(*-static-cast-downcast)
        return inst->instkind == InstKind::PRINT;
    }
};

class BinaryInst : public CFGVisitable<BinaryInst, Instruction> {
public:
    /**
    The operators that can appear inside a binary instruction.

    This is used instead of tokens::BinaryOp because some of those operators
    (namely, logical operators and BINCOMMA) get lowered into other constructs
    instead of a single binary instruction.
    */
    enum class Operator : uint8_t {
        OR,
        XOR,
        AND,
        EQ,
        NE,
        LT,
        GT,
        LE,
        GE,
        SHL,
        SHR,
        ADD,
        SUB,
        MUL,
        DIV,
        MOD,
    };

    BinaryInst(
        BasicBlock *containing, sema::types::Type *type, Operator op, Value *loperand,
        Value *roperand, Location loc)
        : CFGVisitable<BinaryInst, Instruction>(containing, InstKind::BINARY, type, loc), op(op),
          loperand(loperand), roperand(roperand) {}

    BinaryInst(
        BasicBlock *containing, sema::types::Type *type, Operator op, Value *loperand,
        Value *roperand, Optional<Location> loc)
        : CFGVisitable<BinaryInst, Instruction>(containing, InstKind::BINARY, type, loc), op(op),
          loperand(loperand), roperand(roperand) {}

    Operator op;
    Value *loperand, *roperand;

    /**
    Get the Operator corresponding to `op`.

    Throws std::runtime_error if an invalid BinaryOp is provided (logical operator / BINCOMMA).
    */
    static Operator op_from_token(tokens::BinaryOp op);

    static bool classof(const Value *node) {
        if (!Instruction::classof(node)) {
            return false;
        }
        const auto *inst = static_cast<const Instruction *>(node); // NOLINT(*-static-cast-downcast)
        return inst->instkind == InstKind::BINARY;
    }
};

class UnaryInst : public CFGVisitable<UnaryInst, Instruction> {
public:
    /**
    The operators that can appear inside an unary instruction.

    This is used instead of tokens::UnaryOp because some of those operators
    (namely, REF, DEREF, INC and DEC) get lowered into other constructs
    instead of a single unary instruction.
    */
    enum class Operator : uint8_t {
        POS,
        NEG,
        TILDE,
        NOT,
    };

    UnaryInst(
        BasicBlock *containing, sema::types::Type *type, Operator op, Value *operand, Location loc)
        : CFGVisitable<UnaryInst, Instruction>(containing, InstKind::UNARY, type, loc), op(op),
          operand(operand) {}

    UnaryInst(
        BasicBlock *containing, sema::types::Type *type, Operator op, Value *operand,
        Optional<Location> loc)
        : CFGVisitable<UnaryInst, Instruction>(containing, InstKind::UNARY, type, loc), op(op),
          operand(operand) {}

    Operator op;
    Value *operand;

    /**
    Get the Operator corresponding to `op`.

    Throws std::runtime_error if an invalid UnaryOp is provided (REF, DEREF, INC or DEC).
    */
    static Operator op_from_token(tokens::UnaryOp op);

    static bool classof(const Value *node) {
        if (!Instruction::classof(node)) {
            return false;
        }
        const auto *inst = static_cast<const Instruction *>(node); // NOLINT(*-static-cast-downcast)
        return inst->instkind == InstKind::UNARY;
    }
};

class IncrInst : public CFGVisitable<IncrInst, Instruction> {
public:
    IncrInst(BasicBlock *containing, sema::types::Type *type, Value *operand, Location loc)
        : CFGVisitable<IncrInst, Instruction>(containing, InstKind::INC, type, loc),
          operand(operand) {}

    IncrInst(
        BasicBlock *containing, sema::types::Type *type, Value *operand, Optional<Location> loc)
        : CFGVisitable<IncrInst, Instruction>(containing, InstKind::INC, type, loc),
          operand(operand) {}

    Value *operand;

    static bool classof(const Value *node) {
        if (!Instruction::classof(node)) {
            return false;
        }
        const auto *inst = static_cast<const Instruction *>(node); // NOLINT(*-static-cast-downcast)
        return inst->instkind == InstKind::INC;
    }
};

class DecrInst : public CFGVisitable<DecrInst, Instruction> {
public:
    DecrInst(BasicBlock *containing, sema::types::Type *type, Value *operand, Location loc)
        : CFGVisitable<DecrInst, Instruction>(containing, InstKind::DEC, type, loc),
          operand(operand) {}

    DecrInst(
        BasicBlock *containing, sema::types::Type *type, Value *operand, Optional<Location> loc)
        : CFGVisitable<DecrInst, Instruction>(containing, InstKind::DEC, type, loc),
          operand(operand) {}

    Value *operand;

    static bool classof(const Value *node) {
        if (!Instruction::classof(node)) {
            return false;
        }
        const auto *inst = static_cast<const Instruction *>(node); // NOLINT(*-static-cast-downcast)
        return inst->instkind == InstKind::DEC;
    }
};

class CastInst : public CFGVisitable<CastInst, Instruction> {
public:
    CastInst(
        BasicBlock *containing, sema::types::Type *type, sema::types::Type *target, Value *operand,
        Location loc)
        : CFGVisitable<CastInst, Instruction>(containing, InstKind::CAST, type, loc),
          target(target), operand(operand) {}

    CastInst(
        BasicBlock *containing, sema::types::Type *type, sema::types::Type *target, Value *operand,
        Optional<Location> loc)
        : CFGVisitable<CastInst, Instruction>(containing, InstKind::CAST, type, loc),
          target(target), operand(operand) {}

    sema::types::Type *target;
    Value *operand;

    static bool classof(const Value *node) {
        if (!Instruction::classof(node)) {
            return false;
        }
        const auto *inst = static_cast<const Instruction *>(node); // NOLINT(*-static-cast-downcast)
        return inst->instkind == InstKind::CAST;
    }
};

class MemberAccInst : public CFGVisitable<MemberAccInst, Instruction> {
public:
    MemberAccInst(
        BasicBlock *containing, sema::types::Type *type, size_t member_idx, Value *operand,
        Location loc)
        : CFGVisitable<MemberAccInst, Instruction>(containing, InstKind::MEMBERACC, type, loc),
          member_idx(member_idx), operand(operand) {}

    MemberAccInst(
        BasicBlock *containing, sema::types::Type *type, size_t member_idx, Value *operand,
        Optional<Location> loc)
        : CFGVisitable<MemberAccInst, Instruction>(containing, InstKind::MEMBERACC, type, loc),
          member_idx(member_idx), operand(operand) {}

    size_t member_idx;
    Value *operand;

    static bool classof(const Value *node) {
        if (!Instruction::classof(node)) {
            return false;
        }
        const auto *inst = static_cast<const Instruction *>(node); // NOLINT(*-static-cast-downcast)
        return inst->instkind == InstKind::MEMBERACC;
    }
};

class SubscrInst : public CFGVisitable<SubscrInst, Instruction> {
public:
    SubscrInst(
        BasicBlock *containing, sema::types::Type *type, Value *index, Value *operand, Location loc)
        : CFGVisitable<SubscrInst, Instruction>(containing, InstKind::SUBSCR, type, loc),
          index(index), operand(operand) {}

    SubscrInst(
        BasicBlock *containing, sema::types::Type *type, Value *index, Value *operand,
        Optional<Location> loc)
        : CFGVisitable<SubscrInst, Instruction>(containing, InstKind::SUBSCR, type, loc),
          index(index), operand(operand) {}

    Value *index;
    Value *operand;

    static bool classof(const Value *node) {
        if (!Instruction::classof(node)) {
            return false;
        }
        const auto *inst = static_cast<const Instruction *>(node); // NOLINT(*-static-cast-downcast)
        return inst->instkind == InstKind::SUBSCR;
    }
};

class CallInst : public CFGVisitable<CallInst, Instruction> {
public:
    CallInst(
        BasicBlock *containing, sema::types::Type *type, Value *operand, Vec<Value *> args,
        Location loc)
        : CFGVisitable<CallInst, Instruction>(containing, InstKind::CALL, type, loc),
          operand(operand), args(std::move(args)) {}

    CallInst(
        BasicBlock *containing, sema::types::Type *type, Value *operand, Vec<Value *> args,
        Optional<Location> loc)
        : CFGVisitable<CallInst, Instruction>(containing, InstKind::CALL, type, loc),
          operand(operand), args(std::move(args)) {}

    Value *operand;
    Vec<Value *> args;

    static bool classof(const Value *node) {
        if (!Instruction::classof(node)) {
            return false;
        }
        const auto *inst = static_cast<const Instruction *>(node); // NOLINT(*-static-cast-downcast)
        return inst->instkind == InstKind::CALL;
    }
};

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
        SWITCH,
        RETURN,
    };

    Terminator(Kind kind, BasicBlock *terminating) : kind(kind), terminating_blk(terminating) {}
    virtual ~Terminator() = default;

    Kind kind;
    BasicBlock *terminating_blk;

    virtual If *as_if() { return nullptr; }
    virtual Goto *as_goto() { return nullptr; }
    virtual Return *as_return() { return nullptr; }
    virtual Switch *as_switch() { return nullptr; }

    virtual void accept(CFGVisitor& visitor) = 0;

protected:
    virtual void abstract() = 0;
};

class NonReturn : public Terminator {
public:
    NonReturn(Kind kind, BasicBlock *terminating) : Terminator(kind, terminating) {}

    static bool classof(const Terminator *node) {
        switch (node->kind) {
        case Kind::IF:
        case Kind::GOTO:
        case Kind::SWITCH:
            return true;
        case Kind::RETURN:
            return false;
        }
    }
};

class If : public NonReturn {
public:
    If(BasicBlock *termng, Value *cond) : NonReturn(Kind::IF, termng), cond(cond) {}

    Value *cond;

    BasicBlock *then_br = nullptr;
    BasicBlock *else_br = nullptr;

    /**
    Sets and links the then-branch of the terminator with `blk`.
    */
    void set_then_target(BasicBlock *blk);

    /**
    Sets and links the else-branch of the terminator with `blk`.
    */
    void set_else_target(BasicBlock *blk);

    If *as_if() override { return this; }

    void accept(CFGVisitor& visitor) override;

    static bool classof(const Terminator *node) { return node->kind == Kind::IF; }

protected:
    void abstract() override {}
};

class Goto : public NonReturn {
public:
    Goto(BasicBlock *termng) : NonReturn(Kind::GOTO, termng) {}

    BasicBlock *target = nullptr;

    /**
    Sets and links the target of the terminator with `blk`.
    */
    void set_target(BasicBlock *blk);

    Goto *as_goto() override { return this; }

    void accept(CFGVisitor& visitor) override;

    static bool classof(const Terminator *node) { return node->kind == Kind::GOTO; }

protected:
    void abstract() override {}
};

/**
A class marking a possible case for the switch jump table.
*/
class SwitchCase {
public:
    SwitchCase(BasicBlock *blk, eval::Value val) : case_val(val), blk(blk) {}

    SwitchCase(BasicBlock *blk) : blk(blk) {}

    bool is_default() const { return !case_val.has_value(); }

    Optional<eval::Value> case_val;
    BasicBlock *blk;
};

/**
The Switch terminator.
*/
class Switch : public NonReturn {
public:
    Switch(BasicBlock *termng, Value *control)
        : NonReturn(Kind::SWITCH, termng), control(control) {}

    void add_case(eval::Value& val, BasicBlock *blk);

    void add_default(BasicBlock *blk);

    size_t num_cases() const { return cases.size(); }

    Value *control;
    Vec<SwitchCase> cases;

    Switch *as_switch() override { return this; }

    void accept(CFGVisitor& visitor) override;

    static bool classof(const Terminator *node) { return node->kind == Kind::SWITCH; }

protected:
    void abstract() override {}
};

class Return : public Terminator {
public:
    Return(BasicBlock *termng) : Terminator(Kind::RETURN, termng) {}

    Return(BasicBlock *termng, Value *ret) : Terminator(Kind::RETURN, termng), ret_value(ret) {}

    bool is_void() const { return !ret_value.has_value(); }

    void set_ret_value(Value *ret) { ret_value = ret; }

    Optional<Value *> ret_value;

    Return *as_return() override { return this; }

    void accept(CFGVisitor& visitor) override;

    static bool classof(const Terminator *node) { return node->kind == Kind::RETURN; }

protected:
    void abstract() override {}
};

/**
An abstract class for iterating over the successors of a block.
One concrete class is implemented for each terminator.
*/
class BasicBlockTermSuccIter : public NextIterator<BasicBlock *> {};

class BasicBlockIfSuccIter : public BasicBlockTermSuccIter {
    If *i;

    enum State : uint8_t {
        TRUE,
        FALSE,
        DONE,
    } state = TRUE;

public:
    BasicBlockIfSuccIter(If *i) : i(i) {}

    BasicBlock *next() override;
};

class BasicBlockGotoSuccIter : public BasicBlockTermSuccIter {
    Goto *g;
    bool iterated = false;

public:
    BasicBlockGotoSuccIter(Goto *g) : g(g) {}

    BasicBlock *next() override;
};

class BasicBlockSwitchSuccIter : public BasicBlockTermSuccIter {
    Switch *sw;
    size_t idx = 0;

public:
    BasicBlockSwitchSuccIter(Switch *sw) : sw(sw) {}

    BasicBlock *next() override;
};

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"

class BasicBlockReturnSuccIter : public BasicBlockTermSuccIter {
    Return *ret;

public:
    BasicBlockReturnSuccIter(Return *ret) : ret(ret) {}

    BasicBlock *next() override { return nullptr; }
};

#pragma clang diagnostic pop

/**
An iterator class that wraps a BasicBlockTermSuccIter.
*/
class BasicBlockSuccIter {
    BasicBlockTermSuccIter *iter = nullptr;
    BasicBlock *curr             = nullptr;

public:
    using difference_type = std::ptrdiff_t;
    using value_type      = BasicBlock *;

    BasicBlockSuccIter(BasicBlockTermSuccIter *iter) : iter(iter), curr(iter->next()) {}

    BasicBlockSuccIter() {}

    BasicBlock *operator*() const { return curr; }

    BasicBlockSuccIter& operator++() {
        curr = iter->next();

        return *this;
    }

    BasicBlockSuccIter operator++(int) {
        BasicBlockSuccIter tmp = *this;
        curr                   = iter->next();

        return tmp;
    }

    bool operator==(const BasicBlockSuccIter& other) const { return curr == other.curr; }
};

class BasicBlockSuccessors {
    Box<BasicBlockTermSuccIter> iter;

public:
    BasicBlockSuccessors(Terminator *term) {
        if (isa<If>(term)) {
            iter = std::make_unique<BasicBlockIfSuccIter>(term->as_if());
        } else if (isa<Goto>(term)) {
            iter = std::make_unique<BasicBlockGotoSuccIter>(term->as_goto());
        } else if (isa<Switch>(term)) {
            iter = std::make_unique<BasicBlockSwitchSuccIter>(term->as_switch());
        } else if (isa<Return>(term)) {
            iter = std::make_unique<BasicBlockReturnSuccIter>(term->as_return());
        }
    }

    BasicBlockSuccIter begin() { return BasicBlockSuccIter(iter.get()); }

    BasicBlockSuccIter end() { return BasicBlockSuccIter(); }
};

/**
The basic unit of the CFG.
*/
class BasicBlock : public ds::LinkedListNode<BasicBlock> {
    // Whether the block is an entry block into a function.
    bool is_entry = false;

    /**
    The function containing this block.
    */
    FunctionCFG *parent;

public:
    friend class FunctionCFG;

    BasicBlock(FunctionCFG *func) : parent(func) {}

    BasicBlock(std::string& label, FunctionCFG *func) : parent(func), label(label), name(label) {}

    static Box<BasicBlock> entry(std::string& func_name, FunctionCFG *func);

    template <typename Term, typename... Args>
        requires std::derived_from<Term, Terminator>
    Term *terminate(Args&&...args) {
        Box<Term> terminator = std::make_unique<Term>(this, std::forward<Args>(args)...);
        Term *ret            = terminator.get();

        term = std::move(terminator);

        return ret;
    }

    bool is_terminated() const { return term != nullptr; }

    bool is_empty() const { return instructions.empty(); }

    bool has_label() const { return label.has_value(); }

    BasicBlock *next_block() const { return next(); }

    BasicBlock *prev_block() const { return prev(); }

    Terminator *terminator() const { return term.get(); }

    void set_label(std::string&& label) { this->label = name = std::move(label); }

    void set_label(const std::string& label) { this->label = name = label; }

    template <typename Inst, typename... Args>
        requires std::derived_from<Inst, Instruction>
    Inst *add_instruction(Args&&...args) {
        if constexpr (std::is_same_v<Inst, PhiInst>) {
            if (!instructions.empty() &&
                instructions.last().instkind != Instruction::InstKind::PHI) {
                throw std::runtime_error("PhiInst must be the first instruction in a block");
            }
        }

        Box<Inst> inst = std::make_unique<Inst>(this, std::forward<Args>(args)...);
        Inst *ret      = inst.get();

        push_instruction(std::move(inst));

        return ret;
    }

    template <typename Val, typename... Args>
        requires(std::derived_from<Val, Value> && !std::derived_from<Val, Instruction>)
    Val *add_value(Args... args) {
        Box<Val> val = std::make_unique<Val>(args...);
        Val *ret     = val.get();

        push_value(std::move(val));

        return ret;
    }

    Value *insert_value(Box<Value> val);

    /**
    Retrieve the first non-Phi instruction in this block.
    */
    Instruction *first_non_phi_inst() const;

    /**
    Retrieve the function containing this block.
    */
    FunctionCFG *get_parent() { return parent; }

    /**
    Links `target` as a successor block to `this`.
    */
    void link_to(BasicBlock *target);

    /**
    Return the number of successors to this block
    */
    size_t num_successors() const { return succs.size(); }

    /**
    Return the number of incoming blocks to this block.
    */
    size_t num_incoming() const { return incoming.size(); }

    /**
    An iterator over the successor blocks to this block.
    */
    BasicBlockSuccessors successors() { return BasicBlockSuccessors(term.get()); }

    /**
    An explicit, user-set label.
    */
    Optional<std::string> label;

    /**
    An internal compiler-assigned name. If label is set, this will be label.
    */
    std::string name;

    /**
    The blocks incoming to this block.
    */
    Vec<BasicBlock *> incoming;

    auto begin() const { return instructions.begin(); }

    auto end() const { return instructions.end(); }

private:
    /**
    The instructions that make up the block.
    */
    ds::LinkedList<Instruction> instructions;

    /**
    The blocks going out from this block.
    */
    Vec<BasicBlock *> succs;
    Box<Terminator> term = nullptr;
    void push_instruction(Box<Instruction> inst) { instructions.push_back(std::move(inst)); }

    void push_value(Box<Value> val);
};

/**
A single function, composed of linked blocks.
*/
class FunctionCFG : public CFGVisitable<FunctionCFG, Constant> {
public:
    friend class BasicBlock;

    FunctionCFG(sema::types::FunctionType *sig, std::string name)
        : CFGVisitable<FunctionCFG, Constant>(ValueKind::FUNC, sig, std::move(name)),
          signature(sig) {}

    sema::types::FunctionType *get_signature() { return signature; }

    /**
    Initialize the FunctionCFG, returning a pointer to the entry block.
    */
    BasicBlock *initialize();

    const std::string& get_name() { return name; }

    FuncArg *add_arg(sema::types::Type *type);

    FuncArg *arg_idx(size_t idx) { return idx < args.size() ? args[idx].get() : nullptr; }

    BasicBlock *entry_block() { return entry; }

    /**
    Whether the FunctionCFG has been initialized.
    */
    bool is_initialized() { return !blocks.empty(); }

    /**
    Whether the FunctionCFG is defined in this TU.
    */
    bool is_defined() { return entry != nullptr && !blocks.empty(); }

    /**
    Whether the function is a simple plain return.
    */
    bool is_plain_return() const;

    /**
    Insert an anonymous block.
    */
    BasicBlock *create_block();

    /**
    Insert a named block, optionally making it a labeled block.
    */
    BasicBlock *create_block(std::string& name, bool make_labeled = false);

    /**
    Insert an anonymous block before `succ`.
    */
    BasicBlock *create_block_before(BasicBlock *succ);

    /**
    Insert a named block before `succ`, optionally making it a labeled block.
    */
    BasicBlock *create_block_before(BasicBlock *succ, std::string& name, bool make_labeled = false);

    BasicBlock *create_block_after(BasicBlock *prec);

    BasicBlock *create_block_after(BasicBlock *prec, std::string& name, bool make_labeled = false);

    void swap_blocks(BasicBlock *first, BasicBlock *second);

    BasicBlock *lookup_labeled_block(std::string& label);

    size_t num_blocks() const { return blocks.size(); }

    void remove_block(BasicBlock *blk);

    Alloca *add_alloca(sema::types::Type *type, std::string name);

    /**
    Add an anonymous Alloca, typically as a temporary spill variable.
    */
    Alloca *add_alloca(sema::types::Type *type);

    /**
    An iterator over the allocations in the function, in allocation order.
    */
    Span<Box<Alloca>> get_allocas();

    Span<Box<FuncArg>> get_args() { return args; }

    auto begin() const { return blocks.begin(); }

    auto end() const { return blocks.end(); }

    static bool classof(const Value *node) { return node->valkind == ValueKind::FUNC; }

private:
    sema::types::FunctionType *signature;

    Vec<Box<FuncArg>> args;

    BasicBlock *entry = nullptr;
    ds::LinkedList<BasicBlock> blocks;

    HashMap<std::string, BasicBlock *> labeled_blocks;

    // The allocations in the function.
    Vec<Box<Alloca>> allocas;

    // Bag of non-instruction values.
    Vec<Box<Value>> values;
};

class ProgramCFG {
public:
    ProgramCFG() {}

    /**
    Adds a new global to the ProgramCFG corresponding to the passed LIRVarSym,
    or returns the corresponding FunctionCFG if it already exists.
    */
    Global *add_global(sema::types::Type *type, std::string name, Value *init = nullptr);

    FunctionCFG *add_function(sema::types::FunctionType *sig, std::string name);

    /**
    An iterator over the globals in the program, in the order they were added.
    */
    Span<Box<Global>> get_globals();

    Span<Box<FunctionCFG>> get_functions();

    /**
    Adds a new string to the ProgramCFG corresponding to the passed string,
    or returns the corresponding String if it already exists.
    */
    String *add_or_get_string(sema::types::ArrayType *type, const std::string& str);

    /**
    Constructs a new Constant of type T, owned by the ProgramCFG. Used for constant values that
    aren't tied to any particular function -- e.g. nodes in the constant initializer tree of a
    global variable.
    */
    template <typename T, typename... Args>
        requires std::derived_from<T, Constant>
    T *add_constant(Args... args) {
        auto c = std::make_unique<T>(args...);
        T *ret = c.get();

        constants.push_back(std::move(c));

        return ret;
    }

    HashMap<std::string, Box<String>> strings;

private:
    Vec<Box<FunctionCFG>> functions;

    Vec<Box<Global>> globals;

    // Bag of constant values not owned by any function (i.e. those appearing in globals'
    // initializer trees).
    Vec<Box<Constant>> constants;
};

} // end namespace ecc::lower::cfg

#endif
