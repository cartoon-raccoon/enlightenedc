#include "lowering/cfg/printer.hpp"

#include <iostream>

#include "lowering/cfg/cfg.hpp"
#include "tokens.hpp"
#include "util.hpp"

using namespace ecc::lower::cfg;

static std::string binop_to_string(BinaryInst::Operator op) {
    using BinOp = BinaryInst::Operator;
    switch (op) {
    case BinOp::OR:
        return "or";
    case BinOp::XOR:
        return "xor";
    case BinOp::AND:
        return "and";
    case BinOp::EQ:
        return "eq";
    case BinOp::NE:
        return "ne";
    case BinOp::LT:
        return "lt";
    case BinOp::GT:
        return "gt";
    case BinOp::LE:
        return "le";
    case BinOp::GE:
        return "ge";
    case BinOp::SHL:
        return "shl";
    case BinOp::SHR:
        return "shr";
    case BinOp::ADD:
        return "add";
    case BinOp::SUB:
        return "sub";
    case BinOp::MUL:
        return "mul";
    case BinOp::DIV:
        return "div";
    case BinOp::MOD:
        return "mod";
    }
}

static std::string unop_to_string(UnaryInst::Operator op) {
    using UnOp = UnaryInst::Operator;
    switch (op) {
    case UnOp::POS:
        return "pos";
    case UnOp::NEG:
        return "neg";
    case UnOp::TILDE:
        return "compl";
    case UnOp::NOT:
        return "not";
    }
}

void CFGPrinter::name_function(FunctionCFG& func) {
    // for each function, name unlabeled blocks
    MonotonicCtr<size_t> blk_ctr;
    MonotonicCtr<size_t> inst_ctr;

    for (auto *alloc : func.get_allocas()) {
        if (!alloc->named()) {
            alloc->set_name(std::format("{}", *inst_ctr));
            inst_ctr++;
        }
    }
    for (auto& arg : func.get_args()) {
        if (!arg->named()) {
            arg->set_name(std::format("{}", *inst_ctr));
            inst_ctr++;
        }
    }
    for (auto& block : func) {
        // for each block, name unlabeled
        if (!block.has_label()) {
            block.name = std::format("{}", *blk_ctr);
            blk_ctr++;
        }
        for (auto& inst : block) {
            // for each instruction, name unnamed
            if (!inst.named()) {
                inst.set_name(std::format("{}", *inst_ctr));
                inst_ctr++;
            }
        }
    }
}

void CFGPrinter::print(ProgramCFG& cfg) {
    // first pass - assign names to values
    MonotonicCtr<size_t> global_ctr;
    for (auto& strpair : cfg.strings) {
        strpair.second->set_name(std::format("@str.{}", *global_ctr));
        global_ctr++;
    }
    for (auto *global : cfg.get_globals()) {
        if (!global->named()) {
            global->set_name(std::format("{}", *global_ctr));
        }
        global_ctr++;
    }
    for (auto& funcref : cfg.get_funcrefs()) {
        funcref->set_name(funcref->func->get_name());
    }

    for (auto *func : cfg.get_functions()) {
        name_function(*func);
    }
    name_function(*cfg.implicit_main);

    // second pass - print
    for (auto *global : cfg.get_globals()) {
        global->accept(*this);
        std::cout << "\n";
    }
    for (auto& [str, string] : cfg.strings) {
        string->accept(*this);
        std::cout << " = \"" << encode_string_literal(string->data) << "\"\n";
    }
    for (auto *func : cfg.get_functions()) {
        print_function(*func);
        std::cout << "\n";
    }
    print_function(*cfg.implicit_main);
}

void CFGPrinter::print_function(FunctionCFG& func) {
    if (!func.is_defined()) {
        std::cout << "declare function ";
    } else {
        std::cout << "define function ";
    }

    std::cout << func.get_name();
    std::cout << "(";
    for (auto [idx, arg] : std::views::enumerate(func.get_args())) {
        arg->accept(*this);
        if ((size_t)idx + 1 < func.get_args().size()) {
            std::cout << ", ";
        }
    }
    std::cout << ")";

    if (!func.is_defined()) {
        std::cout << ";\n";
    } else {
        std::cout << " {\n";
        for (auto *alloc : func.get_allocas()) {
            print_value(*alloc);
            alloc->accept(*this);
        }
        for (auto& block : func) {
            print_block(block);
        }
        std::cout << "}\n";
    }
}

void CFGPrinter::print_block(BasicBlock& blk) {
    std::cout << "#" << blk.name << ":\n";
    for (auto& inst : *blk) {
        print_value(inst);
        inst.accept(*this);
    }
    if (blk.terminator()) {
        blk.terminator()->accept(*this);
    }
}

void CFGPrinter::print_value(Value& value) {
    if (!isa<StoreInst>(&value) && !isa<PrintInst>(&value)) {
        if (auto *call = dyncast<CallInst>(&value); call && call->type->is_void()) {
            std::cout << "  ";
        } else {
            std::cout << "  %" << value.name << " = ";
        }
    } else {
        std::cout << "  ";
    }
}

void CFGPrinter::print_value_name(Value& value) {
    if (isa<ScalarConst>(&value) || isa<ZeroConst>(&value) || isa<AggregateConst>(&value)) {
        value.accept(*this);
    } else if (isa<FuncRef>(&value) || isa<String>(&value)) {
        std::cout << value.name;
    } else if (isa<Global>(&value)) {
        std::cout << "@" << value.name;
    } else {
        std::cout << "%" << value.name;
    }
}

void CFGPrinter::visit(ScalarConst& val) {
    std::cout << val.value;
}

void CFGPrinter::visit(AggregateConst& val) {
    std::cout << "{";

    for (auto [idx, item] : std::views::enumerate(val.elements)) {
        item->accept(*this);
        if ((size_t) idx + 1 < val.elements.size()) {
            std::cout << ", ";
        }
    }

    std::cout << "}";
}

void CFGPrinter::visit(String& val) {
    std::cout << val.name;
}

void CFGPrinter::visit(ZeroConst& val) {
    std::cout << "zero: [" << val.type->formal() << "]";
}

void CFGPrinter::visit(FuncRef& val) {
    std::cout << "func " << val.func->get_name();
}

void CFGPrinter::visit(Global& val) {
    std::cout << "@" << val.name << " (" << val.type->formal() << ")";
    if (val.initializer) {
        std::cout << " = ";
        val.initializer->accept(*this);
    }
}

void CFGPrinter::visit(FuncArg& val) {
    std::cout << "%" << val.name << ": " << val.type->formal();
}

void CFGPrinter::visit(AllocaInst& inst) {
    std::cout << "alloca (" << inst.type->formal() << ")\n";
}

void CFGPrinter::visit(LoadInst& inst) {
    std::cout << "load ";
    print_value_name(*inst.address);
    std::cout << "\n";
}

void CFGPrinter::visit(StoreInst& inst) {
    std::cout << "store ";
    print_value_name(*inst.value);
    std::cout << " -> ";
    print_value_name(*inst.address);
    std::cout << "\n";
}

void CFGPrinter::visit(PhiInst& inst) {
    std::cout << "phi ";
    for (auto& [val, blk] : inst.incoming) {
        std::cout << "[";
        print_value_name(*val);
        std::cout << " : #" << blk->name << "] ";
    }
    std::cout << "\n";
}

void CFGPrinter::visit(PrintInst& inst) {
    std::cout << "print " << inst.format_string->name << ": ";
    for (auto *arg : inst.args) {
        print_value_name(*arg);
        std::cout << " ";
    }
    std::cout << "\n";
}

void CFGPrinter::visit(BinaryInst& inst) {
    std::cout << binop_to_string(inst.op) << " ";
    print_value_name(*inst.loperand);
    std::cout << " ";
    print_value_name(*inst.roperand);
    std::cout << "\n";
}

void CFGPrinter::visit(UnaryInst& inst) {
    std::cout << unop_to_string(inst.op) << " ";
    print_value_name(*inst.operand);
    std::cout << "\n";
}

void CFGPrinter::visit(IncrInst& inst) {
    std::cout << "inc ";
    print_value_name(*inst.operand);
    std::cout << "\n";
}

void CFGPrinter::visit(DecrInst& inst) {
    std::cout << "dec ";
    print_value_name(*inst.operand);
    std::cout << "\n";
}

void CFGPrinter::visit(CastInst& inst) {
    std::cout << "cast ";
    print_value_name(*inst.operand);
    std::cout << " to (" << inst.target->formal() << ")\n";
}

void CFGPrinter::visit(ReintInst& inst) {
    std::cout << "reint ";
    print_value_name(*inst.operand);
    std::cout << " as " << tokens::primitive_to_string(inst.target) << "\n";
}

void CFGPrinter::visit(MemberAccInst& inst) {
    std::cout << "memacc ";
    print_value_name(*inst.operand);
    std::cout << " [" << inst.member_idx << "] (" << inst.operand->type->formal() << ")\n";
}

void CFGPrinter::visit(SubscrInst& inst) {
    std::cout << "subscr ";
    print_value_name(*inst.operand);
    std::cout << " [";
    print_value_name(*inst.index);
    std::cout << "]\n";
}

void CFGPrinter::visit(CallInst& inst) {
    std::cout << "call ";
    if (inst.type->is_void()) {
        std::cout << "void ";
    }
    print_value_name(*inst.operand);
    std::cout << " (";
    for (auto [idx, arg] : std::views::enumerate(inst.args)) {
        print_value_name(*arg);
        if ((size_t)idx + 1 < inst.args.size()) {
            std::cout << ", ";
        }
    }
    std::cout << ")\n";
}

void CFGPrinter::visit(If& term) {
    std::cout << "  if ";
    print_value_name(*term.cond);
    std::cout << " br #" << term.then_br->name;
    std::cout << " else br #" << term.else_br->name;
    std::cout << "\n";
}

void CFGPrinter::visit(Goto& term) {
    std::cout << "  goto #" << term.target->name << "\n";
}

void CFGPrinter::visit(Switch& term) {
    std::cout << "  switch ";
    print_value_name(*term.control);
    std::cout << " ";
    for (auto [idx, cas] : std::views::enumerate(term.cases)) {
        if (cas.case_val) {
            std::cout << "[" << *cas.case_val << ": #" << cas.blk->name << "]";
        } else {
            std::cout << "def #" << cas.blk->name;
        }
        if ((size_t)idx + 1 < term.cases.size()) {
            std::cout << " ";
        }
    }
    std::cout << "\n";
}

void CFGPrinter::visit(Return& term) {
    std::cout << "  ret";
    if (term.ret_value) {
        std::cout << " ";
        print_value_name(**term.ret_value);
    }
    std::cout << "\n";
}
