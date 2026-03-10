#ifndef ECC_EXEC_H
#define ECC_EXEC_H

#include "ast/ast.hpp"
#include "value.hpp"
#include <stdfloat>

namespace ecc::exec {
/*
Compile-time evaluation functionality.
*/




class Evaluator {
public:
    Value eval(ast::ConstExpression *expr);

    Value eval(ast::BinaryExpression *expr);

    Value eval(ast::CastExpression *expr);

    Value eval(ast::UnaryExpression *expr);

    Value eval(ast::AssignmentExpression *expr);

    Value eval(ast::ConditionalExpression *expr);

    Value eval(ast::IdentifierExpression *expr);

    Value eval(ast::LiteralExpression *expr);

    Value eval(ast::StringExpression *expr);

    Value eval(ast::CallExpression *expr);

    Value eval(ast::MemberAccessExpression *expr);

    Value eval(ast::ArraySubscriptExpression *expr);

    Value eval(ast::PostfixExpression *expr);

    Value eval(ast::SizeofExpression *expr);

};


}

#endif