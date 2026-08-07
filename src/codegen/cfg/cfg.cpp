#include "codegen/cfg/cfg.hpp"

#include "util.hpp"

using namespace codegen::cfg;
using namespace codegen::lir;

Box<BasicBlock> BasicBlock::entry(std::string& func_name, FunctionCFG *func) {
    auto ret      = std::make_unique<BasicBlock>(func_name, func);
    ret->is_entry = true;

    return ret;
}

void BasicBlock::push_value(Box<Value> val) {
    func->values.push_back(std::move(val));
}

BasicBlock *FunctionCFG::create_block() {
    auto blk = std::make_unique<BasicBlock>(this);
    BasicBlock *ret = blk.get();

    blocks.push_back(std::move(blk));

    return ret;
}

BasicBlock *FunctionCFG::create_block(std::string& label, bool make_labeled) {
    auto blk        = std::make_unique<BasicBlock>(label, this);
    BasicBlock *ret = blk.get();

    blocks.push_back(std::move(blk));

    if (make_labeled) {
        labeled_blocks[label] = ret;
    }

    return ret;
}

BasicBlock *FunctionCFG::lookup_labeled_block(std::string& label) {
    if (labeled_blocks.contains(label)) {
        return labeled_blocks[label];
    } else {
        return nullptr;
    }
}

FunctionCFG *ProgramCFG::add_function(FunctionLIR *func) {
    auto funcfg = std::make_unique<FunctionCFG>(func);

    FunctionCFG *ret = funcfg.get();

    functions.push_back(std::move(funcfg));

    return ret;
}