#pragma once

#ifndef ECC_AST_PRINTER_H
#define ECC_AST_PRINTER_H

#include <concepts>
#include <iostream>

#include "ast/ast.hpp"
#include "ast/visitor.hpp"

namespace ecc::ast {

class ASTPrinter : public ASTVisitor {
public:
    int indent = 0;

    void visit(Program& node) override;
    void visit(Function& node) override;

    void visit(TypeDeclaration& node) override;
    void visit(VariableDeclaration& node) override;
    void visit(ParameterDeclaration& node) override;
    void visit(Declarator& node) override;
    void visit(ParenDeclarator& node) override;
    void visit(ArrayDeclarator& node) override;
    void visit(FunctionDeclarator& node) override;
    void visit(InitDeclarator& node) override;
    void visit(Pointer& node) override;
    void visit(ClassDeclarator& node) override;
    void visit(ClassDeclaration& node) override;
    void visit(Enumerator& node) override;
    void visit(StorageClassSpecifier& node) override;
    void visit(TypeQualifier& node) override;
    void visit(EnumSpecifier& node) override;
    void visit(VoidSpecifier& node) override;
    void visit(PrimitiveSpecifier& node) override;
    void visit(ClassSpecifier& node) override;
    void visit(UnionSpecifier& node) override;
    void visit(TypeIdentifier& node) override;
    void visit(Initializer& node) override;
    void visit(TypeName& node) override;
    void visit(IdentifierDeclarator& node) override;

    void visit(CompoundStatement& node) override;
    void visit(ExpressionStatement& node) override;
    void visit(CaseStatement& node) override;
    void visit(CaseRangeStatement& node) override;
    void visit(DefaultStatement& node) override;
    void visit(LabeledStatement& node) override;
    void visit(PrintStatement& node) override;
    void visit(IfStatement& node) override;
    void visit(SwitchStatement& node) override;
    void visit(WhileStatement& node) override;
    void visit(DoWhileStatement& node) override;
    void visit(ForStatement& node) override;
    void visit(GotoStatement& node) override;
    void visit(BreakStatement& node) override;
    void visit(ContinueStatement& node) override;
    void visit(ReturnStatement& node) override;

    void visit(BinaryExpression& node) override;
    void visit(CastExpression& node) override;
    void visit(UnaryExpression& node) override;
    void visit(AssignmentExpression& node) override;
    void visit(ConditionalExpression& node) override;
    void visit(IdentifierExpression& node) override;
    void visit(ConstExpression& node) override;
    void visit(LiteralExpression& node) override;
    void visit(StringExpression& node) override;
    void visit(CallExpression& node) override;
    void visit(MemberAccessExpression& node) override;
    void visit(ArraySubscriptExpression& node) override;
    void visit(PostfixExpression& node) override;
    void visit(SizeofExpression& node) override;

    void print_indent() const;

    template <typename NodeType, typename... Children>
        requires std::derived_from<NodeType, ASTNode>
    void print_node(const std::string& name, NodeType& node, Children&&...children) {
        print_indent();
        std::cout << name << " @ <" << node.loc << "> " << "\n";
        indent++;
        (std::forward<Children>(children)(), ...);
        indent--;
    }
};

} // namespace ecc::ast
#endif