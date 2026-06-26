#pragma once

#ifndef ECC_LIR_PRINTER_H
#define ECC_LIR_PRINTER_H

#include <iostream>
#include <string>

#include "codegen/lir/lir.hpp"
#include "codegen/lir/visitor.hpp"

namespace ecc::codegen::lir {

class LIRPrinter : public LIRVisitor {
public:
    size_t indent = 0;

    void print_indent() const;

    template <typename NodeType, typename... Children>
    void print_node(const std::string& name, NodeType& node, Children&&...children) {
        print_indent();
        std::cout << name << " @ <" << node.loc << ">\n";
        indent++;
        (std::forward<Children>(children)(), ...);
        indent--;
    }

    void visit(ProgramLIR& node) override;
    void visit(FunctionLIR& node) override;

    void visit(VarDeclLIR& node) override;

    void visit(LabelDeclLIR& node) override;
    void visit(CaseLIR& node) override;
    void visit(DefaultLIR& node) override;
    void visit(ExprStmtLIR& node) override;
    void visit(GotoStmtLIR& node) override;
    void visit(SwitchStmtLIR& node) override;
    void visit(BreakStmtLIR& node) override;
    void visit(ContStmtLIR& node) override;
    void visit(IfStmtLIR& node) override;
    void visit(LoopStmtLIR& node) override;
    void visit(PrintStmtLIR& node) override;
    void visit(ReturnStmtLIR& node) override;

    void visit(BinaryExprLIR& node) override;
    void visit(UnaryExprLIR& node) override;
    void visit(CastExprLIR& node) override;
    void visit(AssignExprLIR& node) override;
    void visit(CondExprLIR& node) override;
    void visit(IdentExprLIR& node) override;
    void visit(LiteralExprLIR& node) override;
    void visit(CallExprLIR& node) override;
    void visit(MemberAccExprLIR& node) override;
    void visit(ReintExprLIR& node) override;
    void visit(SubscrExprLIR& node) override;
    void visit(PostfixExprLIR& node) override;
};

}



#endif