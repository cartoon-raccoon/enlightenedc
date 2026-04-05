#pragma once

#ifndef ECC_EVAL_H
#define ECC_EVAL_H

#include "eval/value.hpp"

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

/**
An abstract class for evaluating expressions.
*/
class ExprEvaluator {
public:
    virtual ~ExprEvaluator() = default;

    virtual Value eval(sema::mir::ConstExprMIR& expr) = 0;

    virtual Value eval(sema::mir::BinaryExprMIR& expr) = 0;

    virtual Value eval(sema::mir::CastExprMIR& expr) = 0;

    virtual Value eval(sema::mir::UnaryExprMIR& expr) = 0;

    virtual Value eval(sema::mir::AssignExprMIR& expr) = 0;

    virtual Value eval(sema::mir::CondExprMIR& expr) = 0;

    virtual Value eval(sema::mir::IdentExprMIR& expr) = 0;

    virtual Value eval(sema::mir::LiteralExprMIR& expr) = 0;

    virtual Value eval(sema::mir::CallExprMIR& expr) = 0;

    virtual Value eval(sema::mir::MemberAccExprMIR& expr) = 0;

    virtual Value eval(sema::mir::SubscrExprMIR& expr) = 0;

    virtual Value eval(sema::mir::PostfixExprMIR& expr) = 0;

    virtual Value eval(sema::mir::SizeofExprMIR& expr) = 0;
};

} // namespace ecc::eval

#endif