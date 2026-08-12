#include "lowering/cfg/walker.hpp"

using namespace ecc::lower::cfg;

void CFGWalker::walk_function(FunctionCFG& function) {
    if (!visited.empty()) {
        visited.clear();
    }
    BasicBlock *starting = function.entry_block();
    visit_block(starting);
}

BasicBlock *VecCFGWalkerBlocksInner::next() {
    if (reverse) {
        return idx == 0 ? nullptr : blocks[--idx];
    } else {
        return idx >= blocks.size() ? nullptr : blocks[idx++];
    }
}

void CFGWalker::visit_block(BasicBlock *blk) {
    Vec<BasicBlock *> chain;

    while (!visited.contains(blk)) {
        visited.insert(blk);
        pre_visit(blk);
        chain.push_back(blk);

        size_t n = blk->num_successors();
        if (n == 0) {
            break;
        }
        if (n > 1) {
            for (BasicBlock *succ : blk->successors()) {
                if (!visited.contains(succ))
                    visit_block(succ);
            }
            break;
        }

        blk = *blk->successors().begin();
    }

    for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
        post_visit(*it);
    }
}
