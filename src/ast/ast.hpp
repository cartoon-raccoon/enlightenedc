#ifndef ECC_AST_H
#define ECC_AST_H

#include <optional>
#include <vector>

#include "frontend/tokens.hpp"

/* Class definitions of AST nodes and subclasses. */
namespace ecc::ast {

using namespace ecc::tokens;

class ASTVisitor;

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

//* PROGRAM ITEMS

//
class ProgramItem : public ASTNode {
public:
    ~ProgramItem() = default;

    virtual void accept(ASTVisitor& visitor) = 0;
};

class Function : public ProgramItem {
public:
    ~Function() = default;

    void accept(ASTVisitor& visitor);
};


//* DECLARATIONS

// The Declaration abstract class that all declarations inherit from.
class Declaration : public ProgramItem {
public:
    ~Declaration() = default;

    void accept(ASTVisitor& visitor);
};


// Storage class specifiers.
class StorageClassSpecifier : public ASTNode {
public:
    ~StorageClassSpecifier() = default;

    enum SpecType {
        PUBLIC,
        STATIC,
        EXTERN,
    } spec;

    void accept(ASTVisitor& visitor);
};


// The struct or union specifier.
class StructOrUnionSpecifier : public ASTNode {
public:
    ~StructOrUnionSpecifier() = default;

    enum SpecType {
        STRUCT,
        UNION,
    } spec;

    std::optional<std::string> name;

    // struct declaration list

    void accept(ASTVisitor& visitor);
};


class EnumSpecifier : public ASTNode {
public:
    ~EnumSpecifier() = default;

    void accept(ASTVisitor& visitor);
};


class TypeSpecifier : public ASTNode {
public:
    ~TypeSpecifier() = default;

    // The type of specifier: primitive type, struct/union, or enum.
    enum SpecType {
        PRIMITIVE,
        STRUCT_OR_UNION,
        ENUM,
    } type;

    enum PrimitiveSpecifier {
        VOID,
        U8,
        U16,
        U32,
        U64,
        I0,
        I8,
        I16,
        I32,
        I64,
        F64,
        BOOL,
    };

    union {
        PrimitiveSpecifier primitive;
        StructOrUnionSpecifier *struct_or_union;
        EnumSpecifier *enum_;
    } spec;

    void accept(ASTVisitor& visitor);
};


//* STATEMENTS

// The abstract Statement class that all statements inherit from.
class Statement : public ProgramItem {
public:
    ~Statement() = default;

    void accept(ASTVisitor& visitor);
};


//* EXPRESSIONS

// The abstract Expression class that all expressions inherit from.
class Expression: public ASTNode {
public:
    virtual ~Expression() = default;

    virtual void accept(ASTVisitor& visitor) = 0;
};

class BinaryExpression : public Expression {
public:
    BinaryExpression(ASTNode *left, ASTNode *right, TokenType op)
        : left(left), right(right), op(op) {}
    
    ~BinaryExpression() {};

    // Left operand.
    ASTNode *left;
    // Right operand.
    ASTNode *right;
    // The operator.
    TokenType op;

    void accept(ASTVisitor& visitor);
};

class UnaryExpression : public Expression {
public:
    ~UnaryExpression() {};

    ASTNode *operand;
    TokenType op;

    void accept(ASTVisitor& visitor);

};


// The toplevel Program class.
class Program : public ASTNode {
public:
    ~Program() = default;

    // Program items.
    std::vector<ProgramItem *> items;

    void accept(ASTVisitor& visitor);
};


}

#endif // ECC_AST_H