#pragma once

#ifndef ECC_CFG_H
#define ECC_CFG_H

#include <utility>

#include "codegen/lir/lir.hpp"
#include "eval/value.hpp"
#include "util.hpp"

namespace ecc::codegen::cfg {

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

    Terminator(Kind kind) : kind(kind) {}
    virtual ~Terminator() = default;

    Kind kind;

    virtual If *as_if() { return nullptr; }
    virtual Goto *as_goto() { return nullptr; }
    virtual Return *as_return() { return nullptr; }
    virtual Switch *as_switch() { return nullptr; }
};

class If : public Terminator {
public:
    BasicBlock *then_br = nullptr;
    BasicBlock *else_br = nullptr;
};

class Goto : public Terminator {
public:
    BasicBlock *target = nullptr;
};

class Return : public Terminator {
public:
    Optional<Box<lir::ExprLIR>> expr;
};

class Switch : public Terminator {
public:
    Vec<std::pair<eval::Value, BasicBlock *>> cases;
};

class BasicBlock {
public:

    bool has_terminator() { return term != nullptr; }

    void add_element(Box<lir::NonTerminalLIR> elem) {
        elements.push_back(std::move(elem));
    }

private:
    bool is_entry;
    bool is_part_of_loop;
    
    Optional<std::string> label;

    Vec<BasicBlock *> incoming;
    Vec<Box<lir::NonTerminalLIR>> elements;

    Box<Terminator> term = nullptr;
};

class FunctionCFG {
public:
    BasicBlock *create_block();

private:
    Vec<Box<BasicBlock>> blocks;
};

class ProgramCFG {
public:
};

} // end namespace ecc::codegen::cfg

#endif