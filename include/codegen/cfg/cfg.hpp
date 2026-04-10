#pragma once

#ifndef ECC_CFG_H
#define ECC_CFG_H

#include <utility>

#include "codegen/lir/lir.hpp"
#include "eval/value.hpp"
#include "util.hpp"

namespace ecc::codegen::cfg {

using namespace ecc;
using namespace util;

class BasicBlock;
class If;
class Goto;
class Return;
class Switch;

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
    If(BasicBlock *termng) : Terminator(Kind::IF, termng) {}

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
    SwitchCase(eval::Value val, BasicBlock *blk) : case_val(std::move(val)), blk(blk) {}

    SwitchCase(BasicBlock *blk) : blk(blk) {}

    bool is_default() const { return !case_val.has_value(); }

    Optional<eval::Value> case_val;
    BasicBlock *blk;
};

class Switch : public Terminator {
public:
    Switch(BasicBlock *termng) : Terminator(Kind::SWITCH, termng) {}

    void add_case(eval::Value val, BasicBlock *blk) { cases.emplace_back(std::move(val), blk); }

    void add_default(BasicBlock *blk) { cases.emplace_back(blk); }

    size_t num_cases() const { return cases.size(); }

    Vec<SwitchCase> cases;

    Switch *as_switch() override { return this; }
};

class BasicBlock : public NoCopy, public NoMove {
public:
    BasicBlock(std::string& label) : label(label) {}

    static Box<BasicBlock> entry(std::string& func_name);

    void set_terminator(Box<Terminator> term) { term = std::move(term); }

    bool has_terminator() { return term != nullptr; }

    void add_element(lir::NonTerminalLIR *elem) { elements.push_back(elem); }

private:
    bool is_entry        = false;
    bool is_part_of_loop = false;

    std::string label;

    Vec<BasicBlock *> incoming;
    Vec<lir::NonTerminalLIR *> elements;

    Box<Terminator> term = nullptr;
};

class FunctionCFG {
public:
    FunctionCFG(lir::FunctionLIR *func) : lir(func) {}

    lir::FunctionLIR *lir;

    BasicBlock *create_block(std::string& label);

    void append_block(Box<BasicBlock> block) { blocks.push_back(std::move(block)); }

    BasicBlock *lookup_block();

    size_t num_blocks() { return blocks.size(); }

    void remove_block(BasicBlock *blk);

private:
    Vec<Box<BasicBlock>> blocks;
};

class ProgramCFG {
public:
    ProgramCFG() {}

    FunctionCFG *add_function();

    Vec<Box<FunctionCFG>> functions;
};

} // end namespace ecc::codegen::cfg

#endif