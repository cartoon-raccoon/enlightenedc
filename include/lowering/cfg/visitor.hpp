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

class ScalarConst;
class AggregateConst;
class ZeroConst;
class FuncRef;
class String;
class Global;
class FuncArg;

class If;
class Goto;
class Switch;
class Return;

class CFGVisitor
    : public Visitor<
          CFGVisitor, AllocaInst, LoadInst, StoreInst, PhiInst, PrintInst, BinaryInst, UnaryInst,
          IncrInst, DecrInst, CastInst, ReintInst, MemberAccInst, SubscrInst, CallInst, ScalarConst,
          AggregateConst, ZeroConst, FuncRef, Global, String, FuncArg, If, Goto, Switch, Return> {};

} // namespace ecc::lower::cfg

#endif