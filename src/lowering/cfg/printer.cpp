#include "lowering/cfg/printer.hpp"

#include "lowering/cfg/cfg.hpp"
#include "util.hpp"

using namespace ecc::lower::cfg;

void CFGPrinter::print(ProgramCFG& cfg) {
    // first pass - assign names to values
    MonotonicCtr<size_t> str_ctr;
    for (auto& strpair : cfg.strings) {
        strpair.second->set_name(std::format("@str.{}", *str_ctr));
        str_ctr++;
    }

    for (auto& lircfgpair : cfg.functions) {
        // for each function, name unlabeled blocks
        MonotonicCtr<size_t> blk_ctr;
        MonotonicCtr<size_t> inst_ctr;

        for (auto *alloc : lircfgpair.second->get_allocas()) {
            if (!alloc->named()) {
                alloc->set_name(std::format("%{}", *inst_ctr));
                inst_ctr++;
            }
        }
        for (auto& block : *lircfgpair.second) {
            // for each block, name unlabeled
            if (!block.has_label()) {
                block.name = std::format("#{}", *blk_ctr);
                blk_ctr++;
            }
            for (auto& inst : block) {
                // for each instruction, name unnamed
                if (!inst.named()) {
                    inst.set_name(std::format("%{}", *inst_ctr));
                    inst_ctr++;
                }
            }
        }
    }

    // second pass - print
    for (auto *global : cfg.get_globals()) {

    }
}

void CFGPrinter::print_function(FunctionCFG& func) {
    if (!func.is_defined()) {

    }
}

void CFGPrinter::print_block(BasicBlock *blk) {

}

void CFGPrinter::visit(AllocaInst& inst) {
}

void CFGPrinter::visit(LoadInst& inst) {
}

void CFGPrinter::visit(StoreInst& inst) {
}

void CFGPrinter::visit(PhiInst& inst) {
}

void CFGPrinter::visit(PrintInst& inst) {
}

void CFGPrinter::visit(BinaryInst& inst) {
}

void CFGPrinter::visit(UnaryInst& inst) {
}

void CFGPrinter::visit(IncrInst& inst) {
}

void CFGPrinter::visit(DecrInst& inst) {
}

void CFGPrinter::visit(CastInst& inst) {
}

void CFGPrinter::visit(ReintInst& inst) {
}

void CFGPrinter::visit(MemberAccInst& inst) {
}

void CFGPrinter::visit(SubscrInst& inst) {
}

void CFGPrinter::visit(FuncRef& val) {
}

void CFGPrinter::visit(Literal& val) {
}

void CFGPrinter::visit(Zero& val) {
}

void CFGPrinter::visit(Global& val) {
}

void CFGPrinter::visit(String& val) {
}
