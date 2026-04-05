#include "eval/consteval.hpp"

#include "eval/value.hpp"
#include "semantics/mir/mir.hpp"
#include "tokens.hpp"
#include "util.hpp"

using namespace ecc::sema::sym;
using namespace ecc::sema::types;
using namespace ecc::sema::mir;

namespace ecc::eval {

/* Helper function to throw an error if an expression can't be evaluated.
 */
inline void throw_eval_error(const std::string& msg, const ExprMIR& expr) {
    throw InvalidCompileTimeEval(msg, expr.loc);
}

/* The Evaluator class evaluates AST expressions at compile time.
 */
Value ConstEvaluator::eval(ConstExprMIR& expr) {
    return expr.inner->eval(*this);
}

Value ConstEvaluator::eval(BinaryExprMIR& expr) {
    Value left  = expr.left->eval(*this);
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

    case ecc::tokens::BinaryOp::EQ:
        return left == right;
        break;

    case ecc::tokens::BinaryOp::OROR:
        return left || right;
        break;

    case ecc::tokens::BinaryOp::ANDAND:
        return left && right;
        break;

    case ecc::tokens::BinaryOp::OR:
        return left | right;
        break;

    case ecc::tokens::BinaryOp::AND:
        return left & right;
        break;

    default:
        throw_eval_error("Unsupported binary operator", expr);
        break;
    }

    throw_eval_error("Unsupported binary operation", expr);
}

Value ConstEvaluator::eval(CastExprMIR& expr) {
    Value val = expr.inner->eval(*this);

    return val; // todo
}

Value ConstEvaluator::eval(UnaryExprMIR& expr) {
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

    case ecc::tokens::UnaryOp::NEG:
        return -operand;
        break;

    case ecc::tokens::UnaryOp::TILDE:
        return ~operand;
        break;

    default:
        throw_eval_error("unsupported unary operator", expr);
        break;
    }

    throw_eval_error("unsupported unary expression", expr);
}

Value ConstEvaluator::eval(AssignExprMIR& expr) {
    throw_eval_error("assignment expressions cannot be evaluated at compile time", expr);
}

Value ConstEvaluator::eval(CondExprMIR& expr) {
    Value condition = expr.condition->eval(*this);

    return condition ? expr.true_expr->eval(*this) : expr.false_expr->eval(*this);
}

Value ConstEvaluator::eval(IdentExprMIR& expr) {
    // attempt to resolve symbol to VarSymbol
    VarSymbol *varsym = expr.ident->as_varsym();
    if (!varsym) {
        throw_eval_error("provided identifier is not a valid symbol", expr);
    }
    if (varsym->value) {
        return *varsym->value;
    } else {
        throw InvalidCompileTimeEval("unable to resolve value of identifier", expr.loc);
    }
}

Value ConstEvaluator::eval(LiteralExprMIR& expr) {
    return expr.value;
}

Value ConstEvaluator::eval(CallExprMIR& expr) {
    throw_eval_error("function calls cannot be evaluated at compile time", expr);
}

Value ConstEvaluator::eval(MemberAccExprMIR& expr) {
    throw_eval_error("compile-time member access evaluation is not currently supported", expr);
}

Value ConstEvaluator::eval(SubscrExprMIR& expr) {
    throw_eval_error("compile-time array subscript evaluation is not currently supported", expr);
}

Value ConstEvaluator::eval(PostfixExprMIR& expr) {
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

    throw_eval_error("unsupported postfix operator", expr);
}

Value ConstEvaluator::eval(SizeofExprMIR& expr) {
    // match on inner
    // if type is function, return pointer instead
    throw_eval_error("invalid sizeof operand", expr); // fixme
}

} // namespace ecc::exec