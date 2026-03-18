#include <variant>

#include "exec.hpp"
#include "eval/value.hpp"
#include "frontend/tokens.hpp"
#include "util.hpp"

using namespace ecc::sema::sym;
using namespace ecc::sema::types;

namespace ecc::exec {

/* Helper function to throw an error if an expression can't be evaluated.
 */
inline void throw_eval_error(const std::string& msg,
                               const ast::Expression* expr) {
    throw InvalidCompileTimeEval(msg, expr->loc);
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
        throw_eval_error("Unsupported binary operator", expr);
        break;
    }

    throw_eval_error("Unsupported binary operation", expr);
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
        throw_eval_error("unsupported unary operator", expr);
        break;
    }

    throw_eval_error("unsupported unary expression", expr);
}

Value Evaluator::eval(ast::AssignmentExpression* expr) {
    throw_eval_error("assignment expressions cannot be evaluated at compile time", expr);
}

Value Evaluator::eval(ast::ConditionalExpression* expr) {
    Value condition = expr->condition.get()->accept(*this);

    return condition ? expr->true_expr.get()->accept(*this)
                     : expr->false_expr.get()->accept(*this);
}

Value Evaluator::eval(ast::IdentifierExpression* expr) {
    // lookup symbol
    auto sym = symtable.lookup(expr->name);
    if (!sym) {
        throw_eval_error("undefined variable", expr);
    }
    // attempt to resolve symbol to VarSymbol
    VarSymbol *varsym = sym->as_varsym();
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
    throw_eval_error("Unsupported literal kind", expr);
}

Value Evaluator::eval(ast::StringExpression* expr) {
    return Value(expr->value);
}

Value Evaluator::eval(ast::CallExpression* expr) {
    throw_eval_error(
        "function calls cannot be evaluated at compile time", expr);
}

Value Evaluator::eval(ast::MemberAccessExpression* expr) {
    throw_eval_error(
        "compile-time member access evaluation is not currently supported", expr);
}

Value Evaluator::eval(ast::ArraySubscriptExpression* expr) {
    throw_eval_error(
        "compile-time array subscript evaluation is not currently supported", expr);
}

Value Evaluator::eval(ast::PostfixExpression* expr) {
    Value value = expr->operand.get()->accept(*this);

    if (expr->op == ecc::tokens::POSTINC) {
        if (value.is<long>()) {
            return value++;
        }
        throw_eval_error("Postfix increment requires an integer", expr);
    } else if (expr->op == ecc::tokens::POSTDEC) {
        if (value.is<long>()) {
            return value--;
        }
        throw_eval_error("Postfix decrement requires an integer", expr);
    }

    throw_eval_error("Invalid postfix expression", expr);
}

Value Evaluator::eval(ast::SizeofExpression* expr) {
    throw_eval_error("invalid sizeof operand", expr);
}

}