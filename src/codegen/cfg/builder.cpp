#include "codegen/cfg/cfg.hpp"
#include "codegen/cfg/builder.hpp"
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
    1. check if previous block was terminated
        a. if so, create a new block and set it as current
        b. else, use current block
    2. insert
    */

    if (current_block->has_terminator()) {

    }
    current_block->add_element(&node);
}

void CFGBuilder::visit(PrintStmtLIR& node) {
    /*
    1. check if previous block was terminated
        a. if so, create a new block and set it as current
        b. else, use current block
    2. insert
    */
    if (current_block->has_terminator()) {
        
    }
    current_block->add_element(&node);
}

void CFGBuilder::visit(GotoStmtLIR& node) {
    /*
    0. create goto terminator `term`
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
    0. create switch terminator `term`
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
    2. create a goto terminator `t`, set target as the merge block
    3. check if current block is terminated
        a. if so, create a new block, set it to current, terminate that
        b. else, terminate current block with `t`
    */
}

void CFGBuilder::visit(ContStmtLIR& node) {
    /*
    1. find the latest LoopStmt on the stack
        a. if none, throw runtime error
    2. create a goto terminator `t, set target;
        a. if step has value, set that as target
        b. otherwise, set body as target
    3. check if current block is terminated
        a. if so, create a new block, set it to current, terminate that
        b. else, terminate current block with `t`
    */
}

void CFGBuilder::visit(IfStmtLIR& node) {
    /*
    1. create if terminator `t`; terminate current block with it
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
    1. create goto terminator `t`; terminate current block with it
    2. if init has value:
        a. create new block `init` as loop init
        b. iterate over items in init; accept each
        c. terminate current block with a goto (target uninitialized)
    3. create new block `body` as loop body
    4. if init was created:
        a. set `t`'s target as `init`
        b. set `init`'s goto's target as `body`
       else, set `t`'s target as `body`
    6. set `t`'s target as current block
    5. iterate over items in body; accept each
    */
}

void CFGBuilder::visit(ReturnStmtLIR& node) {

}

