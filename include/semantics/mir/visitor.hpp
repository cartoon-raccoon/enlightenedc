#pragma once

#ifndef ECC_MIR_VISITOR_H
#define ECC_MIR_VISITOR_H

#include "abstract/visitor.hpp"

namespace ecc::sema::mir {

class ProgramMIR;
class FunctionMIR;
class InitializerMIR;
class TypeDeclMIR;
class VarDeclMIR;
class CompoundStmtMIR;
class ExprStmtMIR;
class SwitchStmtMIR;
class CaseStmtMIR;
class CaseRangeStmtMIR;
class DefaultStmtMIR;
class LabeledStmtMIR;
class PrintStmtMIR;
class IfStmtMIR;
class LoopStmtMIR;
class GotoStmtMIR;
class BreakStmtMIR;
class ContStmtMIR;
class ReturnStmtMIR;
class BinaryExprMIR;
class UnaryExprMIR;
class CastExprMIR;
class AssignExprMIR;
class CondExprMIR;
class IdentExprMIR;
class LiteralExprMIR;
class CallExprMIR;
class MemberAccExprMIR;
class ReintExprMIR;
class SubscrExprMIR;
class PostfixExprMIR;
class SizeofExprMIR;

class MIRVisitor
    : public Visitor<
          MIRVisitor, ProgramMIR, FunctionMIR, InitializerMIR, TypeDeclMIR, VarDeclMIR,
          CompoundStmtMIR, ExprStmtMIR, SwitchStmtMIR, CaseStmtMIR, CaseRangeStmtMIR,
          DefaultStmtMIR, LabeledStmtMIR, PrintStmtMIR, IfStmtMIR, LoopStmtMIR, GotoStmtMIR,
          BreakStmtMIR, ContStmtMIR, ReturnStmtMIR, BinaryExprMIR, UnaryExprMIR, CastExprMIR,
          AssignExprMIR, CondExprMIR, IdentExprMIR, LiteralExprMIR, CallExprMIR, MemberAccExprMIR,
          ReintExprMIR, SubscrExprMIR, PostfixExprMIR, SizeofExprMIR> {};

} // namespace ecc::sema::mir

#endif