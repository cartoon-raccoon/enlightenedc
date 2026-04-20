#include "codegen/lir/lir.hpp"

#include "codegen/lir/visitor.hpp"
#include "util.hpp"

using namespace codegen::lir;

DO_ACCEPT(ProgramLIR, LIRVisitor);
DO_ACCEPT(FunctionLIR, LIRVisitor);
DO_ACCEPT(VarDeclLIR, LIRVisitor);
DO_ACCEPT(GotoStmtLIR, LIRVisitor);
DO_ACCEPT(ExprStmtLIR, LIRVisitor);
DO_ACCEPT(SwitchStmtLIR, LIRVisitor);
DO_ACCEPT(CaseLIR, LIRVisitor);
DO_ACCEPT(DefaultLIR, LIRVisitor);
DO_ACCEPT(BreakStmtLIR, LIRVisitor);
DO_ACCEPT(ContStmtLIR, LIRVisitor);
DO_ACCEPT(IfStmtLIR, LIRVisitor);
DO_ACCEPT(LoopStmtLIR, LIRVisitor);
DO_ACCEPT(LabelDeclLIR, LIRVisitor);
DO_ACCEPT(PrintStmtLIR, LIRVisitor);
DO_ACCEPT(ReturnStmtLIR, LIRVisitor);
DO_ACCEPT(BinaryExprLIR, LIRVisitor);
DO_ACCEPT(UnaryExprLIR, LIRVisitor);
DO_ACCEPT(CastExprLIR, LIRVisitor);
DO_ACCEPT(AssignExprLIR, LIRVisitor);
DO_ACCEPT(CondExprLIR, LIRVisitor);
DO_ACCEPT(IdentExprLIR, LIRVisitor);
DO_ACCEPT(LiteralExprLIR, LIRVisitor);
DO_ACCEPT(CallExprLIR, LIRVisitor);
DO_ACCEPT(MemberAccExprLIR, LIRVisitor);
DO_ACCEPT(SubscrExprLIR, LIRVisitor);
DO_ACCEPT(PostfixExprLIR, LIRVisitor);
