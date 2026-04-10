#pragma once
#include "semantics/mir/mir.hpp"
#ifndef ECC_OPTI_CONSTFOLD_H
#define ECC_OPTI_CONSTFOLD_H

#include "eval/consteval.hpp"
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
        : sema::BaseMIRSemaVisitor(State::WRITE), syms(symt), types(types), evalr(syms, types) {}

private:
    sema::sym::SymbolTableWalker syms;
    Ref<sema::types::TypeContext> types;
    eval::ConstEvaluator evalr;

protected:
    sema::ScopeGuard<sema::mir::MIRNode>
    enter_scope(sema::sym::FuncSymbol *assoc = nullptr) override {
        return sema::ScopeGuard<sema::mir::MIRNode>(state, syms, assoc);
    }

    Box<sema::mir::LiteralExprMIR> eval_and_expr(Box<sema::mir::ExprMIR>&, Location);

    void do_visit(sema::mir::ExprStmtMIR& node) override;
    void do_visit(sema::mir::BinaryExprMIR& node) override;
    void do_visit(sema::mir::UnaryExprMIR& node) override;
    void do_visit(sema::mir::CastExprMIR& node) override;
    void do_visit(sema::mir::CondExprMIR& node) override;
    void do_visit(sema::mir::ConstExprMIR& node) override;
    void do_visit(sema::mir::PostfixExprMIR& node) override;
};

} // namespace ecc::opti

#endif