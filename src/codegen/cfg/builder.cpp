#include "codegen/cfg/builder.hpp"

#include "codegen/cfg/cfg.hpp"
#include "codegen/lir/lir.hpp"

using namespace codegen::lir;
using namespace codegen::cfg;

void CFGBuilder::build_cfg(lir::ProgramLIR& prog) {
    prog.accept(*this);
}

NestedStmtInfo *CFGBuilder::find_info(NestedStmtFilter& filter) {
    for (auto stmt = infostack.rbegin(); stmt != infostack.rend(); stmt++) {
        if (filter(stmt->get())) {
            return stmt->get();
        }
    }

    return nullptr;
}

void CFGBuilder::visit(ProgramLIR& node) {
    for (auto& item : node.progitems) {
        item->accept(*this);
    }
}

void CFGBuilder::visit(FunctionLIR& node) {
}

void CFGBuilder::visit(LabelDeclLIR& node) {
    /*
    1. create a new block with label; set incoming as current block
    2. check if current block is terminated
        a. if so, create new block, set that as current
        b. else, use current block
    3. resolve any to_resolve gotos from elsewhere
    4. set current block to new block
    */
}

void CFGBuilder::visit(CaseLIR& node) {
    /*
    1. create a new block
    2. check current block's terminator
        a. if set, should be break; we're good
        b. if unset, terminate with a goto to new block (fallthrough)
    3. find latest SwitchStmtInfo and add new SwitchCase
    4. set current block to new block
    */
}

void CFGBuilder::visit(DefaultLIR& node) {
    /*
    same as CaseLIR
    */
}

void CFGBuilder::visit(ExprStmtLIR& node) {
    /*
    1. check if current block is terminated
        a. if so, create a new block and set it as current
        b. else, use current block
    2. insert
    */

    if (current_block->has_terminator()) {
    }
    node.expr->accept(*this);
}

void CFGBuilder::visit(PrintStmtLIR& node) {
    /*
    1. check if current block is terminated
        a. if so, create a new block and set it as current
        b. else, use current block
    2. insert
    */
    if (current_block->has_terminator()) {
    }
}

void CFGBuilder::visit(GotoStmtLIR& node) {
    /*
    0. create a Goto terminator `term`
    2. check if current block was terminated
        a. if so, create a new block and set it as current
           terminate newly current block with `term`
        b. else, terminate current block with `term`
    2. search current function for targets
        a. if found `targ`, set goto target to `targ`
        b. else, add to to_resolve label pile
    */
}

void CFGBuilder::visit(SwitchStmtLIR& node) {
    /*
    0. create a Switch terminator `term`
    1. check if current block was terminated
        a. if so, create a new block and set it as current
           terminate newly current block with `term`
        b. else, terminate current block with `term`
    2. create new block (outside of function) as merge block
    3. create new block as block for first case
    4. create SwitchStmtInfo and push it to the infostack
    5. iterate over items in body; accept each
    6. add merge block to function and set it as current block
    7. pop the infostack
    */
    for (auto& item : node.body) {
        item->accept(*this);
    }
}

void CFGBuilder::visit(BreakStmtLIR& node) {
    /*
    1. find the latest SwitchStmt or LoopStmt on the stack
        a. if none, throw runtime error
    2. create a Goto terminator `t`, set target as the merge block
    3. check if current block is terminated
        a. if so, create a new block, set it to current, terminate that
        b. else, terminate current block with `t`
    */
}

void CFGBuilder::visit(ContStmtLIR& node) {
    /*
    1. find the latest LoopStmt on the stack
        a. if none, throw runtime error
    2. create a Goto terminator `t, set target;
        a. if step has value, set that as target
        b. otherwise, set body as target
    3. check if current block is terminated
        a. if so, create a new block, set it to current, terminate that
        b. else, terminate current block with `t`
    */
}

void CFGBuilder::visit(IfStmtLIR& node) {
    /*
    0. create If terminator `term` with cond as node.condition
    1. check if current block was terminated
        a. if so, create a new block and set it as current
           terminate newly current block with `term`
        b. else, terminate current block with `term`
    2. create new block (outside of function) as merge block
    3. create new block as then block;
    4. create IfStmtInfo and push it to the infostack
    5. iterate over items in then_br; accept each
    6. if there is an else node, repeat 3-5 for else_br
    7. add merge block to function and set it as current block
    8. pop the infostack
    */
}

void CFGBuilder::visit(LoopStmtLIR& node) {
    /*
    1. check if current block is terminated
       a. if so, create new block and set it as current
       b. else, use current block
    1. create new block (outside of function) as merge block
    2. if init has value:
        a. create Goto terminator `t`
        b. terminate current block with `t`
           (current block is guaranteed to be unterminated as we checked in 1)
        a. create new block `init` as loop init
        b. set `init` as `t`'s target
        c. iterate over items in init; accept each
        d. leave current block unterminated
    3. if node.condition has value:
        a. create new block as condition block
        b. create Goto terminator `g`, set condition block as target
        c. terminate current block with `g`
           (current block is guaranteed to be unterminated)
        d. create If terminator `c` with cond as node.condition
        e. create new blocks for then_br and else_br
        f. create Goto terminators for then_br and else_br blocks
        g. set else_br's Goto target as merge
        h. leave then_br's block unterminated, set as current block
    4. create new block as body block
    5. create LoopStmtInfo with cond and body, push to stack
    6. treat current block as body block; iterate over items in body; accept each

    */
}

void CFGBuilder::visit(ReturnStmtLIR& node) {
    /*
    0. create Return terminator `ret`
    */
}

void CFGBuilder::visit(VarDeclLIR& node) { // NOLINT
}

void CFGBuilder::visit(BinaryExprLIR& node) {
    node.left->accept(*this);
    Value *left = last_value;
    node.right->accept(*this);
    Value *right = last_value;

    current_block->add_instruction<BinaryInst>(
        node.act_type, node.op, left, right, *node.loc);
}

void CFGBuilder::visit(UnaryExprLIR& node) {
}

void CFGBuilder::visit(CastExprLIR& node) {
}

void CFGBuilder::visit(AssignExprLIR& node) {
}

void CFGBuilder::visit(CondExprLIR& node) {
}

void CFGBuilder::visit(IdentExprLIR& node) {
}

void CFGBuilder::visit(LiteralExprLIR& node) {
}

void CFGBuilder::visit(ZeroExprLIR& node) {
}

void CFGBuilder::visit(CallExprLIR& node) {
}

void CFGBuilder::visit(MemberAccExprLIR& node) {
}

void CFGBuilder::visit(SubscrExprLIR& node) {
}

void CFGBuilder::visit(PostfixExprLIR& node) {
}