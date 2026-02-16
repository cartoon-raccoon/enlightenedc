#ifndef ECC_COMPILER_H
#define ECC_COMPILER_H

#include "ast/ast.hpp"
#include "ast/visitor.hpp"

namespace ecc::compiler {

class LLVMVisitor : public ast::ASTVisitor {
  public:
    ~LLVMVisitor() = default;

    void visit(ast::ASTNode* node);

    // Generate the code after visiting the AST.
    void codegen();
};

} // namespace ecc::compiler

#endif