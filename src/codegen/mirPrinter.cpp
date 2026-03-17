#include "codegen/mirPrinter.hpp"

using namespace ecc::compiler::mir;

void MIRPrinter::print_indent() {
    for (int i = 0; i < indent; ++i)
        std::cout << "| ";
}

std::string MIRPrinter::binop_to_string(tokens::BinaryOp op) {
    switch (op) {
    case tokens::PLUS:
        return "+";
    case tokens::MINUS:
        return "-";
    case tokens::MUL:
        return "*";
    case tokens::DIV:
        return "/";
    case tokens::MOD:
        return "%";
    case tokens::EQ:
        return "==";
    case tokens::NE:
        return "!=";
    case tokens::LE:
        return "<=";
    case tokens::GE:
        return ">=";
    case tokens::LT:
        return "<";
    case tokens::GT:
        return ">";
    case tokens::ANDAND:
        return "&&";
    case tokens::OROR:
        return "||";
    case tokens::AND:
        return "&";
    case tokens::OR:
        return "|";
    case tokens::XOR:
        return "^";
    case tokens::LSHIFT:
        return "<<";
    case tokens::RSHIFT:
        return ">>";
    case tokens::BINCOMMA:
        return ",";
    }
    return "";
}

std::string MIRPrinter::unop_to_string(tokens::UnaryOp op) {
    switch (op) {
    case tokens::INC:
        return "++";
    case tokens::DEC:
        return "--";
    case tokens::REF:
        return "&";
    case tokens::DEREF:
        return "*";
    case tokens::POS:
        return "+";
    case tokens::NEG:
        return "-";
    case tokens::TILDE:
        return "~";
    case tokens::NOT:
        return "!";
    }
    return "";
}

std::string MIRPrinter::assignop_to_string(tokens::AssignOp op) {
    switch (op) {
    case tokens::ASSIGN:
        return "=";
    case tokens::PLUSEQ:
        return "+=";
    case tokens::MINUSEQ:
        return "-=";
    case tokens::MULEQ:
        return "*=";
    case tokens::DIVEQ:
        return "/=";
    case tokens::MODEQ:
        return "%=";
    case tokens::LSHIFTEQ:
        return "<<=";
    case tokens::RSHIFTEQ:
        return ">>=";
    case tokens::ANDEQ:
        return "&=";
    case tokens::OREQ:
        return "|=";
    case tokens::XOREQ:
        return "^=";
    }
    return "";
}

std::string MIRPrinter::postfixop_to_string(tokens::PostfixOp op) {
    switch (op) {
    case tokens::POSTINC:
        return "++";
    case tokens::POSTDEC:
        return "--";
    }
    return "";
}

std::string MIRPrinter::value_to_string(const exec::Value& val) {
    return std::visit(
        overloaded{[](std::monostate) { return std::string("void"); },
                   [](char v) { return std::to_string(v); },
                   [](long v) { return std::to_string(v); },
                   [](double v) { return std::to_string(v); },
                   [](bool v) {
                       return v ? std::string("true") : std::string("false");
                   },
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
    print_node("VarDecl: " + node.sym->name + " : " +
                   node.sym->type->to_string(),
               node, [&] {
                   if (node.initializer)
                       node.initializer.value()->accept(*this);
               });
}

void MIRPrinter::visit(InitializerMIR& node) {
    print_node("Initializer", node, [&] {
        if (auto* e = std::get_if<Box<ExprMIR>>(&node.initializer)) {
            (*e)->accept(*this);
        } else {
            for (auto& init :
                 std::get<Vec<Box<InitializerMIR>>>(node.initializer))
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
    print_node("ExprStmt", node, [&] { node.expr->accept(*this); });
}

void MIRPrinter::visit(SwitchStmtMIR& node) {
    print_node(
        "SwitchStmt", node, [&] { node.condition->accept(*this); },
        [&] { node.body->accept(*this); });
}

void MIRPrinter::visit(CaseRangeStmtMIR& node) {
    print_node("CaseRange: " + value_to_string(*node.case_start) + " .. " +
                   value_to_string(*node.case_end),
               node, [&] { node.stmt->accept(*this); });
}

void MIRPrinter::visit(DefaultStmtMIR& node) {
    print_node("DefaultStmt", node, [&] { node.stmt->accept(*this); });
}

void MIRPrinter::visit(LabeledStmtMIR& node) {
    print_node("Label: " + node.label->name, node,
               [&] { node.stmt->accept(*this); });
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
        [&] { node.condition->accept(*this); },
        [&] {
            if (node.step)
                node.step.value()->accept(*this);
        },
        [&] { node.body->accept(*this); });
}

void MIRPrinter::visit(GotoStmtMIR& node) {
    print_node("Goto: " + node.target->name, node);
}

void MIRPrinter::visit(BreakStmtMIR& node) { print_node("Break", node); }

void MIRPrinter::visit(ReturnStmtMIR& node) {
    print_node("Return", node, [&] { node.ret_expr->accept(*this); });
}

void MIRPrinter::visit(BinaryExprMIR& node) {
    print_node(
        "Binary: " + binop_to_string(node.op), node,
        [&] { node.left->accept(*this); }, [&] { node.right->accept(*this); });
}

void MIRPrinter::visit(UnaryExprMIR& node) {
    print_node("Unary: " + unop_to_string(node.op), node,
               [&] { node.operand->accept(*this); });
}

void MIRPrinter::visit(CastExprMIR& node) {
    print_node("Cast -> " + node.target->to_string(), node,
               [&] { node.inner->accept(*this); });
}

void MIRPrinter::visit(AssignExprMIR& node) {
    print_node(
        "Assign: " + assignop_to_string(node.op), node,
        [&] { node.left->accept(*this); }, [&] { node.right->accept(*this); });
}

void MIRPrinter::visit(CondExprMIR& node) {
    print_node(
        "CondExpr", node, [&] { node.condition->accept(*this); },
        [&] { node.true_expr->accept(*this); },
        [&] { node.false_expr->accept(*this); });
}

void MIRPrinter::visit(IdentExprMIR& node) {
    print_node("Ident: " + node.ident->name, node);
}

void MIRPrinter::visit(LiteralExprMIR& node) {
    print_node("Literal: " + value_to_string(*node.value), node);
}

void MIRPrinter::visit(StringExprMIR& node) {
    print_node("String: " + node.value, node);
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
    print_node("Member: ." + node.member, node,
               [&] { node.object->accept(*this); });
}

void MIRPrinter::visit(SubscrExprMIR& node) {
    print_node(
        "Subscript", node, [&] { node.array->accept(*this); },
        [&] { node.index->accept(*this); });
}

void MIRPrinter::visit(PostfixExprMIR& node) {
    print_node("Postfix: " + postfixop_to_string(node.op), node,
               [&] { node.operand->accept(*this); });
}

void MIRPrinter::visit(SizeofExprMIR& node) {
    print_node("Sizeof: " + node.target->to_string(), node);
}