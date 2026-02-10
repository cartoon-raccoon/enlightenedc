#ifndef ECC_AST_H
#define ECC_AST_H

namespace ecc::ast {

class ASTVisitor;

/* Class definitions of AST nodes and subclasses. */

// The abstract class representing an AST node.
//
// Each AST node (binary, unary expr, statement, etc.) defines its own subclass that inherits from this
// main superclass.
class ASTNode {
public:
    virtual ~ASTNode() = default;

    // Accept an AST visitor.
    virtual void accept(ASTVisitor& visitor);
};

class Expression: public ASTNode {
public:
    virtual ~Expression() {};

    virtual void accept(ASTVisitor& visitor);
};

class BinaryExpression: public Expression {
public:
    ~BinaryExpression() {};

    ASTNode *left;
    ASTNode *right;
    // token

    void accept(ASTVisitor& visitor);
};

}

#endif