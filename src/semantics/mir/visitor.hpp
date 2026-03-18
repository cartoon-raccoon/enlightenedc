#ifndef ECC_MIR_VISITOR_H
#define ECC_MIR_VISITOR_H

#include "semantics/mir/mir.hpp"

namespace ecc::sema::mir {

class MIRVisitor {
public:
    virtual ~MIRVisitor() = default;

    virtual void visit(ProgramMIR& node) = 0;
    virtual void visit(FunctionMIR& node) = 0;
    
    virtual void visit(InitializerMIR& node) = 0;
    virtual void visit(TypeDeclMIR& node) = 0;
    virtual void visit(VarDeclMIR& node) = 0;

    virtual void visit(CompoundStmtMIR& node) = 0;
    virtual void visit(ExprStmtMIR& node) = 0;
    virtual void visit(SwitchStmtMIR& node) = 0;
    virtual void visit(CaseStmtMIR& node) = 0;
    virtual void visit(CaseRangeStmtMIR& node) = 0;
    virtual void visit(DefaultStmtMIR& node) = 0;
    virtual void visit(LabeledStmtMIR& node) = 0;
    virtual void visit(PrintStmtMIR& node) = 0;
    virtual void visit(IfStmtMIR& node) = 0;
    virtual void visit(LoopStmtMIR& node) = 0;
    virtual void visit(GotoStmtMIR& node) = 0;
    virtual void visit(BreakStmtMIR& node) = 0;
    virtual void visit(ReturnStmtMIR& node) = 0;

    virtual void visit(BinaryExprMIR& node) = 0;
    virtual void visit(UnaryExprMIR& node) = 0;
    virtual void visit(CastExprMIR& node) = 0;
    virtual void visit(AssignExprMIR& node) = 0;
    virtual void visit(CondExprMIR& node) = 0;
    virtual void visit(IdentExprMIR& node) = 0;
    virtual void visit(ConstExprMIR& node) = 0;
    virtual void visit(LiteralExprMIR& node) = 0;
    virtual void visit(StringExprMIR& node) = 0;
    virtual void visit(CallExprMIR& node) = 0;
    virtual void visit(MemberAccExprMIR& node) = 0;
    virtual void visit(SubscrExprMIR& node) = 0;
    virtual void visit(PostfixExprMIR& node) = 0;
    virtual void visit(SizeofExprMIR& node) = 0;
};

}

#endif