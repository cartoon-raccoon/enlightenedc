#include "eval/consteval.hpp"

#include <stdexcept>

#include "eval/value.hpp"
#include "semantics/mir/mir.hpp"
#include "semantics/types.hpp"
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
        return left % right;
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
    case BinaryOp::LSHIFT:
        return left << right;
    case BinaryOp::RSHIFT:
        return left >> right;
    default:
        throw InvalidCompileTimeEval("unsupported binary operator", expr.loc);
    }
}

Value ConstEvaluator::eval(CastExprMIR& expr) {
    Value val    = expr.inner->eval(*this);
    Type *target = expr.target;
    if (target->is_enum()) {
        target = target->as_enum()->underlying;
    }

    using namespace ecc::sema::types;
    using namespace ecc::tokens;
    if (auto *prim = target->as_primitive()) {
        switch (prim->primkind) {
        case PrimType::U8:
            return val.cast<uint8_t>();
        case PrimType::U16:
            return val.cast<uint16_t>();
        case PrimType::U32:
            return val.cast<uint32_t>();
        case PrimType::U64:
            return val.cast<uint64_t>();
        case PrimType::I8:
            return val.cast<int8_t>();
        case PrimType::I16:
            return val.cast<int16_t>();
        case PrimType::I32:
            return val.cast<int32_t>();
        case PrimType::I64:
            return val.cast<int64_t>();
        case PrimType::F64:
            return val.cast<double>();
        case PrimType::BOOL:
            return val.cast<bool>();
        }
    }

    throw_eval_error("Unsupported cast in constant expression", expr);
}

Value ConstEvaluator::eval(UnaryExprMIR& expr) {
    using namespace ecc::tokens;
    Value operand = expr.operand->eval(*this);

    switch (expr.op) {
    case UnaryOp::NOT:
        return !operand;
        break;

    case UnaryOp::NEG:
        return -operand;
        break;

    case UnaryOp::TILDE:
        return ~operand;
        break;

    case UnaryOp::POS:
        return +operand;
        break;

    case UnaryOp::REF:
    case UnaryOp::DEREF:
        throw_eval_error("pointer operations not allowed in constant expressions", expr);

    case UnaryOp::INC:
    case UnaryOp::DEC:
        throw_eval_error("increment/decrement not allowed in constant expressions", expr);

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
        throw InvalidCompileTimeEval("provided identifier is not a valid symbol", expr.loc);
    }

    // the only identifiers allowed in compile time evaluation are enumerators
    if (varsym->get_type()->is_enum()) {
        EnumType *enmty = varsym->get_type()->as_enum();
        auto *mem       = enmty->find(varsym->name);
        if (!mem) {
            throw InvalidCompileTimeEval("could not resolve symbol", expr.loc);
        } else {
            return mem->value;
        }
    } else {
        throw InvalidCompileTimeEval("non-enumerators cannot be compile-time evaluated", expr.loc);
    }
}

Value ConstEvaluator::eval(LiteralExprMIR& expr) {
    if (auto *val = std::get_if<Value>(&expr.value)) {
        return *val;
    } else {
        throw InvalidCompileTimeEval("could not evaluate string literal at compile time", expr.loc);
    }
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
    throw_eval_error("increment/decrement not allowed in constant expressions", expr);
}

Value ConstEvaluator::eval(SizeofExprMIR& expr) {
    using namespace sema::types;

    Type *target_type = nullptr;

    std::visit(
        util::match{[&](Box<ExprMIR>& e) {
                        // only accept ident expressions
                        if (e->kind != MIRNode::NodeKind::IDENTEXPR_MIR) {
                            throw InvalidCompileTimeEval("invalid expression argument to sizeof",
                                                         e->loc);
                        }

                        IdentExprMIR *identexpr = dynamic_cast<IdentExprMIR *>(e.get());
                        if (!identexpr) {
                            throw std::runtime_error("could not cast ExprMIR to IdentExprMIR");
                        }
                        target_type = identexpr->ident->get_type();
                        if (target_type->is_function()) {
                            target_type = typectxt.get().get_pointer(target_type, false);
                        }
                    },

                    [&](Type *t) {
                        if (!t) {
                            throw_eval_error("sizeof received null type", expr);
                        }

                        if (t->is_function()) {
                            throw_eval_error("sizeof cannot be applied to function types", expr);
                        }

                        target_type = t;
                    }

        },
        expr.operand);

    if (!target_type) {
        throw_eval_error("invalid sizeof operand", expr);
    }

    size_t size = target_type->alloc_size();

    return Value(size);
}

} // namespace ecc::eval