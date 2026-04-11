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
    using namespace ecc::tokens;

    Value left  = expr.left->eval(*this);
    Value right = expr.right->eval(*this);

    switch (expr.op) {

    case BinaryOp::OROR:
        return left || right;
    case BinaryOp::ANDAND:
        return left && right;
    case BinaryOp::PLUS:
        return left + right;
    case BinaryOp::MINUS:
        return left - right;
    case BinaryOp::MUL:
        return left * right;
    case BinaryOp::DIV:
        return left / right;
    case BinaryOp::MOD:
        todo();
    case BinaryOp::EQ:
        return left == right;
    case BinaryOp::NE:
        return left != right;
    case BinaryOp::LT:
        return left < right;
    case BinaryOp::GT:
        return left > right;
    case BinaryOp::LE:
        return left <= right;
    case BinaryOp::GE:
        return left >= right;
    case BinaryOp::OR:
        return left | right;
    case BinaryOp::AND:
        return left & right;
    case BinaryOp::XOR:
        return left ^ right;
    case BinaryOp::LSHIFT: // todo

    case BinaryOp::RSHIFT:
        todo();
    case BinaryOp::BINCOMMA:
        return right;

    default:
        throw InvalidCompileTimeEval("unsupported binary operator", expr.loc);
    }
}

Value ConstEvaluator::eval(CastExprMIR& expr) {
    Value val    = expr.inner->eval(*this);
    auto *target = expr.target;

    using namespace ecc::sema::types;
    if (auto *prim = target->as_primitive()) {
        switch (prim->primkind) {
        case ecc::tokens::PrimType::BOOL:
            return Value(static_cast<bool>(val));

        case ecc::tokens::PrimType::F64: {
            if (val.is<double>())
                return val;
            if (val.is<long>())
                return Value((double)*val.value_as<long>());
            if (val.is<char>())
                return Value((double)*val.value_as<char>());
            if (val.is<bool>())
                return Value((double)*val.value_as<bool>());
            break;
        }

        default: {
            if (val.is<long>())
                return val;
            if (val.is<char>())
                return Value((long)*val.value_as<char>());
            if (val.is<bool>())
                return Value((long)*val.value_as<bool>());
            if (val.is<double>())
                return Value((long)*val.value_as<double>());
            break;
        }
        }

        throw_eval_error("Invalid primitive cast", expr);
    }

    if (target->as_enum()) {
        if (val.is<long>())
            return val;
        if (val.is<char>())
            return Value((long)*val.value_as<char>());
        throw_eval_error("Invalid enum cast", expr);
    }

    throw_eval_error("Unsupported cast in constant expression", expr);
}

Value ConstEvaluator::eval(UnaryExprMIR& expr) {
    using namespace ecc::tokens;
    Value operand = expr.operand->eval(*this);

    switch (expr.op) {
    case UnaryOp::INC:
        return ++operand;
        break;

    case UnaryOp::DEC:
        return --operand;
        break;

    case UnaryOp::NOT:
        return !operand;
        break;

    case UnaryOp::NEG:
        return -operand;
        break;

    case ecc::tokens::UnaryOp::TILDE:
        return ~operand;
        break;

    case UnaryOp::POS:
        return operand;

    case UnaryOp::REF:
    case UnaryOp::DEREF:
        throw_eval_error("Pointer operations not allowed in constant expressions", expr);

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
    using namespace sema::types;

    Type *target_type = nullptr;

    std::visit(util::match{[&](Box<ExprMIR>& e) {
                               if (!e->type) {
                                   throw_eval_error("sizeof expression has no type", expr);
                               }

                               target_type = e->type;
                               if (target_type->is_function()) {
                                   target_type = typectxt.get().get_pointer(target_type, false);
                               }
                           },

                           [&](Type *t) {
                               if (!t) {
                                   throw_eval_error("sizeof received null type", expr);
                               }

                               if (t->is_function()) {
                                   throw_eval_error("sizeof cannot be applied to function types",
                                                    expr);
                               }

                               target_type = t;
                           }

               },
               expr.operand);

    if (!target_type) {
        throw_eval_error("invalid sizeof operand", expr);
    }

    size_t size = target_type->alloc_size();

    return Value((long)size);
}

} // namespace ecc::eval