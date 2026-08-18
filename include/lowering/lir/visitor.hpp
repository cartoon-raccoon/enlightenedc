#pragma once

#ifndef ECC_LIR_VISITOR_H
#define ECC_LIR_VISITOR_H

#include "abstract/visitor.hpp"

namespace ecc::lower::lir {

class ProgramLIR;
class FunctionLIR;
class VarDeclLIR;
class ScalarInitLIR;
class AggregateInitLIR;
class StringInitLIR;
class FuncInitLIR;
class ZeroInitLIR;
class LabelDeclLIR;
class CaseLIR;
class DefaultLIR;
class ExprStmtLIR;
class GotoStmtLIR;
class SwitchStmtLIR;
class BreakStmtLIR;
class ContStmtLIR;
class IfStmtLIR;
class LoopStmtLIR;
class PrintStmtLIR;
class ReturnStmtLIR;
class BinaryExprLIR;
class UnaryExprLIR;
class CastExprLIR;
class AssignExprLIR;
class CondExprLIR;
class IdentExprLIR;
class LiteralExprLIR;
class ZeroExprLIR;
class CallExprLIR;
class MemberAccExprLIR;
class ReintExprLIR;
class SubscrExprLIR;
class PostfixExprLIR;

class LIRVisitor
    : public Visitor<
          LIRVisitor, ProgramLIR, FunctionLIR, VarDeclLIR, ScalarInitLIR, AggregateInitLIR,
          StringInitLIR, FuncInitLIR, ZeroInitLIR, LabelDeclLIR, CaseLIR, DefaultLIR, ExprStmtLIR,
          GotoStmtLIR, SwitchStmtLIR, BreakStmtLIR, ContStmtLIR, IfStmtLIR, LoopStmtLIR,
          PrintStmtLIR, ReturnStmtLIR, BinaryExprLIR, UnaryExprLIR, CastExprLIR, AssignExprLIR,
          CondExprLIR, IdentExprLIR, LiteralExprLIR, ZeroExprLIR, CallExprLIR, MemberAccExprLIR,
          ReintExprLIR, SubscrExprLIR, PostfixExprLIR> {};

} // namespace ecc::lower::lir

#endif