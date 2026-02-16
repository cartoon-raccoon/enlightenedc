// for testing:
// rm -rf build; cmake -B build; cmake --build build
// ./build/ecc src/test/test.ec

#ifndef ECC_AST_PRINTER_H
#define ECC_AST_PRINTER_H

#include "ast/ast.hpp"
#include "ast/visitor.hpp"
#include <iostream>

namespace ecc::ast {

class ASTPrinter : public ASTVisitor {
  public:
    int indent = 0;

    void print_indent() {
        for (int i = 0; i < indent; ++i)
            std::cout << "  ";
    }

    void visit(Program &node) override {
        std::cout << "Program\n";
        indent++;
        for (auto &item : node.items)
            item->accept(*this);
        indent--;
    }

    void visit(Function &node) override {
        print_indent();
        std::cout << "Function\n";
        indent++;
        node.declarator->accept(*this);
        node.statements->accept(*this);
        indent--;
    }

    void visit(CompoundStatement &node) override {
        print_indent();
        std::cout << "CompoundStatement\n";
        indent++;
        for (auto &stmt : node.statements)
            stmt->accept(*this);
        indent--;
    }

    void visit(ExpressionStatement &node) override {
        print_indent();
        std::cout << "ExpressionStatement\n";
        if (node.expression)
            node.expression.value()->accept(*this);
    }

    void visit(VariableDeclaration &node) override {
        print_indent();
        std::cout << "VariableDeclaration\n";
        indent++;
        for (auto &decl : node.declarators)
            decl->accept(*this);
        indent--;
    }

    void visit(InitDeclarator &node) override {
        print_indent();
        std::cout << "InitDeclarator\n";
        indent++;
        node.declarator->accept(*this);
        if (node.initializer)
            node.initializer.value()->expression.value()->accept(*this);
        indent--;
    }

    void visit(IdentifierDeclarator &node) override {
        print_indent();
        std::cout << "IdentifierDeclarator: " << node.name << "\n";
    }

    void visit(LiteralExpression &node) override {
        print_indent();
        std::cout << "Literal: " << node.value << "\n";
    }

    void visit(IdentifierExpression &node) override {
        print_indent();
        std::cout << "Identifier: " << node.name << "\n";
    }

    void visit(BinaryExpression &node) override {
        print_indent();
        std::cout << "BinaryExpression\n";
        indent++;
        node.left->accept(*this);
        node.right->accept(*this);
        indent--;
    }

    void visit(Declaration &) override {}
    void visit(ParameterDeclaration &) override {}
    void visit(Declarator &) override {}
    void visit(ParenDeclarator &) override {}
    void visit(ArrayDeclarator &) override {}
    void visit(FunctionDeclarator &) override {}
    void visit(Pointer &) override {}
    void visit(StructDeclarator &) override {}
    void visit(StructDeclaration &) override {}
    void visit(Enumerator &) override {}
    void visit(StorageClassSpecifier &) override {}
    void visit(TypeSpecifier &) override {}
    void visit(TypeQualifier &) override {}
    void visit(EnumSpecifier &) override {}
    void visit(StructOrUnionSpecifier &) override {}
    void visit(Initializer &) override {}
    void visit(TypeName &) override {}
    void visit(Statement &) override {}
    void visit(LabeledStatement &) override {}
    void visit(PrintStatement &) override {}
    void visit(IfStatement &) override {}
    void visit(SwitchStatement &) override {}
    void visit(WhileStatement &) override {}
    void visit(DoWhileStatement &) override {}
    void visit(ForStatement &) override {}
    void visit(JumpStatement &) override {}
    void visit(UnaryExpression &) override {}
    void visit(AssignmentExpression &) override {}
    void visit(ConditionalExpression &) override {}
    void visit(CallExpression &) override {}
    void visit(MemberAccessExpression &) override {}
    void visit(ArraySubscriptExpression &) override {}
    void visit(PostfixExpression &) override {}
    void visit(SizeofExpression &) override {}
};

}

#endif