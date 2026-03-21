#ifndef ECC_TYPECHECK_H
#define ECC_TYPECHECK_H

#include <variant>

#include "semantics/semantics.hpp"
#include "semantics/symbols.hpp"
#include "semantics/types.hpp"
#include "semantics/mir/mir.hpp"

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

    void eval_initializer(types::Type *type, mir::InitializerMIR& init);

protected:
    void do_visit(mir::InitializerMIR& node) override;
    void do_visit(mir::VarDeclMIR& node) override;

    void do_visit(mir::ExprStmtMIR& node) override;
    void do_visit(mir::SwitchStmtMIR& node) override;
    void do_visit(mir::CaseStmtMIR& node) override;
    void do_visit(mir::CaseRangeStmtMIR& node) override;
    void do_visit(mir::DefaultStmtMIR& node) override;
    void do_visit(mir::PrintStmtMIR& node) override;
    void do_visit(mir::IfStmtMIR& node) override;
    void do_visit(mir::LoopStmtMIR& node) override;
    void do_visit(mir::GotoStmtMIR& node) override;
    void do_visit(mir::BreakStmtMIR& node) override;
    void do_visit(mir::ContStmtMIR& node) override;
    void do_visit(mir::ReturnStmtMIR& node) override;

    void do_visit(mir::BinaryExprMIR& node) override;
    void do_visit(mir::UnaryExprMIR& node) override;
    void do_visit(mir::CastExprMIR& node) override;
    void do_visit(mir::AssignExprMIR& node) override;
    void do_visit(mir::CondExprMIR& node) override;
    void do_visit(mir::IdentExprMIR& node) override;
    void do_visit(mir::ConstExprMIR& node) override;
    void do_visit(mir::LiteralExprMIR& node) override;
    void do_visit(mir::CallExprMIR& node) override;
    void do_visit(mir::MemberAccExprMIR& node) override;
    void do_visit(mir::SubscrExprMIR& node) override;
    void do_visit(mir::PostfixExprMIR& node) override;
    void do_visit(mir::SizeofExprMIR& node) override;

private:
    using Accessor = std::variant<std::string, uint64_t>;

    void visit_single_vardecl(sym::VarSymbol *varsym, mir::InitializerMIR& init);

    void eval_initializer_rec(Vec<Accessor>& path, types::Type *type, mir::InitializerMIR& init);
};

}

#endif