#ifndef ECC_EXEC_H
#define ECC_EXEC_H

#include <stdfloat>

#include "semantics/symbols.hpp"
#include "value.hpp"
#include "semantics/types.hpp"

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
}

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
    
    Value eval(sema::mir::ConstExprMIR& expr);

    Value eval(sema::mir::BinaryExprMIR& expr);

    Value eval(sema::mir::CastExprMIR& expr);

    Value eval(sema::mir::UnaryExprMIR& expr);

    Value eval(sema::mir::AssignExprMIR& expr);

    Value eval(sema::mir::CondExprMIR& expr);

    Value eval(sema::mir::IdentExprMIR& expr);

    Value eval(sema::mir::LiteralExprMIR& expr);

    Value eval(sema::mir::CallExprMIR& expr);

    Value eval(sema::mir::MemberAccExprMIR& expr);

    Value eval(sema::mir::SubscrExprMIR& expr);

    Value eval(sema::mir::PostfixExprMIR& expr);

    Value eval(sema::mir::SizeofExprMIR& expr);
};


}

#endif