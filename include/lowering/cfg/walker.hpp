#pragma once

#ifndef ECC_CFG_WALKERS_H
#define ECC_CFG_WALKERS_H

#include "lowering/cfg/cfg.hpp"
#include "util.hpp"

using namespace ecc::util;

namespace ecc::lower::cfg {

/**
*/
class CFGWalker {
public:
    virtual ~CFGWalker() = default;

    void walk_function(lower::cfg::FunctionCFG& function);

    void visit_block(lower::cfg::BasicBlock *blk);

    virtual void pre_visit(lower::cfg::BasicBlock *blk) {}; // NOLINT

    virtual void post_visit(lower::cfg::BasicBlock *blk) {}; // NOLINT

    /**
    The actual block visit by the consumer.

    For preorder and postorder, this can be called while still traversing the graph;
    for reverse postorder, this is called when iterating over the accumulated blocks.
    */
    virtual void on_block(lower::cfg::BasicBlock *blk) = 0;
    
protected:
    HashSet<lower::cfg::BasicBlock *> visited;
};

class PreorderCFGWalker : public CFGWalker {
public:
};

class PostorderCFGWalker : public CFGWalker {

};

class RevPostorderCFGWalker : public CFGWalker {
public:
};

}

#endif