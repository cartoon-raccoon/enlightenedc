#pragma once

#ifndef ECC_TYPECHECK_H
#define ECC_TYPECHECK_H

#include <variant>

#include "semantics/semantics.hpp"
#include "semantics/symbols.hpp"
#include "semantics/types.hpp"
#include "semantics/mir/mir.hpp"
#include "semantics/semerr.hpp"

using namespace ecc;
using namespace util;

namespace ecc::sema {

/*
The class that performs type-checking and semantic validation.
*/
class Validator : public BaseMIRSemaVisitor {
public:
    Validator(sym::SymbolTable& syms, types::TypeContext& types)
        : BaseMIRSemaVisitor(State::READ, syms, types) {}

    Vec<Box<EccSemError>> errors;

    void eval_initializer(types::Type *type, mir::InitializerMIR& init);

    template<typename E, typename ... Args>
    requires std::derived_from<E, EccSemError>
    void add_error(Args ... args) {
        Box<EccSemError> err = std::make_unique<E>(args ...);
        errors.push_back(std::move(err));
    }

    void do_visit(mir::InitializerMIR& node) final override;
    void do_visit(mir::VarDeclMIR& node) final override;

    void do_visit(mir::ExprStmtMIR& node) final override;
    void do_visit(mir::SwitchStmtMIR& node) final override;
    void do_visit(mir::CaseStmtMIR& node) final override;
    void do_visit(mir::CaseRangeStmtMIR& node) final override;
    void do_visit(mir::DefaultStmtMIR& node) final override;
    void do_visit(mir::PrintStmtMIR& node) final override;
    void do_visit(mir::IfStmtMIR& node) final override;
    void do_visit(mir::LoopStmtMIR& node) final override;
    void do_visit(mir::GotoStmtMIR& node) final override;
    void do_visit(mir::BreakStmtMIR& node) final override;
    void do_visit(mir::ContStmtMIR& node) final override;
    void do_visit(mir::ReturnStmtMIR& node) final override;

    void do_visit(mir::BinaryExprMIR& node) final override;
    void do_visit(mir::UnaryExprMIR& node) final override;
    void do_visit(mir::CastExprMIR& node) final override;
    void do_visit(mir::AssignExprMIR& node) final override;
    void do_visit(mir::CondExprMIR& node) final override;
    void do_visit(mir::IdentExprMIR& node) final override;
    void do_visit(mir::ConstExprMIR& node) final override;
    void do_visit(mir::LiteralExprMIR& node) final override;
    void do_visit(mir::CallExprMIR& node) final override;
    void do_visit(mir::MemberAccExprMIR& node) final override;
    void do_visit(mir::SubscrExprMIR& node) final override;
    void do_visit(mir::PostfixExprMIR& node) final override;
    void do_visit(mir::SizeofExprMIR& node) final override;

private:
    using Accessor = std::variant<std::string, uint64_t>;

    void visit_single_vardecl(sym::VarSymbol *varsym, mir::InitializerMIR& init);

    void eval_initializer_rec(
        Vec<Accessor>& path, types::Type *type, mir::InitializerMIR& init);

    void eval_initializer_rec(
        Vec<Accessor>& path, types::ClassType *cls, Vec<Box<mir::InitializerMIR>>& init);

    void eval_initializer_rec(
        Vec<Accessor>& path, types::ArrayType *arr, Vec<Box<mir::InitializerMIR>>& init);
};

}

#endif