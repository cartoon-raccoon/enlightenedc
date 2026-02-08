#ifndef ECC_COMPILER_H

#include "ast/ast.hpp"

namespace ecc::compiler {

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    virtual void visit(ast::ASTNode *node);
};

}

#endif