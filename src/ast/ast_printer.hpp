// for testing:
// rm -rf build; cmake -B build; cmake --build build
// ./build/ecc src/test/test.ec

#ifndef ECC_AST_PRINTER_H
#define ECC_AST_PRINTER_H

#include "ast/ast.hpp"
#include "ast/visitor.hpp"
#include <iostream>
#include <string>

namespace ecc::ast {

class ASTPrinter : public ASTVisitor {
  public:
    int indent = 0;

    void print_indent() {
        for (int i = 0; i < indent; ++i)
            std::cout << "| ";
    }

    template <typename NodeType, typename... Children>
    void print_node(const std::string& name, NodeType& node,
                    Children&&... children) {
        print_indent();
        std::cout << name << "\n";
        indent++;
        (children(), ...);
        indent--;
    }

    void visit(Program& node) override {
        print_node("Program", node, [&] {
            for (auto& item : node.items)
                item->accept(*this);
        });
    }

    void visit(Function& node) override {
        print_node(
            "Function", node, [&] { node.declarator->accept(*this); },
            [&] { node.statements->accept(*this); });
    }

    void visit(CompoundStatement& node) override {
        print_node("CompoundStatement", node, [&] {
            for (auto& stmt : node.statements)
                stmt->accept(*this);
        });
    }

    void visit(ExpressionStatement& node) override {
        print_node("ExpressionStatement", node, [&] {
            if (node.expression)
                node.expression.value()->accept(*this);
        });
    }

    void visit(LabeledStatement& node) override {
        print_node("LabeledStatement", node, [&] {
            if (node.case_expr)
                node.case_expr.value()->accept(*this);
            if (node.case_range_end)
                node.case_range_end.value()->accept(*this);
            node.statement->accept(*this);
        });
    }

    void visit(PrintStatement& node) override {
        print_node("PrintStatement", node, [&] {
            for (auto& arg : node.arguments)
                arg->accept(*this);
        });
    }

    void visit(IfStatement& node) override {
        print_node(
            "IfStatement", node, [&] { node.condition->accept(*this); },
            [&] { node.then_branch->accept(*this); },
            [&] {
                if (node.else_branch)
                    node.else_branch.value()->accept(*this);
            });
    }

    void visit(SwitchStatement& node) override {
        print_node(
            "SwitchStatement", node, [&] { node.condition->accept(*this); },
            [&] { node.body->accept(*this); });
    }

    void visit(WhileStatement& node) override {
        print_node(
            "WhileStatement", node, [&] { node.condition->accept(*this); },
            [&] { node.body->accept(*this); });
    }

    void visit(DoWhileStatement& node) override {
        print_node(
            "DoWhileStatement", node, [&] { node.body->accept(*this); },
            [&] { node.condition->accept(*this); });
    }

    void visit(ForStatement& node) override {
        print_node(
            "ForStatement", node,
            [&] {
                if (node.init)
                    node.init.value()->accept(*this);
            },
            [&] {
                if (node.condition)
                    node.condition.value()->accept(*this);
            },
            [&] {
                if (node.increment)
                    node.increment.value()->accept(*this);
            },
            [&] { node.body->accept(*this); });
    }

    void visit(JumpStatement& node) override {
        print_node("JumpStatement", node, [&] {
            if (node.return_value)
                node.return_value.value()->accept(*this);
        });
    }

    void visit(VariableDeclaration& node) override {
        print_node("VariableDeclaration", node, [&] {
            for (auto& decl : node.declarators)
                decl->accept(*this);
        });
    }

    void visit(InitDeclarator& node) override {
        print_node(
            "InitDeclarator", node, [&] { node.declarator->accept(*this); },
            [&] {
                if (node.initializer)
                    node.initializer.value()->expression.value()->accept(*this);
            });
    }

    void visit(IdentifierDeclarator& node) override {
        print_node("IdentifierDeclarator: " + node.name, node);
    }

    void visit(ParameterDeclaration& node) override {
        print_node("ParameterDeclaration", node, [&] {
            for (auto& spec : node.specifiers)
                spec->accept(*this);
            if (node.declarator)
                node.declarator.value()->accept(*this);
            if (node.default_value)
                node.default_value.value()->accept(*this);
        });
    }

    void visit(Declarator& node) override {
        print_node(
            "Declarator", node,
            [&] {
                if (node.pointer)
                    node.pointer.value()->accept(*this);
            },
            [&] {
                if (node.direct)
                    node.direct.value()->accept(*this);
            });
    }

    void visit(ParenDeclarator& node) override {
        print_node("ParenDeclarator", node, [&] { node.inner->accept(*this); });
    }

    void visit(ArrayDeclarator& node) override {
        print_node(
            "ArrayDeclarator", node, [&] { node.base->accept(*this); },
            [&] {
                if (node.size)
                    node.size.value()->accept(*this);
            });
    }

    void visit(FunctionDeclarator& node) override {
        print_node(
            "FunctionDeclarator", node, [&] { node.base->accept(*this); },
            [&] {
                for (auto& param : node.parameters)
                    param->accept(*this);
            });
    }

    void visit(Pointer& node) override {
        print_node(
            "Pointer", node,
            [&] {
                for (auto& qual : node.qualifiers)
                    qual->accept(*this);
            },
            [&] {
                if (node.nested)
                    node.nested.value()->accept(*this);
            });
    }

    void visit(StructDeclarator& node) override {
        print_node(
            "StructDeclarator", node,
            [&] {
                if (node.declarator)
                    node.declarator.value()->accept(*this);
            },
            [&] {
                if (node.bit_width)
                    node.bit_width.value()->accept(*this);
            });
    }

    void visit(StructDeclaration& node) override {
        print_node("StructDeclaration", node, [&] {
            for (auto& spec : node.specifiers)
                spec->accept(*this);
            for (auto& decl : node.declarators)
                decl->accept(*this);
        });
    }

    void visit(Enumerator& node) override {
        print_node("Enumerator: " + node.name, node, [&] {
            if (node.value)
                node.value.value()->accept(*this);
        });
    }

    void visit(StorageClassSpecifier& node) override {
        print_node("StorageClassSpecifier", node);
    }

    void visit(TypeSpecifier& node) override {
        print_node("TypeSpecifier", node, [&] {
            if (std::holds_alternative<Box<StructOrUnionSpecifier>>(node.type))
                std::get<Box<StructOrUnionSpecifier>>(node.type)->accept(*this);
            else if (std::holds_alternative<Box<EnumSpecifier>>(node.type))
                std::get<Box<EnumSpecifier>>(node.type)->accept(*this);
        });
    }

    void visit(TypeQualifier& node) override {
        print_node("TypeQualifier", node);
    }

    void visit(EnumSpecifier& node) override {
        print_node("EnumSpecifier", node, [&] {
            if (node.enumerators)
                for (auto& e : node.enumerators.value())
                    e->accept(*this);
        });
    }

    void visit(StructOrUnionSpecifier& node) override {
        print_node("StructOrUnionSpecifier", node, [&] {
            if (node.declarations)
                for (auto& decl : node.declarations.value())
                    decl->accept(*this);
        });
    }

    void visit(Initializer& node) override {
        print_node(
            "Initializer", node,
            [&] {
                if (node.expression)
                    node.expression.value()->accept(*this);
            },
            [&] {
                for (auto& init : node.initializer_list)
                    init->accept(*this);
            });
    }

    void visit(TypeName& node) override {
        print_node("TypeName", node, [&] {
            for (auto& spec : node.specifiers)
                spec->accept(*this);
            if (node.declarator)
                node.declarator.value()->accept(*this);
        });
    }

    void visit(LiteralExpression& node) override {
        print_node("Literal: " + node.value, node);
    }

    void visit(IdentifierExpression& node) override {
        print_node("Identifier: " + node.name, node);
    }

    void visit(BinaryExpression& node) override {
        print_node(
            "BinaryExpression", node, [&] { node.left->accept(*this); },
            [&] { node.right->accept(*this); });
    }

    void visit(UnaryExpression& node) override {
        print_node("UnaryExpression", node,
                   [&] { node.operand->accept(*this); });
    }

    void visit(AssignmentExpression& node) override {
        print_node(
            "AssignmentExpression", node, [&] { node.left->accept(*this); },
            [&] { node.right->accept(*this); });
    }

    void visit(ConditionalExpression& node) override {
        print_node(
            "ConditionalExpression", node,
            [&] { node.condition->accept(*this); },
            [&] { node.true_expr->accept(*this); },
            [&] { node.false_expr->accept(*this); });
    }

    void visit(CallExpression& node) override {
        print_node(
            "CallExpression", node, [&] { node.callee->accept(*this); },
            [&] {
                for (auto& arg : node.arguments)
                    arg->accept(*this);
            });
    }

    void visit(MemberAccessExpression& node) override {
        print_node("MemberAccess: " + node.member, node,
                   [&] { node.object->accept(*this); });
    }

    void visit(ArraySubscriptExpression& node) override {
        print_node(
            "ArraySubscriptExpression", node,
            [&] { node.array->accept(*this); },
            [&] { node.index->accept(*this); });
    }

    void visit(PostfixExpression& node) override {
        print_node("PostfixExpression", node,
                   [&] { node.operand->accept(*this); });
    }

    void visit(SizeofExpression& node) override {
        print_node("SizeofExpression", node, [&] {
            if (std::holds_alternative<Box<Expression>>(node.operand))
                std::get<Box<Expression>>(node.operand)->accept(*this);
            else
                std::get<Box<TypeName>>(node.operand)->accept(*this);
        });
    }

    void visit(Declaration&) override {}
    void visit(Statement&) override {}
};

} // namespace ecc::ast
#endif