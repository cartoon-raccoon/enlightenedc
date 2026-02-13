#ifndef ECC_AST_H
#define ECC_AST_H

#include <vector>

#include "parser.hpp"
namespace ecc::ast {

using ecc::parser::Parser;

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
    virtual void accept(ASTVisitor& visitor) = 0;
};

class Program : public ASTNode {
public:
    ~Program() = default;

    // Program items.
    std::vector<ASTNode *> items;

    void accept(ASTVisitor& visitor);
};

class Function : public ASTNode {
public:
    ~Function() = default;

    void accept(ASTVisitor& visitor);
};

class Expression: public ASTNode {
public:
    virtual ~Expression() = default;

    virtual void accept(ASTVisitor& visitor) = 0;
};

class BinaryExpression : public Expression {
public:
    BinaryExpression(ASTNode *left, ASTNode *right, Parser::token op)
        : left(left), right(right), op(op) {}
    
    ~BinaryExpression() {};

    // Left operand.
    ASTNode *left;
    // Right operand.
    ASTNode *right;
    // The operator.
    Parser::token op;

    void accept(ASTVisitor& visitor);
};

class UnaryExpression : public Expression {
public:
    ~UnaryExpression() {};

    ASTNode *operand;
    Parser::token op;

    void accept(ASTVisitor& visitor);

};

}

#endif // ECC_AST_H