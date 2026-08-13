#pragma once

#include "util.hpp"
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
    types::TypeContext& types;
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
        return ScopeGuard<mir::MIRNode>(State::READ, syms, assoc);
    }

    /**
    Implicitly cast expr into target.

    Concretely, this creates a new CastExprMIR node, with node.castkind set to Implicit.
    */
    Box<mir::CastExprMIR> cast(types::Type *target, Box<mir::ExprMIR> expr);

    /**
    Decays expr into target.

    Concretely, this does the same thing as cast: it creates a new CastExprMIR,
    and returns it, but the difference is that the castkind is set to
    either ArrPtrDecay, or FuncPtrDecay.
    */
    Box<mir::CastExprMIR>
    decay(types::Type *target, Box<mir::ExprMIR> expr, bool is_funcdecay = false);

    Optional<types::Type *>
    eval_initializer(types::Type *type, mir::InitializerMIR& init, bool allow_size_infer = false);

    /**
    Checks if a given statement always returns.

    This is used to check if every path in a given statement always terminates in an
    (explicit) return statement. Implicit returns are not detected.
    */
    bool always_returns(mir::StmtMIR& node);

    void do_visit(mir::FunctionMIR& node) final;
    void do_visit(mir::InitializerMIR& node) final;
    void do_visit(mir::VarDeclMIR& node) final;
    void do_visit(mir::TypeDeclMIR& node) final;

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
    void do_visit(mir::ReintExprMIR& node) final;
    void do_visit(mir::SubscrExprMIR& node) final;
    void do_visit(mir::PostfixExprMIR& node) final;
    void do_visit(mir::SizeofExprMIR& node) final;

private:
    using Accessor = std::variant<std::string, size_t>;

    void visit_single_vardecl(sym::VarSymbol *varsym, mir::InitializerMIR& init);

    Optional<types::Type *> eval_initializer_rec(
        types::AccessorPath& path, types::Type *type, mir::InitializerMIR& init,
        bool allow_size_infer = false);

    Optional<types::Type *> eval_initializer_expr(
        types::Type *type, Box<mir::ExprMIR>& expr, mir::InitializerMIR& init,
        bool allow_size_infer = false);

    void eval_initializer_rec_cls(
        types::AccessorPath& path, types::ClassType *cls, Vec<Box<mir::InitializerMIR>>& init);

    void eval_initializer_rec_arr(
        types::AccessorPath& path, types::ArrayType *arr, Vec<Box<mir::InitializerMIR>>& init);

    void validate_print(std::string& format_str, Span<Box<mir::ExprMIR>> args);
};

} // namespace ecc::sema

#endif