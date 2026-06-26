#include "codegen/lir/printer.hpp"

#include <variant>

#include "eval/value.hpp"
#include "semantics/types.hpp"
#include "tokens.hpp"

using namespace ecc::codegen::lir;
using namespace ecc::sema::types;
using namespace ecc::tokens;

// Needed because LIRNode::loc is Optional<Location>, not Location directly.
namespace ecc::location {
std::ostream& operator<<(std::ostream& os, const std::optional<Location>& loc) {
    if (loc) {
        os << *loc;
    } else {
        os << "?";
    }
    return os;
}
} // namespace ecc::location

void LIRPrinter::print_indent() const {
    for (size_t i = 0; i < indent; ++i)
        std::cout << "| ";
}

void LIRPrinter::visit(ProgramLIR& node) {
    print_node("ProgramLIR", node, [&] {
        for (auto& global : node.globals)
            global->accept(*this);
        for (auto& func : node.functions)
            func->accept(*this);
        for (auto& item : node.progitems)
            item->accept(*this);
    });
}

void LIRPrinter::visit(FunctionLIR& node) {
    print_node(
        "FunctionLIR: " + node.name + " (" + node.mangled_name + ")", node,
        [&] {
            std::cout << std::string(indent * 2, ' ')
                      << "type: " << node.lirsym->symbol->signature->to_string() << "\n";
        },
        [&] {
            for (auto& local : node.locals)
                local->accept(*this);
        },
        [&] {
            for (auto& item : node.body)
                item->accept(*this);
        });
}

void LIRPrinter::visit(VarDeclLIR& node) {
    print_node(
        "VarDecl: " + node.lirsym->name + " (" + node.lirsym->mangled_name + ")", node, [&] {
            print_indent();
            std::cout << "type: " << node.lirsym->sym->get_type()->to_string()
                      << (node.lirsym->is_param ? " [param]" : "") << "\n";
        });
}

void LIRPrinter::visit(LabelDeclLIR& node) {
    print_node("Label: " + node.label + " (" + node.mangled_label + ")", node);
}

void LIRPrinter::visit(CaseLIR& node) {
    print_node("Case: " + node.case_value.to_string(), node);
}

void LIRPrinter::visit(DefaultLIR& node) {
    print_node("Default", node);
}

void LIRPrinter::visit(ExprStmtLIR& node) {
    print_node("ExprStmt", node, [&] { node.expr->accept(*this); });
}

void LIRPrinter::visit(GotoStmtLIR& node) {
    std::string label = node.target ? *node.target + " (" + node.mangled_target + ")"
                                    : node.mangled_target;
    print_node("Goto: " + label, node);
}

void LIRPrinter::visit(SwitchStmtLIR& node) {
    print_node(
        "SwitchStmt", node, [&] { node.condition->accept(*this); },
        [&] {
            for (auto& item : node.body)
                item->accept(*this);
        });
}

void LIRPrinter::visit(BreakStmtLIR& node) {
    print_node("Break", node);
}

void LIRPrinter::visit(ContStmtLIR& node) {
    print_node("Continue", node);
}

void LIRPrinter::visit(IfStmtLIR& node) {
    print_node(
        "IfStmt", node, [&] { node.condition->accept(*this); },
        [&] {
            for (auto& item : node.then_br)
                item->accept(*this);
        },
        [&] {
            if (node.else_br) {
                for (auto& item : *node.else_br)
                    item->accept(*this);
            }
        });
}

void LIRPrinter::visit(LoopStmtLIR& node) {
    print_node(
        std::string("LoopStmt") + (node.is_dowhile ? " (do-while)" : ""), node,
        [&] {
            if (node.init) {
                for (auto& item : *node.init)
                    item->accept(*this);
            }
        },
        [&] {
            if (node.condition)
                (*node.condition)->accept(*this);
        },
        [&] {
            if (node.step) {
                for (auto& item : *node.step)
                    item->accept(*this);
            }
        },
        [&] {
            for (auto& item : node.body)
                item->accept(*this);
        });
}

void LIRPrinter::visit(PrintStmtLIR& node) {
    print_node("Print: \"" + node.format_string + "\"", node, [&] {
        for (auto& arg : node.args)
            arg->accept(*this);
    });
}

void LIRPrinter::visit(ReturnStmtLIR& node) {
    print_node("Return", node, [&] {
        if (node.ret_value)
            (*node.ret_value)->accept(*this);
    });
}

void LIRPrinter::visit(BinaryExprLIR& node) {
    print_node(
        "Binary: " + binop_to_string(node.op) + " :: " + node.act_type->formal(), node,
        [&] { node.left->accept(*this); }, [&] { node.right->accept(*this); });
}

void LIRPrinter::visit(UnaryExprLIR& node) {
    print_node("Unary: " + unop_to_string(node.op) + " :: " + node.act_type->formal(), node, [&] {
        node.operand->accept(*this);
    });
}

void LIRPrinter::visit(CastExprLIR& node) {
    print_node(
        "Cast -> " + node.target->to_string() + " :: " + node.act_type->formal(), node,
        [&] { node.inner->accept(*this); });
}

void LIRPrinter::visit(AssignExprLIR& node) {
    print_node(
        "Assign: " + assignop_to_string(node.op) + " :: " + node.act_type->formal(), node,
        [&] { node.left->accept(*this); }, [&] { node.right->accept(*this); });
}

void LIRPrinter::visit(CondExprLIR& node) {
    print_node(
        "CondExpr :: " + node.act_type->formal(), node,
        [&] { node.condition->accept(*this); }, [&] { node.true_value->accept(*this); },
        [&] { node.false_value->accept(*this); });
}

void LIRPrinter::visit(IdentExprLIR& node) {
    print_node(
        "Ident: " + node.sym->name + " (" + node.sym->mangled_name + ") :: " +
            node.act_type->formal(),
        node);
}

void LIRPrinter::visit(LiteralExprLIR& node) {
    print_node("Literal :: " + node.act_type->formal(), node, [&] {
        std::visit(
            match{
                [&](eval::Value& val) { std::cout << val.to_string() << "\n"; },
                [&](std::string& s) { std::cout << s << "\n"; }},
            node.value);
    });
}

void LIRPrinter::visit(CallExprLIR& node) {
    print_node(
        "Call :: " + node.act_type->formal(), node, [&] { node.callee->accept(*this); },
        [&] {
            for (auto& arg : node.args)
                arg->accept(*this);
        });
}

void LIRPrinter::visit(MemberAccExprLIR& node) {
    print_node(
        "Member: [" + std::to_string(node.member_idx) + "] :: " + node.act_type->formal(), node,
        [&] { node.object->accept(*this); });
}

void LIRPrinter::visit(ReintExprLIR& node) {
    print_node(
        "Reint: ." + primitive_to_string(node.target) + " :: " + node.act_type->formal(), node,
        [&] { node.object->accept(*this); });
}

void LIRPrinter::visit(SubscrExprLIR& node) {
    print_node(
        "Subscript :: " + node.act_type->formal(), node, [&] { node.array->accept(*this); },
        [&] { node.index->accept(*this); });
}

void LIRPrinter::visit(PostfixExprLIR& node) {
    print_node(
        "Postfix: " + postfixop_to_string(node.op) + " :: " + node.act_type->formal(), node,
        [&] { node.operand->accept(*this); });
}