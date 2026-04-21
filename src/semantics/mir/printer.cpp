#include "semantics/mir/printer.hpp"

#include <variant>

#include "semantics/types.hpp"
#include "tokens.hpp"

using namespace ecc::sema::mir;
using namespace ecc::sema::types;
using namespace ecc::tokens;

void MIRPrinter::print_indent() const {
    for (size_t i = 0; i < indent; ++i)
        std::cout << "| ";
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
        std::visit(
            match{
                [&](Box<ExprMIR>& expr) { expr->accept(*this); },
                [&](Box<InitializerMIR::Member>& mem) {
                    std::cout << "." << mem->member << ": ";
                    mem->initializer->accept(*this);
                },
                [&](Box<InitializerMIR::Index>& idx) {
                    std::cout << "[" << idx->idx.to_string() << "]: ";
                    idx->initializer->accept(*this);
                },
                [&](Vec<Box<InitializerMIR>>& inits) {
                    for (auto& init : inits) {
                        init->accept(*this);
                    }
                }},
            node.initializer);
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
        "SwitchStmt", node, [&] { node.control_val->accept(*this); },
        [&] { node.body->accept(*this); });
}

void MIRPrinter::visit(CaseStmtMIR& node) {
    print_node("Case: " + node.case_val.to_string(), node, [&] { node.stmt->accept(*this); });
}

void MIRPrinter::visit(CaseRangeStmtMIR& node) {
    print_node(
        "CaseRange: " + node.case_start.to_string() + "..." + node.case_end.to_string(), node,
        [&] { node.stmt->accept(*this); });
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

void MIRPrinter::visit(LiteralExprMIR& node) {
    print_node("Literal: ", node, [&] {
        std::visit(
            match{
                [&](eval::Value& val) { std::cout << val.to_string(); },
                [&](std::string& s) { std::cout << s; }},
            node.value);
    });
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
    print_node(
        "Postfix: " + postfixop_to_string(node.op), node, [&] { node.operand->accept(*this); });
}

void MIRPrinter::visit(SizeofExprMIR& node) {
    print_node("Sizeof: ", node, [&] {
        std::visit(
            match{
                [this](Box<ExprMIR>& expr) { expr->accept(*this); },
                [](Type *& type) { std::cout << type->to_string() << '\n'; }},
            node.operand);
    });
}