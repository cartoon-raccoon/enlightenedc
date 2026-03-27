#ifndef ECC_LIR_VISITOR_H
#define ECC_LIR_VISITOR_H

#include "codegen/lir/lir.hpp"

namespace ecc::codegen::lir {

class LIRVisitor {
public:
    virtual ~LIRVisitor() = default;

    virtual void visit(ProgramLIR& node) = 0;
    virtual void visit(FunctionLIR& node) = 0;
    
    virtual void visit(VarDeclLIR& node) = 0;
    
    virtual void visit(ExprStmtLIR& node) = 0;
    virtual void visit(GotoStmtLIR& node) = 0;
    virtual void visit(SwitchStmtLIR& node) = 0;
    virtual void visit(BreakStmtLIR& node) = 0;
    virtual void visit(ContStmtLIR& node) = 0;
    virtual void visit(IfStmtLIR& node) = 0;
    virtual void visit(CaseStmtLIR& node) = 0;
    virtual void visit(DefaultStmtLIR& node) = 0;
    virtual void visit(LoopStmtLIR& node) = 0;
    virtual void visit(LabelStmtLIR& node) = 0;
    virtual void visit(PrintStmtLIR& node) = 0;
    virtual void visit(ReturnStmtLIR& node) = 0;
    
    virtual void visit(BinaryExprLIR& node) = 0;
    virtual void visit(UnaryExprLIR& node) = 0;
    virtual void visit(CastExprLIR& node) = 0;
    virtual void visit(AssignExprLIR& node) = 0;
    virtual void visit(CondExprLIR& node) = 0;
    virtual void visit(IdentExprLIR& node) = 0;
    virtual void visit(LiteralExprLIR& node) = 0;
    virtual void visit(CallExprLIR& node) = 0;
    virtual void visit(MemberAccExprLIR& node) = 0;
    virtual void visit(SubscrExprLIR& node) = 0;
    virtual void visit(PostfixExprLIR& node) = 0;    
};

}

#endif