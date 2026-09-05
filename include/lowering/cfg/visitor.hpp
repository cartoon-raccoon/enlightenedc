#pragma once

#ifndef ECC_INST_VISITOR_H
#define ECC_INST_VISITOR_H

#include "abstract/visitor.hpp"

namespace ecc::lower::cfg {

class Alloca;
class LoadInst;
class StoreInst;
class PhiInst;
class PrintInst;
class MemcpyInst;
class BinaryInst;
class UnaryInst;
class IncrInst;
class DecrInst;
class CastInst;
class MemberAccInst;
class SubscrInst;
class CallInst;

class ScalarConst;
class PointerConst;
class AggregateConst;
class ZeroConst;
class String;
class Global;
class FunctionCFG;
class FuncArg;

class If;
class Goto;
class Switch;
class Return;

class CFGVisitor : public Visitor<
                       CFGVisitor, Alloca, LoadInst, StoreInst, PhiInst, PrintInst, MemcpyInst,
                       BinaryInst, UnaryInst, IncrInst, DecrInst, CastInst, MemberAccInst,
                       SubscrInst, CallInst, ScalarConst, PointerConst, AggregateConst, ZeroConst,
                       FunctionCFG, Global, String, FuncArg, If, Goto, Switch, Return> {};

} // namespace ecc::lower::cfg

#endif