#include "codegen/cfg/cfg.hpp"

#include "util.hpp"

using namespace codegen::cfg;

Box<BasicBlock> BasicBlock::entry(std::string& func_name) {
    auto ret      = std::make_unique<BasicBlock>(func_name);
    ret->is_entry = true;

    return ret;
}

BasicBlock *FunctionCFG::create_block(std::string& label) {
    auto blk        = std::make_unique<BasicBlock>(label);
    BasicBlock *ret = blk.get();

    blocks.push_back(std::move(blk));

    return ret;
}

BasicBlock *FunctionCFG::lookup_block() {
    todo();
}

FunctionCFG *ProgramCFG::add_function() {
    todo();
}