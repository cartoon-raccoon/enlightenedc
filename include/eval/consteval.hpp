#pragma once

#ifndef ECC_CONSTEVAL_H
#define ECC_CONSTEVAL_H

#include <stdfloat>

#include "eval/evaluator.hpp"
#include "eval/value.hpp"
#include "semantics/symbols.hpp"
#include "semantics/types.hpp"
#include "util.hpp"

namespace ecc::sema::mir {
class ConstExprMIR;
class BinaryExprMIR;
class CastExprMIR;
class UnaryExprMIR;
class AssignExprMIR;
class CondExprMIR;
class IdentExprMIR;
class LiteralExprMIR;
class CallExprMIR;
class MemberAccExprMIR;
class SubscrExprMIR;
class PostfixExprMIR;
class SizeofExprMIR;
} // namespace ecc::sema::mir

namespace ecc::eval {
/*
Compile-time evaluation functionality.
*/

using namespace ecc;

/*
A MIR walker for evaluating expressions at compile time.
*/
class ConstEvaluator : public ExprEvaluator {
public:
    ConstEvaluator(sema::sym::SymbolTableWalker& symtable, sema::types::TypeContext& typectxt)
        : typectxt(typectxt), symtable(symtable) {}

    Ref<sema::types::TypeContext> typectxt;
    Ref<sema::sym::SymbolTableWalker> symtable;

    Value eval(sema::mir::ConstExprMIR& expr) override;

    Value eval(sema::mir::BinaryExprMIR& expr) override;

    Value eval(sema::mir::CastExprMIR& expr) override;

    Value eval(sema::mir::UnaryExprMIR& expr) override;

    Value eval(sema::mir::AssignExprMIR& expr) override;

    Value eval(sema::mir::CondExprMIR& expr) override;

    Value eval(sema::mir::IdentExprMIR& expr) override;

    Value eval(sema::mir::LiteralExprMIR& expr) override;

    Value eval(sema::mir::CallExprMIR& expr) override;

    Value eval(sema::mir::MemberAccExprMIR& expr) override;

    Value eval(sema::mir::SubscrExprMIR& expr) override;

    Value eval(sema::mir::PostfixExprMIR& expr) override;

    Value eval(sema::mir::SizeofExprMIR& expr) override;
};

} // namespace ecc::eval

#endif