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

    std::string token_type_to_string(TokenType t) {
        switch (t) {
        case PLUS:
            return "+";
        case MINUS:
            return "-";
        case MUL:
            return "*";
        case DIV:
            return "/";
        case MOD:
            return "%";

        case ASSIGN:
            return "=";
        case EQ:
            return "==";
        case NE:
            return "!=";
        case LE:
            return "<=";
        case GE:
            return ">=";

        case ANDAND:
            return "&&";
        case OROR:
            return "||";

        case INC:
            return "++";
        case DEC:
            return "--";

        case PLUSEQ:
            return "+=";
        case MINUSEQ:
            return "-=";
        case MULEQ:
            return "*=";
        case DIVEQ:
            return "/=";
        case MODEQ:
            return "%=";
        case LSHIFTEQ:
            return "<<=";
        case RSHIFTEQ:
            return ">>=";
        case ANDEQ:
            return "&=";
        case OREQ:
            return "|=";
        case XOREQ:
            return "^=";

        case AND:
            return "&";
        case OR:
            return "|";
        case XOR:
            return "^";
        case TILDE:
            return "~";
        case NOT:
            return "!";

        case LT:
            return "<";
        case GT:
            return ">";

        case LSHIFT:
            return "<<";
        case RSHIFT:
            return ">>";

        case DOT:
            return ".";
        case ARROW:
            return "->";

        case COMMA:
            return ",";
        case SEMI:
            return ";";

        default:
            return "<unknown>";
        }
    }

    std::string primitive_to_string(TypeSpecifier::Primitive p) {
        using P = TypeSpecifier::Primitive;
        switch (p) {
        case P::VOID:
            return "void";
        case P::U0:
            return "u0";
        case P::U8:
            return "u8";
        case P::U16:
            return "u16";
        case P::U32:
            return "u32";
        case P::U64:
            return "u64";
        case P::I0:
            return "i0";
        case P::I8:
            return "i8";
        case P::I16:
            return "i16";
        case P::I32:
            return "i32";
        case P::I64:
            return "i64";
        case P::F64:
            return "f64";
        case P::BOOL:
            return "bool";
        }
        return "";
    }

    std::string storage_to_string(StorageClassSpecifier::SpecType t) {
        using S = StorageClassSpecifier::SpecType;
        switch (t) {
        case S::PUBLIC:
            return "public";
        case S::STATIC:
            return "static";
        case S::EXTERN:
            return "extern";
        }
        return "";
    }

    std::string qualifier_to_string(TypeQualifier::QualType q) {
        using Q = TypeQualifier::QualType;
        switch (q) {
        case Q::CONST:
            return "const";
        }
        return "";
    }

    std::string jump_to_string(JumpStatement::Kind k) {
        using J = JumpStatement::Kind;
        switch (k) {
        case J::GOTO:
            return "goto";
        case J::BREAK:
            return "break";
        case J::RETURN:
            return "return";
        }
        return "";
    }

    std::string label_kind_to_string(LabeledStatement::Kind k) {
        using L = LabeledStatement::Kind;
        switch (k) {
        case L::IDENTIFIER:
            return "identifier";
        case L::CASE:
            return "case";
        case L::CASE_RANGE:
            return "case_range";
        case L::DEFAULT:
            return "default";
        }
        return "";
    }

    void visit(Program& node) override {
        print_node("Program", node, [&] {
            for (auto& item : node.items)
                item->accept(*this);
        });
    }

    void visit(Function& node) override {
        print_node(
            "Function", node,
            [&] {
                for (auto& spec : node.decl_spec_list)
                    spec->accept(*this);
            },
            [&] { node.declarator->accept(*this); },
            [&] { node.statements->accept(*this); });
    }

    void visit(CompoundStatement& node) override {
        print_node("CompoundStatement", node, [&] {
            for (auto& decl : node.declarations)
                decl->accept(*this);

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
        print_node(
            "LabeledStatement: " + label_kind_to_string(node.kind) +
                (node.label.empty() ? "" : " " + node.label),
            node,
            [&] {
                if (node.case_expr)
                    node.case_expr.value()->accept(*this);
            },
            [&] {
                if (node.case_range_end)
                    node.case_range_end.value()->accept(*this);
            },
            [&] { node.statement->accept(*this); });
    }

    void visit(PrintStatement& node) override {
        print_node("PrintStatement: \"" + node.format_string + "\"", node, [&] {
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
        print_node(
            "JumpStatement: " + jump_to_string(node.kind) +
                (node.target_label.empty() ? "" : " " + node.target_label),
            node, [&] {
                if (node.return_value)
                    node.return_value.value()->accept(*this);
            });
    }

    void visit(VariableDeclaration& node) override {
        print_node("VariableDeclaration", node, [&] {
            for (auto& spec : node.specifiers)
                spec->accept(*this);
            for (auto& decl : node.declarators)
                decl->accept(*this);
        });
    }

    void visit(InitDeclarator& node) override {
        print_node(
            "InitDeclarator", node, [&] { node.declarator->accept(*this); },
            [&] {
                if (node.initializer)
                    node.initializer.value()->accept(*this);
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
            std::string("FunctionDeclarator") +
                (node.is_variadic ? " (variadic)" : ""),
            node, [&] { node.base->accept(*this); },
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
        print_node("StorageClassSpecifier: " + storage_to_string(node.type),
                   node);
    }

    void visit(TypeSpecifier& node) override {
        if (std::holds_alternative<TypeSpecifier::Primitive>(node.type)) {
            auto prim = std::get<TypeSpecifier::Primitive>(node.type);
            print_node("TypeSpecifier: " + primitive_to_string(prim), node);
        } else if (std::holds_alternative<Box<StructOrUnionSpecifier>>(
                       node.type)) {
            print_node("TypeSpecifier", node, [&] {
                std::get<Box<StructOrUnionSpecifier>>(node.type)->accept(*this);
            });
        } else if (std::holds_alternative<Box<EnumSpecifier>>(node.type)) {
            print_node("TypeSpecifier", node, [&] {
                std::get<Box<EnumSpecifier>>(node.type)->accept(*this);
            });
        }
    }

    void visit(TypeQualifier& node) override {
        print_node("TypeQualifier: " + qualifier_to_string(node.qual), node);
    }

    void visit(EnumSpecifier& node) override {
        print_node(std::string("EnumSpecifier") +
                       (node.name ? ": " + node.name.value() : ""),
                   node, [&] {
                       if (node.enumerators)
                           for (auto& e : node.enumerators.value())
                               e->accept(*this);
                   });
    }

    void visit(StructOrUnionSpecifier& node) override {
        std::string kind =
            node.kind == StructOrUnionSpecifier::STRUCT ? "struct" : "union";

        print_node("StructOrUnionSpecifier: " + kind +
                       (node.name ? " " + node.name.value() : ""),
                   node, [&] {
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
            "BinaryExpression: " + token_type_to_string(node.op), node,
            [&] { node.left->accept(*this); },
            [&] { node.right->accept(*this); });
    }

    void visit(UnaryExpression& node) override {
        print_node("UnaryExpression: " + token_type_to_string(node.op), node,
                   [&] { node.operand->accept(*this); });
    }

    void visit(AssignmentExpression& node) override {
        print_node(
            "AssignmentExpression: " + token_type_to_string(node.op), node,
            [&] { node.left->accept(*this); },
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
        print_node(std::string("MemberAccess: ") +
                       (node.is_arrow ? "->" : ".") + node.member,
                   node, [&] { node.object->accept(*this); });
    }

    void visit(ArraySubscriptExpression& node) override {
        print_node(
            "ArraySubscriptExpression", node,
            [&] { node.array->accept(*this); },
            [&] { node.index->accept(*this); });
    }

    void visit(PostfixExpression& node) override {
        print_node("PostfixExpression: " + token_type_to_string(node.op), node,
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