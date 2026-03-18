#ifndef ECC_MIR_PRINTER_H
#define ECC_MIR_PRINTER_H

#include <iostream>
#include <string>

#include "codegen/mir.hpp"
#include "codegen/visitor.hpp"
#include "frontend/tokens.hpp"

namespace ecc::compiler::mir {

class MIRPrinter : public MIRVisitor {
  public:
    int indent = 0;

    void print_indent();

    template <typename NodeType, typename... Children>
    void print_node(const std::string& name, NodeType& node,
                    Children&&... children) {
        print_indent();
        std::cout << name << " @ <" << node.loc << ">\n";
        indent++;
        (children(), ...);
        indent--;
    }

    void visit(ProgramMIR& node) override;
    void visit(FunctionMIR& node) override;

    void visit(TypeDeclMIR& node) override;
    void visit(VarDeclMIR& node) override;
    void visit(InitializerMIR& node) override;

    void visit(CompoundStmtMIR& node) override;
    void visit(ExprStmtMIR& node) override;
    void visit(SwitchStmtMIR& node) override;
    void visit(CaseRangeStmtMIR& node) override;
    void visit(DefaultStmtMIR& node) override;
    void visit(LabeledStmtMIR& node) override;
    void visit(PrintStmtMIR& node) override;
    void visit(IfStmtMIR& node) override;
    void visit(LoopStmtMIR& node) override;
    void visit(GotoStmtMIR& node) override;
    void visit(BreakStmtMIR& node) override;
    void visit(ReturnStmtMIR& node) override;

    void visit(BinaryExprMIR& node) override;
    void visit(UnaryExprMIR& node) override;
    void visit(CastExprMIR& node) override;
    void visit(AssignExprMIR& node) override;
    void visit(CondExprMIR& node) override;
    void visit(IdentExprMIR& node) override;
    void visit(LiteralExprMIR& node) override;
    void visit(StringExprMIR& node) override;
    void visit(CallExprMIR& node) override;
    void visit(MemberAccExprMIR& node) override;
    void visit(SubscrExprMIR& node) override;
    void visit(PostfixExprMIR& node) override;
    void visit(SizeofExprMIR& node) override;

  private:
    std::string binop_to_string(tokens::BinaryOp op);
    std::string unop_to_string(tokens::UnaryOp op);
    std::string assignop_to_string(tokens::AssignOp op);
    std::string postfixop_to_string(tokens::PostfixOp op);

    std::string value_to_string(const exec::Value& val);
};

} // namespace ecc::compiler::mir
#endif