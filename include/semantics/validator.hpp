#pragma once

#ifndef ECC_TYPECHECK_H
#define ECC_TYPECHECK_H

#include <variant>

#include "semantics/mir/mir.hpp"
#include "semantics/semantics.hpp"
#include "semantics/symbols.hpp"
#include "semantics/types.hpp"

using namespace ecc;
using namespace util;

namespace ecc::sema {

/*
The class that performs type-checking and semantic validation.
*/
class Validator : public BaseMIRSemaVisitor, public NoMove {
    Ref<types::TypeContext> types;
    sym::SymbolTableWalker syms;
public:
    Validator(sym::SymbolTable& syms, types::TypeContext& types)
        : BaseMIRSemaVisitor(State::READ), types(types), syms(syms) {}

    Vec<Box<EccSemError>> errors;

    template <typename E, typename... Args>
        requires std::derived_from<E, EccSemError>
    void add_error(Args... args) {
        Box<EccSemError> err = std::make_unique<E>(args...);
        errors.push_back(std::move(err));
    }

    void validate(mir::ProgramMIR& progmir);

protected:
    ScopeGuard<mir::MIRNode> enter_scope(sym::FuncSymbol *assoc = nullptr) override {
        return ScopeGuard<mir::MIRNode>(state, syms, assoc);
    }

    void eval_initializer(types::Type *type, mir::InitializerMIR& init);

    void do_visit(mir::InitializerMIR& node) final;
    void do_visit(mir::VarDeclMIR& node) final;

    void do_visit(mir::ExprStmtMIR& node) final;
    void do_visit(mir::SwitchStmtMIR& node) final;
    void do_visit(mir::CaseStmtMIR& node) final;
    void do_visit(mir::CaseRangeStmtMIR& node) final;
    void do_visit(mir::DefaultStmtMIR& node) final;
    void do_visit(mir::PrintStmtMIR& node) final;
    void do_visit(mir::IfStmtMIR& node) final;
    void do_visit(mir::LoopStmtMIR& node) final;
    void do_visit(mir::GotoStmtMIR& node) final;
    void do_visit(mir::BreakStmtMIR& node) final;
    void do_visit(mir::ContStmtMIR& node) final;
    void do_visit(mir::ReturnStmtMIR& node) final;

    void do_visit(mir::BinaryExprMIR& node) final;
    void do_visit(mir::UnaryExprMIR& node) final;
    void do_visit(mir::CastExprMIR& node) final;
    void do_visit(mir::AssignExprMIR& node) final;
    void do_visit(mir::CondExprMIR& node) final;
    void do_visit(mir::IdentExprMIR& node) final;
    void do_visit(mir::LiteralExprMIR& node) final;
    void do_visit(mir::CallExprMIR& node) final;
    void do_visit(mir::MemberAccExprMIR& node) final;
    void do_visit(mir::SubscrExprMIR& node) final;
    void do_visit(mir::PostfixExprMIR& node) final;
    void do_visit(mir::SizeofExprMIR& node) final;

private:
    using Accessor = std::variant<std::string, uint64_t>;

    void visit_single_vardecl(sym::VarSymbol *varsym, mir::InitializerMIR& init);

    void eval_initializer_rec(Vec<Accessor>& path, types::Type *type, mir::InitializerMIR& init);

    void eval_initializer_rec_cls(Vec<Accessor>& path, types::ClassType *cls,
                              Vec<Box<mir::InitializerMIR>>& init);

    void eval_initializer_rec_arr(Vec<Accessor>& path, types::ArrayType *arr,
                              Vec<Box<mir::InitializerMIR>>& init);
};

} // namespace ecc::sema

#endif