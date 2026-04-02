#include "semantics/mir/printer.hpp"

#include <variant>

#include "semantics/types.hpp"

using namespace ecc::sema::mir;
using namespace ecc::sema::types;

void MIRPrinter::print_indent() {
    for (int i = 0; i < indent; ++i)
        std::cout << "| ";
}

std::string MIRPrinter::binop_to_string(tokens::BinaryOp op) {
    switch (op) {
    case tokens::BinaryOp::PLUS:
        return "+";
    case tokens::BinaryOp::MINUS:
        return "-";
    case tokens::BinaryOp::MUL:
        return "*";
    case tokens::BinaryOp::DIV:
        return "/";
    case tokens::BinaryOp::MOD:
        return "%";
    case tokens::BinaryOp::EQ:
        return "==";
    case tokens::BinaryOp::NE:
        return "!=";
    case tokens::BinaryOp::LE:
        return "<=";
    case tokens::BinaryOp::GE:
        return ">=";
    case tokens::BinaryOp::LT:
        return "<";
    case tokens::BinaryOp::GT:
        return ">";
    case tokens::BinaryOp::ANDAND:
        return "&&";
    case tokens::BinaryOp::OROR:
        return "||";
    case tokens::BinaryOp::AND:
        return "&";
    case tokens::BinaryOp::OR:
        return "|";
    case tokens::BinaryOp::XOR:
        return "^";
    case tokens::BinaryOp::LSHIFT:
        return "<<";
    case tokens::BinaryOp::RSHIFT:
        return ">>";
    case tokens::BinaryOp::BINCOMMA:
        return ",";
    }
    return "";
}

std::string MIRPrinter::unop_to_string(tokens::UnaryOp op) {
    switch (op) {
    case tokens::UnaryOp::INC:
        return "++";
    case tokens::UnaryOp::DEC:
        return "--";
    case tokens::UnaryOp::REF:
        return "&";
    case tokens::UnaryOp::DEREF:
        return "*";
    case tokens::UnaryOp::POS:
        return "+";
    case tokens::UnaryOp::NEG:
        return "-";
    case tokens::UnaryOp::TILDE:
        return "~";
    case tokens::UnaryOp::NOT:
        return "!";
    }
    return "";
}

std::string MIRPrinter::assignop_to_string(tokens::AssignOp op) {
    switch (op) {
    case tokens::AssignOp::ASSIGN:
        return "=";
    case tokens::AssignOp::PLUSEQ:
        return "+=";
    case tokens::AssignOp::MINUSEQ:
        return "-=";
    case tokens::AssignOp::MULEQ:
        return "*=";
    case tokens::AssignOp::DIVEQ:
        return "/=";
    case tokens::AssignOp::MODEQ:
        return "%=";
    case tokens::AssignOp::LSHIFTEQ:
        return "<<=";
    case tokens::AssignOp::RSHIFTEQ:
        return ">>=";
    case tokens::AssignOp::ANDEQ:
        return "&=";
    case tokens::AssignOp::OREQ:
        return "|=";
    case tokens::AssignOp::XOREQ:
        return "^=";
    }
    return "";
}

std::string MIRPrinter::postfixop_to_string(tokens::PostfixOp op) {
    switch (op) {
    case tokens::PostfixOp::POSTINC:
        return "++";
    case tokens::PostfixOp::POSTDEC:
        return "--";
    }
    return "";
}

std::string MIRPrinter::value_to_string(const exec::Value& val) {
    return std::visit(match{[](std::monostate) { return std::string("void"); },
                            [](char v) { return std::to_string(v); },
                            [](long v) { return std::to_string(v); },
                            [](double v) { return std::to_string(v); },
                            [](bool v) { return v ? std::string("true") : std::string("false"); },
                            [](const std::string& v) { return "\"" + v + "\""; }},
                      val.inner);
}

void MIRPrinter::visit(ProgramMIR& node) {
    print_node("ProgramMIR", node, [&] {
        for (auto& item : node.items)
            item->accept(*this);
    });
}

void MIRPrinter::visit(FunctionMIR& node) {
    print_node(
        "FunctionMIR: " + node.sym->name, node,
        [&] {
            std::cout << std::string(indent * 2, ' ')
                      << "type: " << node.sym->signature->to_string() << "\n";
        },
        [&] { node.body->accept(*this); });
}

void MIRPrinter::visit(TypeDeclMIR& node) {
    print_node("TypeDecl: " + node.sym->name, node);
}

void MIRPrinter::visit(VarDeclMIR& node) {
    print_node("VarDecl: ", node, [&] {
        indent++;
        for (auto& decl : node.decls) {
            print_indent();
            std::cout << decl.sym->to_string() << "\n";
            if (decl.initializer) {
                (*decl.initializer)->accept(*this);
            }
        }
        indent--;
    });
}

void MIRPrinter::visit(InitializerMIR& node) {
    print_node("Initializer", node, [&] {
        if (auto *e = std::get_if<Box<ExprMIR>>(&node.initializer)) {
            (*e)->accept(*this);
        } else {
            for (auto& init : std::get<Vec<Box<InitializerMIR>>>(node.initializer))
                init->accept(*this);
        }
    });
}

void MIRPrinter::visit(CompoundStmtMIR& node) {
    print_node("CompoundStmtMIR", node, [&] {
        for (auto& item : node.items)
            item->accept(*this);
    });
}

void MIRPrinter::visit(ExprStmtMIR& node) {
    print_node("ExprStmt", node, [&] {
        if (node.expr) {
            node.expr.value()->accept(*this);
        }
    });
}

void MIRPrinter::visit(SwitchStmtMIR& node) {
    print_node(
        "SwitchStmt", node, [&] { node.condition->accept(*this); },
        [&] { node.body->accept(*this); });
}

void MIRPrinter::visit(CaseStmtMIR& node) {
    print_node(
        "Case: ", node, [&] { node.case_expr->accept(*this); }, [&] { node.stmt->accept(*this); });
}

void MIRPrinter::visit(CaseRangeStmtMIR& node) {
    print_node(
        "CaseRange: ", node, [&] { node.case_start->accept(*this); },
        [&] { node.case_end->accept(*this); }, [&] { node.stmt->accept(*this); });
}

void MIRPrinter::visit(DefaultStmtMIR& node) {
    print_node("DefaultStmt", node, [&] { node.stmt->accept(*this); });
}

void MIRPrinter::visit(LabeledStmtMIR& node) {
    print_node("Label: " + node.label->name, node, [&] { node.stmt->accept(*this); });
}

void MIRPrinter::visit(PrintStmtMIR& node) {
    print_node("Print: \"" + node.format_string + "\"", node, [&] {
        for (auto& arg : node.arguments)
            arg->accept(*this);
    });
}

void MIRPrinter::visit(IfStmtMIR& node) {
    print_node(
        "IfStmt", node, [&] { node.condition->accept(*this); },
        [&] { node.then_branch->accept(*this); },
        [&] {
            if (node.else_branch)
                node.else_branch.value()->accept(*this);
        });
}

void MIRPrinter::visit(LoopStmtMIR& node) {
    print_node(
        std::string("LoopStmt") + (node.is_dowhile ? " (do-while)" : ""), node,
        [&] {
            if (node.init)
                node.init.value()->accept(*this);
        },
        [&] {
            if (node.condition)
                node.condition.value()->accept(*this);
        },
        [&] {
            if (node.step)
                node.step.value()->accept(*this);
        },
        [&] { node.body->accept(*this); });
}

void MIRPrinter::visit(GotoStmtMIR& node) {
    print_node("Goto: " + node.target, node);
}

void MIRPrinter::visit(BreakStmtMIR& node) {
    print_node("Break", node);
}

void MIRPrinter::visit(ContStmtMIR& node) {
    print_node("Continue", node);
}

void MIRPrinter::visit(ReturnStmtMIR& node) {
    print_node("Return", node, [&] {
        if (node.ret_expr)
            node.ret_expr.value()->accept(*this);
    });
}

void MIRPrinter::visit(BinaryExprMIR& node) {
    print_node(
        "Binary: " + binop_to_string(node.op), node, [&] { node.left->accept(*this); },
        [&] { node.right->accept(*this); });
}

void MIRPrinter::visit(UnaryExprMIR& node) {
    print_node("Unary: " + unop_to_string(node.op), node, [&] { node.operand->accept(*this); });
}

void MIRPrinter::visit(CastExprMIR& node) {
    print_node("Cast -> " + node.target->to_string(), node, [&] { node.inner->accept(*this); });
}

void MIRPrinter::visit(AssignExprMIR& node) {
    print_node(
        "Assign: " + assignop_to_string(node.op), node, [&] { node.left->accept(*this); },
        [&] { node.right->accept(*this); });
}

void MIRPrinter::visit(CondExprMIR& node) {
    print_node(
        "CondExpr", node, [&] { node.condition->accept(*this); },
        [&] { node.true_expr->accept(*this); }, [&] { node.false_expr->accept(*this); });
}

void MIRPrinter::visit(IdentExprMIR& node) {
    print_node("Ident: " + node.ident->name, node);
}

void MIRPrinter::visit(ConstExprMIR& node) {
    print_node("Const: ", node, [&] { node.inner->accept(*this); });
}

void MIRPrinter::visit(LiteralExprMIR& node) {
    print_node("Literal: " + value_to_string(node.value), node);
}

void MIRPrinter::visit(CallExprMIR& node) {
    print_node(
        "Call", node, [&] { node.callee->accept(*this); },
        [&] {
            for (auto& arg : node.args)
                arg->accept(*this);
        });
}

void MIRPrinter::visit(MemberAccExprMIR& node) {
    print_node("Member: ." + node.member, node, [&] { node.object->accept(*this); });
}

void MIRPrinter::visit(SubscrExprMIR& node) {
    print_node(
        "Subscript", node, [&] { node.array->accept(*this); }, [&] { node.index->accept(*this); });
}

void MIRPrinter::visit(PostfixExprMIR& node) {
    print_node("Postfix: " + postfixop_to_string(node.op), node,
               [&] { node.operand->accept(*this); });
}

void MIRPrinter::visit(SizeofExprMIR& node) {
    print_node("Sizeof: ", node, [&] {
        std::visit(match{[this](Box<ExprMIR>& expr) { expr->accept(*this); },
                         [](Type *& type) { std::cout << type->to_string() << '\n'; }},
                   node.operand);
    });
}