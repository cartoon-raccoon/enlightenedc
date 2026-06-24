#pragma once

#ifndef ECC_LIR_SYNTH_H
#define ECC_LIR_SYNTH_H

#include <queue>
#include <stack>
#include <variant>

#include "codegen/lir/lir.hpp"
#include "codegen/lir/symbols.hpp"
#include "semantics/mir/mir.hpp"
#include "semantics/semantics.hpp"
#include "semantics/symbols.hpp"
#include "util.hpp"

using namespace ecc;
using namespace util;

namespace ecc::codegen::lir {

class LIRSynthesizer : public sema::BaseMIRSemaVisitor, public NoMove {
public:
    LIRSynthesizer(ProgramLIR& prog_lir, LIRSymbolMap& symbolmap)
        : sema::BaseMIRSemaVisitor(State::READ), symbolmap(symbolmap), prog_lir(prog_lir) {}

    using LIRSynthItem = std::variant<Box<FunctionLIR>, Box<VarDeclLIR>, Box<ProgItemLIR>>;

    Ref<LIRSymbolMap> symbolmap;

    Ref<ProgramLIR> prog_lir;

    void generate_lir(sema::mir::ProgramMIR& prog);

protected:
    // Emit a LIR item into the current queue.
    void emit(LIRSynthItem item);

    // Consume a LIR item from the current queue.
    LIRSynthItem consume();

    void push_queue();

    void pop_queue();

    bool curr_is_empty();

    LIRVarSym *insert_varsym(sema::sym::VarSymbol *sym, Box<LIRVarSym> varsym);

    void unfold_initializer(sema::sym::VarSymbol *sym, sema::mir::InitializerMIR& init);

    void do_visit(sema::mir::ProgramMIR& node) override;
    void do_visit(sema::mir::FunctionMIR& node) override;

    void do_visit(sema::mir::InitializerMIR& node) override;
    void do_visit(sema::mir::TypeDeclMIR& node) override;
    void do_visit(sema::mir::VarDeclMIR& node) override;

    void do_visit(sema::mir::CompoundStmtMIR& node) override;
    void do_visit(sema::mir::ExprStmtMIR& node) override;
    void do_visit(sema::mir::SwitchStmtMIR& node) override;
    void do_visit(sema::mir::CaseStmtMIR& node) override;
    void do_visit(sema::mir::CaseRangeStmtMIR& node) override;
    void do_visit(sema::mir::DefaultStmtMIR& node) override;
    void do_visit(sema::mir::LabeledStmtMIR& node) override;
    void do_visit(sema::mir::PrintStmtMIR& node) override;
    void do_visit(sema::mir::IfStmtMIR& node) override;
    void do_visit(sema::mir::LoopStmtMIR& node) override;
    void do_visit(sema::mir::GotoStmtMIR& node) override;
    void do_visit(sema::mir::BreakStmtMIR& node) override;
    void do_visit(sema::mir::ContStmtMIR& node) override;
    void do_visit(sema::mir::ReturnStmtMIR& node) override;

    void do_visit(sema::mir::BinaryExprMIR& node) override;
    void do_visit(sema::mir::UnaryExprMIR& node) override;
    void do_visit(sema::mir::CastExprMIR& node) override;
    void do_visit(sema::mir::AssignExprMIR& node) override;
    void do_visit(sema::mir::CondExprMIR& node) override;
    void do_visit(sema::mir::IdentExprMIR& node) override;
    void do_visit(sema::mir::LiteralExprMIR& node) override;
    void do_visit(sema::mir::CallExprMIR& node) override;
    void do_visit(sema::mir::MemberAccExprMIR& node) override;
    void do_visit(sema::mir::ReintExprMIR& node) override;
    void do_visit(sema::mir::SubscrExprMIR& node) override;
    void do_visit(sema::mir::PostfixExprMIR& node) override;
    void do_visit(sema::mir::SizeofExprMIR& node) override;

private:
    Box<ExprLIR> last_expr;

    /*
    The queue streaming system for LIRSynthesizer, which performs hoisting of function and
    variable declarations to the correct scope and ensures correct ordering of emitted LIR.

    When synthesizing LIR for a MIR node, we may encounter sub-nodes which will need to be
    hoisted to a higher scope once LIR is generated (e.g. nested function definitions, variable
    declarations (which must be at the top of the function, i.e. function-level scope)). To
    handle this, we maintain a stack of queues tracking the current nesting level. The `current_q`
    variable is the nesting level we are currently in.

    `current_q` accumulates emitted LIR items as we synthesize a MIR node. Once we finish
    synthesizing the MIR node, we flush the current queue into the queue at the top of the stack
    and pop the next queue from the stack to be the new current queue. This pushes all items
    into it, effectively hoisting the nested items to the next lower level. This continues up the
    nesting hierarchy until we reach the global level, at which point we emit all items in the
    current queue to the ProgramLIR.

    While draining the current queue, we also perform sorting of program items. Functions are sent
    to the next queue, variable declarations are sent to the next queue, and other statements are
    emitted in order. At the function level, functions are again emitted to the next queue, variable
    declarations are sent to locals, and other statements are emitted in order.

    This also ensures correct ordering of emitted LIR at the scope level. For example, in a
    function body, if we encounter a variable declaration after some statements, the variable
    declaration will be emitted before those statements in the final LIR. More importantly, if
    we encounter a nested function definition, it will be emitted separately from any other
    items in the same scope, so we don't accidentally emit a function definition in the middle
    of another function's body.
    */
    std::queue<LIRSynthItem> current_q;
    std::stack<std::queue<LIRSynthItem>> queue_stack;

    /*
    A stack for handling nested functions.
    */
    std::stack<LIRFuncSym *> func_stack;
};

} // namespace ecc::codegen::lir

#endif