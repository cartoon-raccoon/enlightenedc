#include "ast/printer.hpp"
#include "ast/ast.hpp"

#include <iostream>
#include <string>
#include <utility>
#include <variant>

using namespace ecc::ast;

void ASTPrinter::print_indent() {
    for (int i = 0; i < indent; ++i)
        std::cout << "| ";
}

std::string ASTPrinter::token_type_to_string(TokenType t) {
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

std::string ASTPrinter::primitive_to_string(PrimitiveSpecifier::PrimKind p) {
    using P = PrimitiveSpecifier::PrimKind;
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

std::string ASTPrinter::storage_to_string(StorageClassSpecifier::SpecType t) {
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

std::string ASTPrinter::qualifier_to_string(TypeQualifier::QualType q) {
    using Q = TypeQualifier::QualType;
    switch (q) {
    case Q::CONST:
        return "const";
    }
    return "";
}

std::string ASTPrinter::case_kind_to_string(CaseDefaultStatement::Kind k) {
    using L = CaseDefaultStatement::Kind;
    switch (k) {
    case L::CASE:
        return "case";
    case L::CASE_RANGE:
        return "case_range";
    case L::DEFAULT:
        return "default";
    }
    return "";
}

void ASTPrinter::visit(Program& node) {
    print_node("Program", node, [&] {
        for (auto& item : node.items)
            item->accept(*this);
    });
}

void ASTPrinter::visit(Function& node) {
    print_node(
        "Function", node,
        [&] {
            for (auto& spec : node.decl_spec_list)
                spec->accept(*this);
        },
        [&] { node.declarator->accept(*this); },
        [&] { node.body->accept(*this); });
}

void ASTPrinter::visit(CompoundStatement& node) {
    print_node("CompoundStatement", node, [&] {
        for (auto& decl : node.items)
            decl->accept(*this);
    });
}

void ASTPrinter::visit(ExpressionStatement& node) {
    print_node("ExpressionStatement", node, [&] {
        if (node.expression)
            node.expression.value()->accept(*this);
    });
}

void ASTPrinter::visit(CaseDefaultStatement& node) {
    print_node(
        "CaseDefaultStatement: " + case_kind_to_string(node.kind),
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

void ASTPrinter::visit(LabeledStatement& node) {
    print_node(
        "LabeledStatement: " + node.label,
        node,
        [&] {
            if (node.statement)
                node.statement->accept(*this);
        }
    );
}

void ASTPrinter::visit(PrintStatement& node) {
    print_node("PrintStatement: \"" + node.format_string + "\"", node, [&] {
        for (auto& arg : node.arguments)
            arg->accept(*this);
    });
}

void ASTPrinter::visit(IfStatement& node) {
    print_node(
        "IfStatement", node, [&] { node.condition->accept(*this); },
        [&] { node.then_branch->accept(*this); },
        [&] {
            if (node.else_branch)
                node.else_branch.value()->accept(*this);
        });
}

void ASTPrinter::visit(SwitchStatement& node) {
    print_node(
        "SwitchStatement", node, [&] { node.condition->accept(*this); },
        [&] { node.body->accept(*this); });
}

void ASTPrinter::visit(WhileStatement& node) {
    print_node(
        "WhileStatement", node, [&] { node.condition->accept(*this); },
        [&] { node.body->accept(*this); });
}

void ASTPrinter::visit(DoWhileStatement& node) {
    print_node(
        "DoWhileStatement", node, [&] { node.body->accept(*this); },
        [&] { node.condition->accept(*this); });
}

void ASTPrinter::visit(ForStatement& node) {
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

void ASTPrinter::visit(JumpStatement& node) {
    std::unreachable();
}

void ASTPrinter::visit(GotoStatement& node) {
    print_node(
        "GotoStatement: " + node.target_label,
        node
    );
}

void ASTPrinter::visit(BreakStatement& node) {
    print_node(
        "BreakStatement",
        node
    );
}

void ASTPrinter::visit(ReturnStatement& node) {
    print_node(
        "ReturnStatement",
        node,
        [&] {
            if (node.return_value) {
                node.return_value.value()->accept(*this);
            }
        }
    );
}

void ASTPrinter::visit(TypeDeclaration& node) {
    print_node("TypeDeclaration", node, [&] {
        for (auto& spec : node.specifiers)
            spec->accept(*this);
    });
}

void ASTPrinter::visit(VariableDeclaration& node) {
    print_node("VariableDeclaration", node, [&] {
        for (auto& spec : node.specifiers)
            spec->accept(*this);
        for (auto& decl : node.declarators)
            decl->accept(*this);
    });
}

void ASTPrinter::visit(InitDeclarator& node) {
    print_node(
        "InitDeclarator", node, [&] { node.declarator->accept(*this); },
        [&] {
            if (node.initializer)
                node.initializer.value()->accept(*this);
        });
}

void ASTPrinter::visit(IdentifierDeclarator& node) {
    print_node("IdentifierDeclarator: " + node.name, node);
}

void ASTPrinter::visit(ParameterDeclaration& node) {
    print_node("ParameterDeclaration", node, [&] {
        for (auto& spec : node.specifiers)
            spec->accept(*this);
        if (node.declarator)
            node.declarator.value()->accept(*this);
        if (node.default_value)
            node.default_value.value()->accept(*this);
    });
}

void ASTPrinter::visit(Declarator& node) {
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

void ASTPrinter::visit(ParenDeclarator& node) {
    print_node("ParenDeclarator", node, [&] { node.inner->accept(*this); });
}

void ASTPrinter::visit(ArrayDeclarator& node) {
    print_node(
        "ArrayDeclarator", node, [&] { node.base->accept(*this); },
        [&] {
            if (node.size)
                node.size.value()->accept(*this);
        });
}

void ASTPrinter::visit(FunctionDeclarator& node) {
    print_node(
        std::string("FunctionDeclarator") +
            (node.is_variadic ? " (variadic)" : ""),
        node, [&] { node.base->accept(*this); },
        [&] {
            for (auto& param : node.parameters)
                param->accept(*this);
        });
}

void ASTPrinter::visit(Pointer& node) {
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

void ASTPrinter::visit(ClassDeclarator& node) {
    print_node(
        "ClassDeclarator", node,
        [&] {
            if (node.declarator)
                node.declarator.value()->accept(*this);
        },
        [&] {
            if (node.bit_width)
                node.bit_width.value()->accept(*this);
        });
}

void ASTPrinter::visit(ClassDeclaration& node) {
    print_node("ClassDeclaration", node, [&] {
        for (auto& spec : node.specifiers)
            spec->accept(*this);
        for (auto& decl : node.declarators)
            decl->accept(*this);
    });
}

void ASTPrinter::visit(Enumerator& node) {
    print_node("Enumerator: " + node.name, node, [&] {
        if (node.value)
            node.value.value()->accept(*this);
    });
}

void ASTPrinter::visit(StorageClassSpecifier& node) {
    print_node("StorageClassSpecifier: " + storage_to_string(node.type),
                node);
}

void ASTPrinter::visit(PrimitiveSpecifier& node) {
    print_node("PrimitiveSpecifier: " + primitive_to_string(node.pkind), node);
}

void ASTPrinter::visit(TypeQualifier& node) {
    print_node("TypeQualifier: " + qualifier_to_string(node.qual), node);
}

void ASTPrinter::visit(EnumSpecifier& node) {
    print_node(std::string("EnumSpecifier") +
                    (node.name ? ": " + node.name.value() : ""),
                node, [&] {
                    if (node.enumerators)
                        for (auto& e : node.enumerators.value())
                            e->accept(*this);
                });
}

void ASTPrinter::visit(ClassOrUnionSpecifier& node) {
    std::string kind =
        node.kind == ClassOrUnionSpecifier::CLASS ? "class" : "union";

    print_node("ClassOrUnionSpecifier: " + kind +
                    (node.name ? " " + node.name.value() : ""),
                node, [&] {
                    if (node.declarations)
                        for (auto& decl : node.declarations.value())
                            decl->accept(*this);
                });
}

void ASTPrinter::visit(Initializer& node) {
    print_node(
        "Initializer: ", node,
        [&] {
            if (auto* s = std::get_if<Box<Expression>>(&node.initializer)) {
                (*s)->accept(*this);
            } else if (auto* s = std::get_if<Vec<Box<Initializer>>>(&node.initializer)) {
                for (auto& init : *s) {
                    init->accept(*this);
                }
            }
        });
}

void ASTPrinter::visit(TypeName& node) {
    print_node("TypeName", node, [&] {
        for (auto& spec : node.specifiers)
            spec->accept(*this);
        if (node.declarator)
            node.declarator.value()->accept(*this);
    });
}

void ASTPrinter::visit(LiteralExpression& node) {
    switch (node.kind) {
        case LiteralExpression::LiteralKind::INT:
        print_node("Literal: " + std::to_string(node.value.i_val), node);
        break;
        case LiteralExpression::LiteralKind::FLOAT:
        print_node("Literal: " + std::to_string(node.value.f_val), node);
        break;
        case LiteralExpression::LiteralKind::CHAR:
        print_node("Literal: " + std::to_string(node.value.c_val), node);
        break;
        case LiteralExpression::LiteralKind::BOOL:
        print_node("Literal: " + std::to_string(node.value.b_val), node);
        break;
    }
}

void ASTPrinter::visit(StringExpression& node) {
    print_node("String: " + node.value, node);
}

void ASTPrinter::visit(IdentifierExpression& node) {
    print_node("Identifier: " + node.name, node);
}

void ASTPrinter::visit(BinaryExpression& node) {
    print_node(
        "BinaryExpression: " + token_type_to_string(node.op), node,
        [&] { node.left->accept(*this); },
        [&] { node.right->accept(*this); });
}

void ASTPrinter::visit(UnaryExpression& node) {
    print_node("UnaryExpression: " + token_type_to_string(node.op), node,
                [&] { node.operand->accept(*this); });
}

void ASTPrinter::visit(AssignmentExpression& node) {
    print_node(
        "AssignmentExpression: " + token_type_to_string(node.op), node,
        [&] { node.left->accept(*this); },
        [&] { node.right->accept(*this); });
}

void ASTPrinter::visit(ConditionalExpression& node) {
    print_node(
        "ConditionalExpression", node,
        [&] { node.condition->accept(*this); },
        [&] { node.true_expr->accept(*this); },
        [&] { node.false_expr->accept(*this); });
}

void ASTPrinter::visit(CallExpression& node) {
    print_node(
        "CallExpression", node, [&] { node.callee->accept(*this); },
        [&] {
            for (auto& arg : node.arguments)
                arg->accept(*this);
        });
}

void ASTPrinter::visit(MemberAccessExpression& node) {
    print_node(std::string("MemberAccess: ") +
                    (node.is_arrow ? "->" : ".") + node.member,
                node, [&] { node.object->accept(*this); });
}

void ASTPrinter::visit(ArraySubscriptExpression& node) {
    print_node(
        "ArraySubscriptExpression", node,
        [&] { node.array->accept(*this); },
        [&] { node.index->accept(*this); });
}

void ASTPrinter::visit(PostfixExpression& node) {
    print_node("PostfixExpression: " + token_type_to_string(node.op), node,
                [&] { node.operand->accept(*this); });
}

void ASTPrinter::visit(SizeofExpression& node) {
    print_node("SizeofExpression", node, [&] {
        if (std::holds_alternative<Box<Expression>>(node.operand))
            std::get<Box<Expression>>(node.operand)->accept(*this);
        else
            std::get<Box<TypeName>>(node.operand)->accept(*this);
    });
}