#pragma once

#ifndef ECC_TYPECHECK_H
#define ECC_TYPECHECK_H

#include "config.hpp"
#include "eval/value.hpp"
#include "prelude.hpp"
#include "semantics/mir/mir.hpp"
#include "semantics/semantics.hpp"
#include "semantics/symbols.hpp"
#include "semantics/types.hpp"

using namespace ecc;
using namespace util;

namespace ecc::sema {

class ExprValidator : public BaseMIRSemaVisitor, public Fallible, public NoMove {
    types::TypeContext& types;
    sym::SymbolTableWalker& syms;
public:
    ExprValidator(types::TypeContext& types, sym::SymbolTableWalker& syms)
        : BaseMIRSemaVisitor(State::READ), types(types), syms(syms) {}

    sym::SymbolTableWalker *symwalker() override { return &syms; }

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

    /**
    Check if an expression is tautological.
    */
    bool expr_is_tautological(mir::ExprMIR& expr);

    VISIT_NO_IMPL(mir::ProgramMIR);
    VISIT_NO_IMPL(mir::FunctionMIR);
    VISIT_NO_IMPL(mir::InitializerMIR);
    VISIT_NO_IMPL(mir::VarDeclMIR);
    VISIT_NO_IMPL(mir::TypeDeclMIR);

    VISIT_NO_IMPL(mir::ExprStmtMIR);
    VISIT_NO_IMPL(mir::CompoundStmtMIR);
    VISIT_NO_IMPL(mir::SwitchStmtMIR);
    VISIT_NO_IMPL(mir::CaseStmtMIR);
    VISIT_NO_IMPL(mir::CaseRangeStmtMIR);
    VISIT_NO_IMPL(mir::DefaultStmtMIR);
    VISIT_NO_IMPL(mir::PrintStmtMIR);
    VISIT_NO_IMPL(mir::IfStmtMIR);
    VISIT_NO_IMPL(mir::GotoStmtMIR);
    VISIT_NO_IMPL(mir::BreakStmtMIR);
    VISIT_NO_IMPL(mir::ContStmtMIR);
    VISIT_NO_IMPL(mir::ReturnStmtMIR);

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
    void validate_binexpr_nonprim(mir::BinaryExprMIR& node);

    void validate_binexpr_ptr_left(mir::BinaryExprMIR& node);

    void validate_binexpr_ptr_right(mir::BinaryExprMIR& node);

    void validate_binexpr_ptr_both(mir::BinaryExprMIR& node);

    void validate_binexpr_prim(mir::BinaryExprMIR& node);
};

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
        ECC_ASSERT(it != cases.end(), "get_loc() called for a case that was never inserted");
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
class Validator : public BaseMIRSemaVisitor, public Fallible, public NoMove {
    types::TypeContext& types;

    sym::SymbolTableWalker syms;
    RuntimeConfig& rtcfg;

    // keep the ExprValidator here, because it holds a reference to syms.
    ExprValidator exprv;

    Vec<SwitchTracker> switches;

public:
    Validator(sym::SymbolTable& symtab, types::TypeContext& types, RuntimeConfig& rtcfg)
        : BaseMIRSemaVisitor(State::READ), types(types), syms(symtab), rtcfg(rtcfg), exprv(types, syms) {}

    sym::SymbolTableWalker *symwalker() override { return &syms; }

    void validate(mir::ProgramMIR& progmir);

protected:

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
    void do_visit(mir::SubscrExprMIR& node) final;
    void do_visit(mir::PostfixExprMIR& node) final;
    void do_visit(mir::SizeofExprMIR& node) final;

private:

    /**
    The location of the main function, if found.
    */
    Optional<Location> main_loc;

    /**
    The location of the print function, if found.
    */
    Optional<Location> print_loc;

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

    void validate_print(StringRef format_str, Span<Chunk<mir::ExprMIR>> args);
};

} // namespace ecc::sema

#endif