#pragma once

#ifndef ECC_INST_VISITOR_H
#define ECC_INST_VISITOR_H

#include "abstract/visitor.hpp"

namespace ecc::lower::cfg {

class AllocaInst;
class LoadInst;
class StoreInst;
class PhiInst;
class PrintInst;
class BinaryInst;
class UnaryInst;
class IncrInst;
class DecrInst;
class CastInst;
class ReintInst;
class MemberAccInst;
class SubscrInst;
class CallInst;

class FuncRef;
class Literal;
class Zero;

class CFGValueVisitor : public Visitor<
                            CFGValueVisitor, AllocaInst, LoadInst, StoreInst, PhiInst, PrintInst,
                            BinaryInst, UnaryInst, IncrInst, DecrInst, CastInst, ReintInst,
                            MemberAccInst, SubscrInst, CallInst, FuncRef, Literal, Zero> {};

} // namespace ecc::lower::cfg

#endif