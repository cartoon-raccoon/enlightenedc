#pragma once

#ifndef ECC_CFG_PRINTER_H
#define ECC_CFG_PRINTER_H

#include "lowering/cfg/cfg.hpp"
#include "lowering/cfg/visitor.hpp"

namespace ecc::lower::cfg {

/**
A class that prints the CFG, in linear function and block order.

CFGPrinter walks the function list in the order it was created,
and for each function, walks the blocks of the function in
creation order as well. It does not perform DFS (as of now).
*/
class CFGPrinter : public CFGVisitor {
    size_t indent = 0;

    /**
    Assigns names to the unlabeled blocks, unnamed instructions, and unnamed allocas of
    a single function.
    */
    void name_function(FunctionCFG& func);

public:
    void print(ProgramCFG& cfg);

    void print_function(FunctionCFG& func);

    void print_block(BasicBlock& blk);

    void print_value(Value& value);

    void print_value_name(Value& value);

    void visit(AllocaInst& inst) override;
    void visit(LoadInst& inst) override;
    void visit(StoreInst& inst) override;
    void visit(PhiInst& inst) override;
    void visit(PrintInst& inst) override;
    void visit(BinaryInst& inst) override;
    void visit(UnaryInst& inst) override;
    void visit(IncrInst& inst) override;
    void visit(DecrInst& inst) override;
    void visit(CastInst& inst) override;
    void visit(ReintInst& inst) override;
    void visit(MemberAccInst& inst) override;
    void visit(SubscrInst& inst) override;
    void visit(CallInst& inst) override;

    void visit(ScalarConst& val) override;
    void visit(AggregateConst& val) override;
    void visit(ZeroConst& val) override;
    void visit(FuncRef& val) override;
    void visit(Global& val) override;
    void visit(String& val) override;
    void visit(FuncArg& val) override;

    void visit(If& term) override;
    void visit(Goto& term) override;
    void visit(Switch& term) override;
    void visit(Return& term) override;
};

} // namespace ecc::lower::cfg

#endif