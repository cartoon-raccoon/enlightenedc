#pragma once
#ifndef ECC_OPTI_CONSTFOLD_H
#define ECC_OPTI_CONSTFOLD_H

#include "allocator/chunk.hpp"
#include "eval/consteval.hpp"
#include "semantics/mir/mir.hpp"
#include "semantics/semantics.hpp"
#include "semantics/symbols.hpp"
#include "semantics/types.hpp"
#include "util.hpp"

namespace ecc::opti {

using namespace ecc;
using namespace util;

class ConstantFolder : public sema::BaseMIRSemaVisitor, public NoMove {
public:
    ConstantFolder(sema::sym::SymbolTable& symt, sema::types::TypeContext& types)
        : sema::BaseMIRSemaVisitor(State::READ), syms(symt), types(types), evalr(syms, types) {}

private:
    sema::sym::SymbolTableWalker syms;
    Ref<sema::types::TypeContext> types;
    eval::ConstEvaluator evalr;

protected:
    sema::ScopeGuard<sema::mir::MIRNode>
    enter_scope(sema::sym::FuncSymbol *assoc = nullptr) override {
        return sema::ScopeGuard<sema::mir::MIRNode>(State::READ, syms, assoc);
    }

    Chunk<sema::mir::ExprMIR> eval_and_expr(Chunk<sema::mir::ExprMIR>&, Location);

    // Fold `operand` in place: replace it with a literal when it is
    // constant-foldable, otherwise recurse into it so nested constants fold.
    void fold_operand(Chunk<sema::mir::ExprMIR>& operand);

    void do_visit(sema::mir::InitializerMIR& node) override;
    void do_visit(sema::mir::ExprStmtMIR& node) override;
    void do_visit(sema::mir::ReturnStmtMIR& node) override;
    void do_visit(sema::mir::IfStmtMIR& node) override;
    void do_visit(sema::mir::LoopStmtMIR& node) override;
    void do_visit(sema::mir::SwitchStmtMIR& node) override;
    void do_visit(sema::mir::PrintStmtMIR& node) override;
    void do_visit(sema::mir::BinaryExprMIR& node) override;
    void do_visit(sema::mir::UnaryExprMIR& node) override;
    void do_visit(sema::mir::CastExprMIR& node) override;
    void do_visit(sema::mir::CondExprMIR& node) override;
    void do_visit(sema::mir::CallExprMIR& node) override;
    void do_visit(sema::mir::AssignExprMIR& node) override;
};

} // namespace ecc::opti

#endif