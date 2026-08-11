#include "lowering/cfg/walker.hpp"

using namespace ecc::lower::cfg;

void CFGWalker::walk_function(FunctionCFG& function) {
    if (!visited.empty()) {
        visited.clear();
    }
    BasicBlock *starting = function.entry_block();
    visit_block(starting);
}

void CFGWalker::visit_block(BasicBlock *blk) {
    visited.insert(blk);

    pre_visit(blk);
    for (auto *succ : blk->successors()) {
        if (!visited.contains(succ))
        visit_block(succ);
    }
    post_visit(blk);
}