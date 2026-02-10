#ifndef ECC_AST_VISITOR_H
#define ECC_AST_VISITOR_H

#include "ast/ast.hpp"

namespace ecc::ast {

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    // Visit an AST Node.
    virtual void visit(ASTNode& node);
};

}

#endif