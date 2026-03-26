#include <variant>

#include "exec.hpp"
#include "semantics/mir/mir.hpp"
#include "eval/value.hpp"
#include "frontend/tokens.hpp"
#include "util.hpp"

using namespace ecc::sema::sym;
using namespace ecc::sema::types;
using namespace ecc::sema::mir;

namespace ecc::exec {

/* Helper function to throw an error if an expression can't be evaluated.
 */
inline void throw_eval_error(const std::string& msg,
                               const ExprMIR& expr) {
    throw InvalidCompileTimeEval(msg, expr.loc);
}

/* The Evaluator class evaluates AST expressions at compile time.
 */
Value Evaluator::eval(ConstExprMIR& expr) {
    return expr.inner->eval(*this);
}

Value Evaluator::eval(BinaryExprMIR& expr) {
    Value left = expr.left->eval(*this);
    Value right = expr.right->eval(*this);

    switch (expr.op) {
    case ecc::tokens::BinaryOp::PLUS:
        return left + right;
        break;

    case ecc::tokens::BinaryOp::MINUS:
        return left - right;
        break;

    case ecc::tokens::BinaryOp::MUL:
        return left * right;
        break;

    case ecc::tokens::BinaryOp::DIV:
        return left / right;
        break;

    default:
        throw_eval_error("Unsupported binary operator", expr);
        break;
    }

    throw_eval_error("Unsupported binary operation", expr);
}

Value Evaluator::eval(CastExprMIR& expr) {
    Value val = expr.inner->eval(*this);

   
    return val; // todo
}

Value Evaluator::eval(UnaryExprMIR& expr) {
    Value operand = expr.operand->eval(*this);

    switch (expr.op) {
    case ecc::tokens::UnaryOp::INC:
        return ++operand;
        break;

    case ecc::tokens::UnaryOp::DEC:
        return --operand;
        break;

    case ecc::tokens::UnaryOp::NOT:
        return !operand;
        break;

    case ecc::tokens::UnaryOp::TILDE:
        return ~operand;

    default:
        throw_eval_error("unsupported unary operator", expr);
        break;
    }

    throw_eval_error("unsupported unary expression", expr);
}

Value Evaluator::eval(AssignExprMIR& expr) {
    throw_eval_error("assignment expressions cannot be evaluated at compile time", expr);
}

Value Evaluator::eval(CondExprMIR& expr) {
    Value condition = expr.condition->eval(*this);

    return condition ? expr.true_expr->eval(*this)
                     : expr.false_expr->eval(*this);
}

Value Evaluator::eval(IdentExprMIR& expr) {
    // attempt to resolve symbol to VarSymbol
    VarSymbol *varsym = expr.ident->as_varsym();
    if (!varsym) {
        throw_eval_error("provided identifier is not a valid symbol", expr);
    }
    if (varsym->value) {
        return *varsym->value;
    } else {
        throw_eval_error("unable to resolve value of identifier", expr);
    }

    return std::monostate {};
}

Value Evaluator::eval(LiteralExprMIR& expr) {
    return expr.value;
}

Value Evaluator::eval(CallExprMIR& expr) {
    throw_eval_error(
        "function calls cannot be evaluated at compile time", expr);
}

Value Evaluator::eval(MemberAccExprMIR& expr) {
    throw_eval_error(
        "compile-time member access evaluation is not currently supported", expr);
}

Value Evaluator::eval(SubscrExprMIR& expr) {
    throw_eval_error(
        "compile-time array subscript evaluation is not currently supported", expr);
}

Value Evaluator::eval(PostfixExprMIR& expr) {
    Value value = expr.operand->eval(*this);

    if (expr.op == ecc::tokens::PostfixOp::POSTINC) {
        if (value.is<long>()) {
            return value++;
        }
        throw_eval_error("Postfix increment requires an integer", expr);
    } else if (expr.op == ecc::tokens::PostfixOp::POSTDEC) {
        if (value.is<long>()) {
            return value--;
        }
        throw_eval_error("Postfix decrement requires an integer", expr);
    }

    throw_eval_error("Invalid postfix expression", expr);
}

Value Evaluator::eval(SizeofExprMIR& expr) {
    throw_eval_error("invalid sizeof operand", expr);
}

}