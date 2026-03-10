#include "exec.hpp"
#include "ast/ast.hpp"
#include "value.hpp"

using namespace ecc::exec;

Value Evaluator::eval(ast::ConstExpression *expr) {
    return std::monostate {};
}

Value Evaluator::eval(ast::BinaryExpression *expr) {
    return std::monostate {};
}

Value Evaluator::eval(ast::CastExpression *expr) {
    return std::monostate {};
}

Value Evaluator::eval(ast::UnaryExpression *expr) {
    return std::monostate {};
}

Value Evaluator::eval(ast::AssignmentExpression *expr) {
    return std::monostate {};
}

Value Evaluator::eval(ast::ConditionalExpression *expr) {
    return std::monostate {};
}

Value Evaluator::eval(ast::IdentifierExpression *expr) {
    return std::monostate {};
}

Value Evaluator::eval(ast::LiteralExpression *expr) {
    return std::monostate {};
}

Value Evaluator::eval(ast::StringExpression *expr) {
    return std::monostate {};
}

Value Evaluator::eval(ast::CallExpression *expr) {
    return std::monostate {};
}

Value Evaluator::eval(ast::MemberAccessExpression *expr) {
    return std::monostate {};
}

Value Evaluator::eval(ast::ArraySubscriptExpression *expr) {
    return std::monostate {};
}

Value Evaluator::eval(ast::PostfixExpression *expr) {
    return std::monostate {};
}

Value Evaluator::eval(ast::SizeofExpression *expr) {
    return std::monostate {};
}