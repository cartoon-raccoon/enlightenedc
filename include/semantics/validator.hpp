#pragma once

#include "eval/value.hpp"
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

/**
A helper struct for tracking cases in a switch statement.
*/
class SwitchTracker {
    HashMap<eval::Value, Location, eval::ValueHash, eval::ValueStructEq> cases;
    Optional<Location> default_loc;

public:
    void insert_case(eval::Value& val, Location loc) { cases.insert_or_assign(val, loc); }

    Location get_loc(eval::Value& val) {
        auto it = cases.find(val);
        assert(it != cases.end() && "get_loc() called for a case that was never inserted");
        return it->second;
    }

    Optional<Location> get_default_loc() { return default_loc; }

    bool contains_case(eval::Value& val) { return cases.contains(val); }

    void set_default(Location loc) { default_loc = loc; }

    bool has_default() const { return default_loc.has_value(); }
};

/*
The class that performs type-checking and semantic validation.
*/
class Validator : public BaseMIRSemaVisitor, public NoMove {
    types::TypeContext& types;
    sym::SymbolTableWalker syms;

    Vec<SwitchTracker> switches;

public:
    Validator(sym::SymbolTable& syms, types::TypeContext& types)
        : BaseMIRSemaVisitor(State::READ), types(types), syms(syms) {}

    Vec<Box<EccSemError>> errors;

    template <typename E, typename... Args>
        requires std::derived_from<E, EccSemError>
    void add_error(Args... args) {
        Box<EccSemError> err = make_box<E>(args...);
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
    Chunk<mir::CastExprMIR> cast(types::Type *target, Chunk<mir::ExprMIR> expr);

    /**
    Decays expr into target.

    Concretely, this does the same thing as cast: it creates a new CastExprMIR,
    and returns it, but the difference is that the castkind is set to
    either ArrPtrDecay, or FuncPtrDecay.
    */
    Chunk<mir::CastExprMIR>
    decay(types::Type *target, Chunk<mir::ExprMIR> expr, bool is_funcdecay = false);

    Optional<types::Type *>
    eval_initializer(types::Type *type, mir::InitializerMIR& init, bool allow_size_infer = false);

    /**
    Check if an expression is tautological.
    */
    bool expr_is_tautological(mir::ExprMIR& expr);

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
        types::Type *type, Chunk<mir::ExprMIR>& expr, mir::InitializerMIR& init,
        bool allow_size_infer = false);

    void eval_initializer_rec_cls(
        types::AccessorPath& path, types::ClassType *cls,
        ds::ArenaVec<Chunk<mir::InitializerMIR>>& init);

    void eval_initializer_rec_arr(
        types::AccessorPath& path, types::ArrayType *arr,
        ds::ArenaVec<Chunk<mir::InitializerMIR>>& init);

    void validate_binexpr_nonprim(mir::BinaryExprMIR& node);

    void validate_binexpr_prim(mir::BinaryExprMIR& node);

    void validate_print(std::string& format_str, Span<Chunk<mir::ExprMIR>> args);
};

} // namespace ecc::sema

#endif