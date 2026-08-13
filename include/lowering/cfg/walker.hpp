#pragma once

#ifndef ECC_CFG_WALKERS_H
#define ECC_CFG_WALKERS_H

#include <concepts>
#include <utility>

#include "lowering/cfg/cfg.hpp"
#include "util.hpp"

using namespace ecc::util;

namespace ecc::lower::cfg {

class CFGWalkerBlocks;

/**
A CFG walker that traverses a Control-Flow Graph using DFS.
*/
class CFGWalker {
public:
    virtual ~CFGWalker() = default;

    void walk_function(FunctionCFG& function);

    /**
    The actual block visit in the order specified by the subclass.

    This function returns an iterator that yields the blocks in the order
    specified by the subclass. For example, PostorderCFGWalker returns an
    iterator that returns blocks postorder.
    */
    virtual CFGWalkerBlocks blocks() = 0;

protected:
    void visit_block(BasicBlock *blk);

    virtual void pre_visit(BasicBlock *blk) {}; // NOLINT

    virtual void post_visit(BasicBlock *blk) {}; // NOLINT

    HashSet<BasicBlock *> visited;
};

/**
The next()-style iterator that drives CFGBlocksIter.
*/
class CFGWalkerBlocksInner : public NextIterator<BasicBlock *> {};

class VecCFGWalkerBlocksInner : public CFGWalkerBlocksInner {
    Span<BasicBlock *const> blocks;
    size_t idx;
    bool reverse;

public:
    VecCFGWalkerBlocksInner(Span<BasicBlock *const> blocks, bool reverse)
        : blocks(blocks), idx(reverse ? blocks.size() : 0), reverse(reverse) {}

    BasicBlock *next() override;
};

/**
The iterator over basic blocks.
*/
class CFGBlocksIter {
    CFGWalkerBlocksInner *inner = nullptr;
    BasicBlock *curr            = nullptr;

public:
    using difference_type = std::ptrdiff_t;
    using value_type      = BasicBlock *;

    CFGBlocksIter(CFGWalkerBlocksInner *inner) : inner(inner), curr(inner->next()) {}

    CFGBlocksIter() {}

    BasicBlock *operator*() const { return curr; }

    CFGBlocksIter& operator++() {
        curr = inner->next();

        return *this;
    }

    CFGBlocksIter operator++(int) {
        CFGBlocksIter tmp = *this;
        curr              = inner->next();

        return tmp;
    }

    bool operator==(const CFGBlocksIter& other) const { return curr == other.curr; }
};

/**

*/
class CFGWalkerBlocks {
    Box<CFGWalkerBlocksInner> inner;

    CFGWalkerBlocks(Box<CFGWalkerBlocksInner> inner) : inner(std::move(inner)) {}

public:
    template <typename Iter, typename... Args>
        requires std::derived_from<Iter, CFGWalkerBlocksInner>
    static CFGWalkerBlocks make(Args&&...args) {
        return CFGWalkerBlocks(std::make_unique<Iter>(std::forward<Args>(args)...));
    }

    CFGBlocksIter begin() { return CFGBlocksIter(inner.get()); }

    CFGBlocksIter end() { return CFGBlocksIter(); }
};

class PreorderCFGWalker : public CFGWalker {
public:
    CFGWalkerBlocks blocks() override {
        return CFGWalkerBlocks::make<VecCFGWalkerBlocksInner>(preorder, false);
    }

protected:
    void pre_visit(BasicBlock *blk) override { preorder.push_back(blk); }

    Vec<BasicBlock *> preorder;
};

class PostorderCFGWalker : public CFGWalker {
public:
protected:
    Vec<BasicBlock *> postorder;
};

class RevPostorderCFGWalker : public PostorderCFGWalker {
public:
};

} // namespace ecc::lower::cfg

#endif