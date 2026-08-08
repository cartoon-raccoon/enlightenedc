#pragma once

#ifndef ECC_CFG_H
#define ECC_CFG_H

#include <concepts>
#include <utility>
#include <stdexcept>
#include <variant>

#include "codegen/lir/lir.hpp"
#include "codegen/lir/symbols.hpp"
#include "ds/linkedlist.hpp"
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

class BasicBlock;
class If;
class Goto;
class Return;
class Switch;

class FunctionCFG;

/**
A CFG value.
*/
class Value : public NoCopy {
public:
    enum class ValueKind : uint8_t {
        INST,
        FUNC,
        LIT,
        ZERO,
    };

    Value(ValueKind kind, sema::types::Type *type, Location loc)
        : valkind(kind), type(type), eff_type(type->effective_type()), loc(loc) {}

    Value(ValueKind kind, Location loc) : valkind(kind), loc(loc) {}

    Value(ValueKind kind, sema::types::Type *type)
        : valkind(kind), type(type), eff_type(type->effective_type()) {}

    Value(ValueKind kind) : valkind(kind) {}

    ValueKind valkind;
    sema::types::Type *type     = nullptr;
    sema::types::Type *eff_type = nullptr;
    Optional<Location> loc;

    virtual ~Value() = default;

    virtual Instruction *as_instruction() { return nullptr; }
    virtual FuncRef *as_funcref() { return nullptr; }
    virtual Literal *as_literal() { return nullptr; }
    virtual Zero *as_zero() { return nullptr; }
};

/**
A unit of execution in the CFG IR.
*/
class Instruction : public Value {
public:
    enum class InstKind : uint8_t {
        ALLOCA,
        LOAD,
        STORE,
        PHI,
        PRINT,
        BINARY,
        UNARY,
        INC,
        DEC,
        CAST,
        REINT,
        MEMBERACC,
        SUBSCR,
        CALL,
    };

    Instruction(BasicBlock *containing, InstKind kind, sema::types::Type *type, Location loc)
        : Value(ValueKind::INST, type, loc), containing(containing), instkind(kind) {}

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

class AllocaInst : public Instruction {
public:
    AllocaInst(BasicBlock *containing, sema::types::Type *type, lir::LIRVarSym *sym)
        : Instruction(containing, InstKind::ALLOCA, type, sym->sym->loc), sym(sym) {}

    lir::LIRVarSym *sym;

    static bool classof(const Value *node) {
        if (!Instruction::classof(node)) {
            return false;
        }
        const auto *inst = static_cast<const Instruction *>(node); // NOLINT(*-static-cast-downcast)
        return inst->instkind == InstKind::ALLOCA;
    }
};

class LoadInst : public Instruction {
public:
    LoadInst(BasicBlock *containing, sema::types::Type *type, Value *addr, Location loc)
        : Instruction(containing, InstKind::LOAD, type, loc), address(addr) {}

    Value *address;

    static bool classof(const Value *node) {
        if (!Instruction::classof(node)) {
            return false;
        }
        const auto *inst = static_cast<const Instruction *>(node); // NOLINT(*-static-cast-downcast)
        return inst->instkind == InstKind::LOAD;
    }
};

class StoreInst : public Instruction {
public:
    StoreInst(
        BasicBlock *containing, sema::types::TypeContext& tyctxt, Value *address, Value *value,
        Location loc)
        : Instruction(containing, InstKind::STORE, tyctxt.get_void(), loc), address(address),
          value(value) {}

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
class PhiInst : public Instruction {
public:
    PhiInst(BasicBlock *containing, sema::types::Type *type, Location loc)
        : Instruction(containing, InstKind::PHI, type, loc) {}

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

class PrintInst : public Instruction {
public:
    PrintInst(BasicBlock *containing, sema::types::TypeContext& tyctxt, Location loc)
        : Instruction(containing, InstKind::PRINT, tyctxt.get_void(), loc) {}

    PrintInst(
        BasicBlock *containing, sema::types::TypeContext& tyctxt, std::string format,
        Vec<Value *> args, Location loc)
        : Instruction(containing, InstKind::PRINT, tyctxt.get_void(), loc),
          format_string(std::move(format)), args(std::move(args)) {}

    std::string format_string;
    Vec<Value *> args;

    void set_format(std::string& str) { format_string = str; }

    void add_arg(Value *arg) { args.push_back(arg); }

    static bool classof(const Value *node) {
        if (!Instruction::classof(node)) {
            return false;
        }
        const auto *inst = static_cast<const Instruction *>(node); // NOLINT(*-static-cast-downcast)
        return inst->instkind == InstKind::PRINT;
    }
};

class BinaryInst : public Instruction {
public:
    BinaryInst(
        BasicBlock *containing, sema::types::Type *type, tokens::BinaryOp op, Value *loperand,
        Value *roperand, Location loc)
        : Instruction(containing, InstKind::BINARY, type, loc), op(op), loperand(loperand),
          roperand(roperand) {}

    tokens::BinaryOp op;
    Value *loperand, *roperand;

    static bool classof(const Value *node) {
        if (!Instruction::classof(node)) {
            return false;
        }
        const auto *inst = static_cast<const Instruction *>(node); // NOLINT(*-static-cast-downcast)
        return inst->instkind == InstKind::BINARY;
    }
};

class UnaryInst : public Instruction {
public:
    UnaryInst(
        BasicBlock *containing, sema::types::Type *type, tokens::UnaryOp op, Value *operand,
        Location loc)
        : Instruction(containing, InstKind::UNARY, type, loc), op(op), operand(operand) {}

    tokens::UnaryOp op;
    Value *operand;

    static bool classof(const Value *node) {
        if (!Instruction::classof(node)) {
            return false;
        }
        const auto *inst = static_cast<const Instruction *>(node); // NOLINT(*-static-cast-downcast)
        return inst->instkind == InstKind::UNARY;
    }
};

class IncrInst : public Instruction {
public:
    IncrInst(BasicBlock *containing, sema::types::Type *type, Value *operand, Location loc)
        : Instruction(containing, InstKind::INC, type, loc), operand(operand) {}

    Value *operand;

    static bool classof(const Value *node) {
        if (!Instruction::classof(node)) {
            return false;
        }
        const auto *inst = static_cast<const Instruction *>(node); // NOLINT(*-static-cast-downcast)
        return inst->instkind == InstKind::INC;
    }
};

class DecrInst : public Instruction {
public:
    DecrInst(BasicBlock *containing, sema::types::Type *type, Value *operand, Location loc)
        : Instruction(containing, InstKind::DEC, type, loc), operand(operand) {}

    Value *operand;

    static bool classof(const Value *node) {
        if (!Instruction::classof(node)) {
            return false;
        }
        const auto *inst = static_cast<const Instruction *>(node); // NOLINT(*-static-cast-downcast)
        return inst->instkind == InstKind::DEC;
    }
};

class CastInst : public Instruction {
public:
    CastInst(
        BasicBlock *containing, sema::types::Type *type, sema::types::Type *target, Value *operand,
        Location loc)
        : Instruction(containing, InstKind::CAST, type, loc), target(target), operand(operand) {}

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

class ReintInst : public Instruction {
public:
    ReintInst(
        BasicBlock *containing, sema::types::Type *type, tokens::PrimType target, Value *operand,
        Location loc)
        : Instruction(containing, InstKind::REINT, type, loc), target(target), operand(operand) {}

    tokens::PrimType target;
    Value *operand;

    static bool classof(const Value *node) {
        if (!Instruction::classof(node)) {
            return false;
        }
        const auto *inst = static_cast<const Instruction *>(node); // NOLINT(*-static-cast-downcast)
        return inst->instkind == InstKind::REINT;
    }
};

class MemberAccInst : public Instruction {
public:
    MemberAccInst(
        BasicBlock *containing, sema::types::Type *type, size_t member_idx, Value *operand,
        Location loc)
        : Instruction(containing, InstKind::MEMBERACC, type, loc), member_idx(member_idx),
          operand(operand) {}

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

class SubscrInst : public Instruction {
public:
    SubscrInst(
        BasicBlock *containing, sema::types::Type *type, Value *index, Value *operand, Location loc)
        : Instruction(containing, InstKind::SUBSCR, type, loc), index(index), operand(operand) {}

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

class CallInst : public Instruction {
public:
    CallInst(
        BasicBlock *containing, sema::types::Type *type, Value *operand, Vec<Value *> args,
        Location loc)
        : Instruction(containing, InstKind::CALL, type, loc), operand(operand),
          args(std::move(args)) {}

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
A value that holds a reference to a function symbol.

FuncRef holds a reference to a LIRFuncSym, to distinguish it from LIRVarSym,
which is the only symbol type Load can operate on.
*/
class FuncRef : public Value {
public:
    FuncRef(sema::types::FunctionType *sig, FunctionCFG *ref)
        : Value(ValueKind::FUNC, sig), func(ref) {}

    FunctionCFG *func;

    FuncRef *as_funcref() override { return this; }

    static bool classof(const Value *node) { return node->valkind == ValueKind::FUNC; }
};

class Literal : public Value {
public:
    using LiteralVariant = std::variant<eval::Value, std::string>;

    Literal(sema::types::Type *type, eval::Value& value)
        : Value(ValueKind::LIT, type), value(value) {}

    Literal(sema::types::Type *type, std::string value)
        : Value(ValueKind::LIT, type), value(std::move(value)) {}

    Literal(sema::types::Type *type, LiteralVariant value)
        : Value(ValueKind::LIT, type), value(std::move(value)) {}

    LiteralVariant value;

    bool is_string() const { return std::holds_alternative<std::string>(value); }

    bool is_value() const { return std::holds_alternative<eval::Value>(value); }

    Literal *as_literal() override { return this; }

    static bool classof(const Value *node) { return node->valkind == ValueKind::LIT; }
};

class Zero : public Value {
public:
    Zero(sema::types::Type *type) : Value(ValueKind::ZERO, type) {}

    Zero *as_zero() override { return this; }

    static bool classof(const Value *node) { return node->valkind == ValueKind::ZERO; }
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

    static bool classof(const Terminator *node) { return node->kind == Kind::RETURN; }

protected:
    void abstract() override {}
};

/**
The basic unit of the CFG.
*/
class BasicBlock : public ds::LinkedListNode<BasicBlock> {
    // Whether the block is an entry block into a function.
    bool is_entry = false;
    // Whether the block is part of a loop structure.
    bool is_part_of_loop = false;

    FunctionCFG *func;

public:
    friend class FunctionCFG;

    BasicBlock(FunctionCFG *func) : func(func) {}

    BasicBlock(std::string& label, FunctionCFG *func) : func(func), label(label) {}

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

    bool has_label() const { return label.has_value(); }

    BasicBlock *next_block() { return next(); }

    BasicBlock *prev_block() { return prev(); }

    Terminator *terminator() { return term.get(); }

    template <typename Inst, typename... Args>
        requires std::derived_from<Inst, Instruction>
    Inst *add_instruction(Args&&...args) {
        if constexpr (std::is_same_v<Inst, PhiInst>) {
            if (!instructions.empty() &&
                instructions.back()->instkind != Instruction::InstKind::PHI) {
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

    /**
    Links `target` as a successor block to `this`.
    */
    void link_to(BasicBlock *target);

    Optional<std::string> label;

    Vec<BasicBlock *> incoming;
    Vec<BasicBlock *> successors;
    // The instructions that make up the block.
    Vec<Box<Instruction>> instructions;

    
private:
    Box<Terminator> term = nullptr;
    void push_instruction(Box<Instruction> inst) { instructions.push_back(std::move(inst)); }

    void push_value(Box<Value> val);
};

/**
A single function, composed of linked blocks.
*/
class FunctionCFG {
public:
    friend class BasicBlock;

    FunctionCFG(lir::FunctionLIR *func) : lir(func) {}

    lir::FunctionLIR *lir = nullptr;

    /**
    Initialize the FunctionCFG, returning a pointer to the entry block.
    */
    BasicBlock *initialize();

    /**
    Whether the FunctionCFG has been initialized.
    */
    bool is_initialized() { return !blocks.empty(); }

    /**
    Insert an anonymous block.
    */
    BasicBlock *create_block();

    /**
    Insert a named block, optionally making it a labeled block.
    */
    BasicBlock *create_block(std::string& name, bool make_labeled = false);

    BasicBlock *create_block_before(BasicBlock *succ);

    BasicBlock *create_block_before(BasicBlock *succ, std::string& name, bool make_labeled = false);

    BasicBlock *create_block_after(BasicBlock *prec);

    BasicBlock *create_block_after(BasicBlock *prec, std::string& name, bool make_labeled = false);

    void swap_blocks(BasicBlock *first, BasicBlock *second);

    BasicBlock *lookup_labeled_block(std::string& label);

    /**
    Add a goto to `label` to be resolved later.
    */
    void add_pending_goto(std::string& label, Goto *g);

    /**
    Resolve all pending gotos with target `label` to `targ`.
    */
    size_t resolve_pending_gotos(std::string& label, BasicBlock *targ);

    size_t num_blocks() { return blocks.size(); }

    void remove_block(BasicBlock *blk);

    AllocaInst *lookup_alloca(lir::LIRVarSym *sym);

    AllocaInst *add_alloca(BasicBlock *blk, lir::LIRVarSym *sym);

    auto begin() { return blocks.begin(); }

    auto end() { return blocks.end(); }

private:
    ds::LinkedList<BasicBlock> blocks;

    HashMap<std::string, BasicBlock *> labeled_blocks;

    // The allocations in the function, one per LIRVarSym.
    //
    // Vec is used to preserve allocation order.
    Vec<lir::LIRVarSym *> alloca_order;
    HashMap<lir::LIRVarSym *, Box<AllocaInst>> allocas;

    // Bag of non-instruction values.
    Vec<Box<Value>> values;

    HashMap<std::string, Vec<Goto *>> pending_gotos;
};

class ProgramCFG {
public:
    ProgramCFG() : implicit_main(std::make_unique<FunctionCFG>(nullptr)) {}

    // The implicit main that all top-level program items go into.
    Box<FunctionCFG> implicit_main;

    /**
    Adds a new function to the ProgramCFG.
    */
    FunctionCFG *add_function(lir::FunctionLIR *func);

    /**
    Construct a FuncRef from a given FunctionLIR.
    */
    FuncRef *ref_function(lir::FunctionLIR *func);

    HashMap<lir::FunctionLIR *, Box<FunctionCFG>> functions;

private:
    Vec<Box<FuncRef>> funcrefs;
};

} // end namespace ecc::codegen::cfg

#endif
