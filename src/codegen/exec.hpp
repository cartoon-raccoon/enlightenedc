#ifndef ECC_EXEC_H
#define ECC_EXEC_H

#include <stdfloat>

#include "ast/ast.hpp"
#include "semantics/symbols.hpp"
#include "value.hpp"
#include "semantics/types.hpp"

namespace ecc::exec {
/*
Compile-time evaluation functionality.
*/

using namespace ecc;

/*
An AST walker for evaluating expressions at compile time.
*/
class Evaluator {
public:
    Evaluator(sema::sym::SymbolTable& symtable, sema::types::TypeContext& typectxt)
    : typectxt(typectxt), symtable(symtable) {}

    sema::types::TypeContext& typectxt;
    sema::sym::SymbolTable& symtable;
    
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