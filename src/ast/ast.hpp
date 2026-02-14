#ifndef ECC_AST_H
#define ECC_AST_H

#include <optional>
#include <vector>
#include <memory>
#include <utility>

#include "frontend/tokens.hpp"
#include "util.hpp"

/* Class definitions of AST nodes and subclasses. */
namespace ecc::ast {

using namespace ecc::tokens;
using namespace util;

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


//* DECLARATIONS

// The Declaration abstract class that all declarations inherit from.
class Declaration : public ProgramItem {
public:
    Declaration() = default;
    ~Declaration() = default;

    void accept(ASTVisitor& visitor);
};


class DeclarationSpecifier : public ASTNode {
public:
    DeclarationSpecifier() = default;
    ~DeclarationSpecifier() = default;

    void accept(ASTVisitor& visitor);
};


class Declarator : public ASTNode {
public:
    Declarator() = default;
    ~Declarator() = default;
    void accept(ASTVisitor& visitor);
};

class VariableDeclaration : public Declaration {
public:
    VariableDeclaration(
        Vec<Box<DeclarationSpecifier>> specifiers,
        Vec<Box<Declarator>> declarators
    )
        : specifiers(std::move(specifiers)),
          declarators(std::move(declarators))
    {}

    Vec<Box<DeclarationSpecifier>> specifiers;
    Vec<Box<Declarator>> declarators;

    void accept(ASTVisitor& visitor);
};

class InitDeclarator : public ASTNode {
public:
    InitDeclarator(
        Box<Declarator> declarator,
        std::optional<Box<Expression>> initializer
    )
        : declarator(std::move(declarator)),
          initializer(std::move(initializer))
    {}

    Box<Declarator> declarator;
    std::optional<Box<Expression>> initializer;

    void accept(ASTVisitor& visitor);
};

class Pointer : public ASTNode {
public:
    Pointer(
        Vec<Box<DeclarationSpecifier>> qualifiers,
        Box<Pointer> nested
    )
        : qualifiers(std::move(qualifiers)),
          nested(std::move(nested))
    {}

    Vec<Box<DeclarationSpecifier>> qualifiers;
    Box<Pointer> nested;

    void accept(ASTVisitor& visitor);
};

class DirectDeclarator : public ASTNode {
public:
    enum Kind {
        IDENTIFIER,
        PAREN_DECLARATOR,
        ARRAY,
        FUNCTION
    };

    DirectDeclarator(
        Kind kind,
        std::string identifier,
        Box<Declarator> inner,
        std::optional<Box<Expression>> array_size,
        Vec<Box<Declaration>> parameters
    )
        : kind(kind),
          identifier(std::move(identifier)),
          inner(std::move(inner)),
          array_size(std::move(array_size)),
          parameters(std::move(parameters))
    {}

    Kind kind;
    std::string identifier;
    Box<Declarator> inner;
    std::optional<Box<Expression>> array_size;
    Vec<Box<Declaration>> parameters;

    void accept(ASTVisitor& visitor);
};

class StructDeclaration : public ASTNode {
public:
    StructDeclaration(
        Vec<Box<DeclarationSpecifier>> specifiers,
        Vec<Box<Declarator>> declarators
    )
        : specifiers(std::move(specifiers)),
          declarators(std::move(declarators))
    {}

    Vec<Box<DeclarationSpecifier>> specifiers;
    Vec<Box<Declarator>> declarators;

    void accept(ASTVisitor& visitor);
};

class Enumerator : public ASTNode {
public:
    Enumerator(
        std::string name,
        std::optional<Box<Expression>> value
    )
        : name(std::move(name)),
          value(std::move(value))
    {}

    std::string name;
    std::optional<Box<Expression>> value;

    void accept(ASTVisitor& visitor);
};

// Storage class specifiers.
class StorageClassSpecifier : public ASTNode {
public:
    enum SpecType {
        PUBLIC,
        STATIC,
        EXTERN,
    };

    explicit StorageClassSpecifier(SpecType spec)
        : spec(spec)
    {}

    SpecType spec;

    void accept(ASTVisitor& visitor);
};


// The struct or union specifier.
class StructOrUnionSpecifier : public ASTNode {
public:
    enum SpecType {
        STRUCT,
        UNION,
    };

    StructOrUnionSpecifier(
        SpecType spec,
        std::optional<std::string> name
    )
        : spec(spec),
          name(std::move(name))
    {}

    SpecType spec;

    std::optional<std::string> name;

    // struct declaration list

    void accept(ASTVisitor& visitor);
};


class EnumSpecifier : public ASTNode {
public:
    EnumSpecifier() = default;

    void accept(ASTVisitor& visitor);
};


class TypeSpecifier : public ASTNode {
public:

    // The type of specifier: primitive type, struct/union, or enum.
    enum SpecType {
        PRIMITIVE,
        STRUCT_OR_UNION,
        ENUM,
    };

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

    explicit TypeSpecifier(PrimitiveSpecifier primitive)
        : type(PRIMITIVE)
    {
        spec.primitive = primitive;
    }

    explicit TypeSpecifier(StructOrUnionSpecifier* s)
        : type(STRUCT_OR_UNION)
    {
        spec.struct_or_union = s;
    }

    explicit TypeSpecifier(EnumSpecifier* e)
        : type(ENUM)
    {
        spec.enum_ = e;
    }

    SpecType type;

    union {
        PrimitiveSpecifier primitive;
        StructOrUnionSpecifier* struct_or_union;
        EnumSpecifier* enum_;
    } spec;

    void accept(ASTVisitor& visitor);
};


//* STATEMENTS

// The abstract Statement class that all statements inherit from.
class Statement : public ProgramItem {
public:
    Statement() = default;
    ~Statement() = default;

    void accept(ASTVisitor& visitor);
};


class CompoundStatement : public Statement {
public:
    CompoundStatement(
        Vec<Box<Declaration>> declarations,
        Vec<Box<Statement>> statements
    )
        : declarations(std::move(declarations)),
          statements(std::move(statements))
    {}

    Vec<Box<Declaration>> declarations;
    Vec<Box<Statement>> statements;

    void accept(ASTVisitor& visitor);
};

class ExpressionStatement : public Statement {
public:
    explicit ExpressionStatement(std::optional<Box<Expression>> expression)
        : expression(std::move(expression))
    {}

    std::optional<Box<Expression>> expression;

    void accept(ASTVisitor& visitor);
};

class LabeledStatement : public Statement {
public:
    enum Kind {
        IDENTIFIER,
        CASE,
        CASE_RANGE,
        DEFAULT
    };

    LabeledStatement(
        Kind kind,
        std::string label,
        std::optional<Box<Expression>> case_expr,
        std::optional<Box<Expression>> case_range_end,
        Box<Statement> statement
    )
        : kind(kind),
          label(std::move(label)),
          case_expr(std::move(case_expr)),
          case_range_end(std::move(case_range_end)),
          statement(std::move(statement))
    {}

    Kind kind;
    std::string label;
    std::optional<Box<Expression>> case_expr;
    std::optional<Box<Expression>> case_range_end;
    Box<Statement> statement;

    void accept(ASTVisitor& visitor);
};

class PrintStatement : public Statement {
public:
    PrintStatement(
        std::string format_string,
        Vec<Box<Expression>> arguments
    )
        : format_string(std::move(format_string)),
          arguments(std::move(arguments))
    {}

    std::string format_string;
    Vec<Box<Expression>> arguments;

    void accept(ASTVisitor& visitor);
};

class IfStatement : public Statement {
public:
    IfStatement(
        Box<Expression> condition,
        Box<Statement> then_branch,
        std::optional<Box<Statement>> else_branch
    )
        : condition(std::move(condition)),
          then_branch(std::move(then_branch)),
          else_branch(std::move(else_branch))
    {}

    Box<Expression> condition;
    Box<Statement> then_branch;
    std::optional<Box<Statement>> else_branch;

    void accept(ASTVisitor& visitor);
};

class SwitchStatement : public Statement {
public:
    SwitchStatement(
        Box<Expression> condition,
        Box<Statement> body
    )
        : condition(std::move(condition)),
          body(std::move(body))
    {}

    Box<Expression> condition;
    Box<Statement> body;

    void accept(ASTVisitor& visitor);
};

class WhileStatement : public Statement {
public:
    WhileStatement(
        Box<Expression> condition,
        Box<Statement> body
    )
        : condition(std::move(condition)),
          body(std::move(body))
    {}

    Box<Expression> condition;
    Box<Statement> body;

    void accept(ASTVisitor& visitor);
};

class DoWhileStatement : public Statement {
public:
    DoWhileStatement(
        Box<Statement> body,
        Box<Expression> condition
    )
        : body(std::move(body)),
          condition(std::move(condition))
    {}

    Box<Statement> body;
    Box<Expression> condition;

    void accept(ASTVisitor& visitor);
};

class ForStatement : public Statement {
public:
    ForStatement(
        std::optional<Box<Expression>> init,
        std::optional<Box<Expression>> condition,
        std::optional<Box<Expression>> increment,
        Box<Statement> body
    )
        : init(std::move(init)),
          condition(std::move(condition)),
          increment(std::move(increment)),
          body(std::move(body))
    {}

    std::optional<Box<Expression>> init;
    std::optional<Box<Expression>> condition;
    std::optional<Box<Expression>> increment;
    Box<Statement> body;

    void accept(ASTVisitor& visitor);
};

class JumpStatement : public Statement {
public:
    enum Kind {
        GOTO,
        BREAK,
        RETURN
    };

    JumpStatement(
        Kind kind,
        std::string target_label,
        std::optional<Box<Expression>> return_value
    )
        : kind(kind),
          target_label(std::move(target_label)),
          return_value(std::move(return_value))
    {}

    Kind kind;
    std::string target_label;
    std::optional<Box<Expression>> return_value;

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
    BinaryExpression(
        // Left operand.
        Box<Expression> left,
        // Right operand.

        Box<Expression> right,
        // The operator.
        TokenType op
    )
        : left(std::move(left)),
          right(std::move(right)),
          op(op)
    {}

    Box<Expression> left;
    Box<Expression> right;
    TokenType op;

    void accept(ASTVisitor& visitor);
};

class UnaryExpression : public Expression {
public:
    UnaryExpression(
        Box<Expression> operand,
        TokenType op
    )
        : operand(std::move(operand)),
          op(op)
    {}

    Box<Expression> operand;
    TokenType op;

    void accept(ASTVisitor& visitor);
};

class AssignmentExpression : public Expression {
public:
    AssignmentExpression(
        Box<Expression> left,
        Box<Expression> right,
        TokenType op
    )
        : left(std::move(left)),
          right(std::move(right)),
          op(op)
    {}

    Box<Expression> left;
    Box<Expression> right;
    TokenType op;

    void accept(ASTVisitor& visitor);
};

class ConditionalExpression : public Expression {
public:
    ConditionalExpression(
        Box<Expression> condition,
        Box<Expression> true_expr,
        Box<Expression> false_expr
    )
        : condition(std::move(condition)),
          true_expr(std::move(true_expr)),
          false_expr(std::move(false_expr))
    {}

    Box<Expression> condition;
    Box<Expression> true_expr;
    Box<Expression> false_expr;

    void accept(ASTVisitor& visitor);
};

class IdentifierExpression : public Expression {
public:
    explicit IdentifierExpression(std::string name)
        : name(std::move(name))
    {}

    std::string name;

    void accept(ASTVisitor& visitor);
};

class LiteralExpression : public Expression {
public:
    enum Kind {
        INT,
        FLOAT,
        CHAR,
        STRING
    };

    LiteralExpression(
        Kind kind,
        std::string value
    )
        : kind(kind),
          value(std::move(value))
    {}

    Kind kind;
    std::string value;

    void accept(ASTVisitor& visitor);
};

class CallExpression : public Expression {
public:
    CallExpression(
        Box<Expression> callee,
        Vec<Box<Expression>> arguments
    )
        : callee(std::move(callee)),
          arguments(std::move(arguments))
    {}

    Box<Expression> callee;
    Vec<Box<Expression>> arguments;

    void accept(ASTVisitor& visitor);
};

class MemberAccessExpression : public Expression {
public:
    MemberAccessExpression(
        Box<Expression> object,
        std::string member,
        bool is_arrow
    )
        : object(std::move(object)),
          member(std::move(member)),
          is_arrow(is_arrow)
    {}

    Box<Expression> object;
    std::string member;
    bool is_arrow;

    void accept(ASTVisitor& visitor);
};

class ArraySubscriptExpression : public Expression {
public:
    ArraySubscriptExpression(
        Box<Expression> array,
        Box<Expression> index
    )
        : array(std::move(array)),
          index(std::move(index))
    {}

    Box<Expression> array;
    Box<Expression> index;

    void accept(ASTVisitor& visitor);
};

class PostfixExpression : public Expression {
public:
    PostfixExpression(
        Box<Expression> operand,
        TokenType op
    )
        : operand(std::move(operand)),
          op(op)
    {}

    Box<Expression> operand;
    TokenType op;

    void accept(ASTVisitor& visitor);

};

class SizeofExpression : public Expression {
public:
    SizeofExpression(
        std::optional<Box<Expression>> expression,
        std::optional<Box<ASTNode>> type_name
    )
        : expression(std::move(expression)),
          type_name(std::move(type_name))
    {}

    std::optional<Box<Expression>> expression;
    std::optional<Box<ASTNode>> type_name;

    void accept(ASTVisitor& visitor);
};


class Function : public ProgramItem {
public:
    Function(
        Vec<Box<DeclarationSpecifier>> decl_spec_list,
        Box<Declarator> declarator,
        Box<CompoundStatement> statements
    ) : 
    decl_spec_list(std::move(decl_spec_list)),
    declarator(std::move(declarator)),
    statements(std::move(statements))
    {}

    Function(
        Box<Declarator> declarator,
        Box<CompoundStatement> statements
    ) : 
    declarator(std::move(declarator)),
    statements(std::move(statements))
    {}

    ~Function() = default;

    Vec<Box<DeclarationSpecifier>> decl_spec_list = {};
    Box<Declarator> declarator;
    Box<CompoundStatement> statements;

    void accept(ASTVisitor& visitor);
};


// The toplevel Program class.
class Program : public ASTNode {
public:
    Program() = default;
    ~Program() = default;

    // Program items.
    std::vector<std::unique_ptr<ProgramItem>> items;

    void accept(ASTVisitor& visitor);

    // Add a new program item.
    void add_item(std::unique_ptr<ProgramItem> item);
};


}

#endif // ECC_AST_H