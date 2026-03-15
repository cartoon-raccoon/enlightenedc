#include "exec.hpp"
#include "codegen/value.hpp"
#include "frontend/tokens.hpp"
#include "util.hpp"

namespace ecc::exec {

/* Helper function to throw an error if an expression can't be evaluated.
 */
inline void throwCompileTimeEvalError(const std::string& msg,
                               const ast::Expression* expr) {
    throw EccInvalidCompileTimeEval(msg, expr->loc);
}

/* The Evaluator class evaluates AST expressions at compile time.
 */
Value Evaluator::eval(ast::ConstExpression* expr) {
    return expr->inner.get()->accept(*this);
}

Value Evaluator::eval(ast::BinaryExpression* expr) {
    Value left = expr->left.get()->accept(*this);
    Value right = expr->right.get()->accept(*this);

    switch (expr->op) {
    case ecc::tokens::PLUS:
        return left + right;
        break;

    case ecc::tokens::MINUS:
        return left - right;
        break;

    case ecc::tokens::MUL:
        return left * right;
        break;

    case ecc::tokens::DIV:
        return left / right;
        break;

    default:
        throwCompileTimeEvalError("Unsupported binary operator", expr);
        break;
    }

    throwCompileTimeEvalError("Unsupported binary operation", expr);
}

Value Evaluator::eval(ast::CastExpression* expr) {
    Value val = expr->inner.get()->accept(*this);

   
    return val; // todo
}

Value Evaluator::eval(ast::UnaryExpression* expr) {
    Value operand = expr->operand.get()->accept(*this);

    switch (expr->op) {
    case ecc::tokens::INC:
        return ++operand;
        break;

    case ecc::tokens::DEC:
        return --operand;
        break;

    case ecc::tokens::NOT:
        return !operand;
        break;

    case ecc::tokens::TILDE:
        return ~operand;

    default:
        throwCompileTimeEvalError("unsupported unary operator", expr);
        break;
    }

    throwCompileTimeEvalError("unsupported unary expression", expr);
}

Value Evaluator::eval(ast::AssignmentExpression* expr) {
    throwCompileTimeEvalError("assignment expressions cannot be evaluated at compile time", expr);
}

Value Evaluator::eval(ast::ConditionalExpression* expr) {
    Value condition = expr->condition.get()->accept(*this);

    return condition ? expr->true_expr.get()->accept(*this)
                     : expr->false_expr.get()->accept(*this);
}

Value Evaluator::eval(ast::IdentifierExpression* expr) {

    auto sym = symtable.lookup(expr->name);
    if (!sym) {
        throwCompileTimeEvalError("Undefined variable", expr);
    }
    return std::monostate {}; // todo: add a Symbol * -> Value map for consts
}

Value Evaluator::eval(ast::LiteralExpression* expr) {

    switch (expr->kind) {
        case ast::LiteralExpression::INT:
            return Value(static_cast<long>(expr->value.i_val));
        case ast::LiteralExpression::FLOAT:
            return Value(expr->value.f_val);
        case ast::LiteralExpression::CHAR:
            return Value(expr->value.c_val);
        case ast::LiteralExpression::BOOL:
            return Value(expr->value.b_val);
    }
    throwCompileTimeEvalError("Unsupported literal kind", expr);
}

Value Evaluator::eval(ast::StringExpression* expr) {
    return Value(expr->value);
}

Value Evaluator::eval(ast::CallExpression* expr) {
    throwCompileTimeEvalError(
        "function calls cannot be evaluated at compile time", expr);
}

Value Evaluator::eval(ast::MemberAccessExpression* expr) {
    throwCompileTimeEvalError(
        "compile-time member access evaluation is not currently supported", expr);
}

Value Evaluator::eval(ast::ArraySubscriptExpression* expr) {
    throwCompileTimeEvalError(
        "compile-time array subscript evaluation is not currently supported", expr);
}

Value Evaluator::eval(ast::PostfixExpression* expr) {
    Value value = expr->operand.get()->accept(*this);

    if (expr->op == ecc::tokens::INC) {
        if (value.is<long>()) {
            return value++;
        }
        throwCompileTimeEvalError("Postfix increment requires an integer", expr);
    } else if (expr->op == ecc::tokens::DEC) {
        if (value.is<long>()) {
            return value--;
        }
        throwCompileTimeEvalError("Postfix decrement requires an integer", expr);
    }

    throwCompileTimeEvalError("Invalid postfix expression", expr);
}

Value Evaluator::eval(ast::SizeofExpression* expr) {
    throwCompileTimeEvalError("invalid sizeof operand", expr);
}

}