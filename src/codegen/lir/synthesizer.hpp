#ifndef ECC_LIR_SYNTH_H
#define ECC_LIR_SYNTH_H

#include <stack>
#include <queue>

#include "semantics/semantics.hpp"
#include "semantics/symbols.hpp"
#include "semantics/types.hpp"
#include "semantics/mir/mir.hpp"
#include "codegen/lir/lir.hpp"
#include "codegen/lir/symbols.hpp"
#include "util.hpp"

using namespace ecc;
using namespace util;

namespace ecc::codegen::lir {

class LIRSynthesizer : public sema::BaseMIRSemaVisitor {
public:
    LIRSynthesizer(sema::sym::SymbolTable& syms, sema::types::TypeContext& types)
        : sema::BaseMIRSemaVisitor(State::READ, syms, types), 
        symbolmap() {} 

    LIRSymbolMap symbolmap;

    Box<ExprLIR> last_expr;

    void enqueue(Box<ProgItemLIR> item);
    Box<ProgItemLIR> dequeue();

    void push_queue();
    void pop_queue();

    bool curr_is_empty();
    
protected:
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
    void do_visit(sema::mir::ConstExprMIR& node) override;
    void do_visit(sema::mir::LiteralExprMIR& node) override;
    void do_visit(sema::mir::CallExprMIR& node) override;
    void do_visit(sema::mir::MemberAccExprMIR& node) override;
    void do_visit(sema::mir::SubscrExprMIR& node) override;
    void do_visit(sema::mir::PostfixExprMIR& node) override;
    void do_visit(sema::mir::SizeofExprMIR& node) override;

private:
    std::queue<Box<ProgItemLIR>> current_q;
    std::stack<std::queue<Box<ProgItemLIR>>> queue_stack;
};

}

#endif