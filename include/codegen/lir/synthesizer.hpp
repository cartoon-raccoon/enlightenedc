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

    LIRSymbolMap& symbolmap;

    ProgramLIR& prog_lir;

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
    void do_visit(sema::mir::SubscrExprMIR& node) override;
    void do_visit(sema::mir::PostfixExprMIR& node) override;
    void do_visit(sema::mir::SizeofExprMIR& node) override;

private:
    Box<ExprLIR> last_expr;

    std::queue<LIRSynthItem> current_q;
    std::stack<std::queue<LIRSynthItem>> queue_stack;
    std::stack<LIRFuncSym *> func_stack;
};

} // namespace ecc::codegen::lir

#endif