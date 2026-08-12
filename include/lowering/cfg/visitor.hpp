#pragma once

#ifndef ECC_INST_VISITOR_H
#define ECC_INST_VISITOR_H

#include "lowering/cfg/cfg.hpp"

namespace ecc::lower::cfg {

class CFGValueVisitor {
public:
    virtual ~CFGValueVisitor() = default;

    virtual void visit(AllocaInst& inst)    = 0;
    virtual void visit(LoadInst& inst)      = 0;
    virtual void visit(StoreInst& inst)     = 0;
    virtual void visit(PhiInst& inst)       = 0;
    virtual void visit(PrintInst& inst)     = 0;
    virtual void visit(BinaryInst& inst)    = 0;
    virtual void visit(UnaryInst& inst)     = 0;
    virtual void visit(IncrInst& inst)      = 0;
    virtual void visit(DecrInst& inst)      = 0;
    virtual void visit(CastInst& inst)      = 0;
    virtual void visit(ReintInst& inst)     = 0;
    virtual void visit(MemberAccInst& inst) = 0;
    virtual void visit(SubscrInst& inst)    = 0;
    virtual void visit(CallInst& inst)      = 0;

    virtual void visit(FuncRef& val) = 0;
    virtual void visit(Literal& val) = 0;
    virtual void visit(Zero& val)    = 0;
};

} // namespace ecc::lower::cfg

#endif