#ifndef ECC_AST_H
#define ECC_AST_H

#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include "frontend/tokens.hpp"
#include "compiler/types.hpp"
#include "util.hpp"

/* Class definitions of AST nodes and subclasses. */
namespace ecc::ast {

using namespace ecc::tokens;
using namespace ecc::compiler;
using namespace util;

class ASTVisitor;

// The abstract class representing an AST node.
//
// Each AST node (binary, unary expr, statement, etc.) defines its own subclass
// that inherits from this main superclass.
class ASTNode {
  public:
    virtual ~ASTNode() = default;

    // Accept an AST visitor.
    virtual void accept(ASTVisitor& visitor) = 0;
};

//* PROGRAM ITEMS

/*
Abstract class denoting a program item: declaration, statement, or function definition.
*/
class ProgramItem : public ASTNode {
  public:
    ~ProgramItem() = default;

    virtual void accept(ASTVisitor& visitor) = 0;
};

// The abstract Expression class that all expressions inherit from.
class Expression : public ASTNode {
  public:
    virtual ~Expression() = default;

    // The type of the expression, populated during semantic elaboration.
    types::Type *type = nullptr;

    virtual void accept(ASTVisitor& visitor) = 0;
};

//* DECLARATIONS

/*
The Declaration abstract class that all declarations inherit from.
*/
class Declaration : public ProgramItem {
  public:
    Declaration() = default;
    ~Declaration() = default;

    void accept(ASTVisitor& visitor) override;
};

/*
Abstract class denoting a storage class or type specifier, or a type qualifier.
*/
class DeclarationSpecifier : public ASTNode {
  public:
    virtual ~DeclarationSpecifier() = default;
    virtual void accept(ASTVisitor& visitor) = 0;
};

class TypeQualifier : public DeclarationSpecifier {
  public:
    enum QualType { CONST };

    explicit TypeQualifier(QualType qual) : qual(qual) {}

    QualType qual;

    void accept(ASTVisitor& visitor) override;
};

class Pointer : public ASTNode {
  public:
    Pointer(Vec<Box<TypeQualifier>> qualifiers,
            std::optional<Box<Pointer>> nested)
        : qualifiers(std::move(qualifiers)), nested(std::move(nested)) {}

    Vec<Box<TypeQualifier>> qualifiers;
    std::optional<Box<Pointer>> nested;

    void accept(ASTVisitor& visitor) override;
};

class DirectDeclarator : public ASTNode {};

// Initializers
class Initializer : public ASTNode {
  public:
    Initializer(Box<Expression> expr) : expression(std::move(expr)) {}

    Initializer(Vec<Box<Initializer>> list)
        : initializer_list(std::move(list)) {}

    std::optional<Box<Expression>> expression;
    Vec<Box<Initializer>> initializer_list;

    void accept(ASTVisitor& visitor) override;
};

/*
A variable declarator (e.g. `x` in `U8 x`).
*/
class Declarator : public ASTNode {
  public:
    Declarator(std::optional<Box<Pointer>> pointer,
               std::optional<Box<DirectDeclarator>> direct)
        : pointer(std::move(pointer)), direct(std::move(direct)) {}

    std::optional<Box<Pointer>> pointer;
    std::optional<Box<DirectDeclarator>> direct;

    void accept(ASTVisitor& visitor) override;
};

/*
A declarator initializing a variable.
*/
class InitDeclarator : public ASTNode {
  public:
    InitDeclarator(Box<Declarator> declarator,
                   std::optional<Box<Initializer>> initializer)
        : declarator(std::move(declarator)),
          initializer(std::move(initializer)) {}

    Box<Declarator> declarator;
    std::optional<Box<Initializer>> initializer;

    void accept(ASTVisitor& visitor) override;
};

class ParameterDeclaration : public ASTNode {
  public:
    ParameterDeclaration(Vec<Box<DeclarationSpecifier>> specifiers,
                         std::optional<Box<Declarator>> declarator,
                         std::optional<Box<Expression>> default_value)
        : specifiers(std::move(specifiers)), declarator(std::move(declarator)),
          default_value(std::move(default_value)) {}

    Vec<Box<DeclarationSpecifier>> specifiers;
    std::optional<Box<Declarator>> declarator;
    std::optional<Box<Expression>> default_value;

    void accept(ASTVisitor& visitor) override;
};

/*
A variable declaration (e.g. `U32 x = 5;`, `struct Flags {U16 x; bool y} = {3, true}`).

A variable declaration with no init declarators is treated as a type declaration.
*/
class VariableDeclaration : public Declaration {
  public:
    VariableDeclaration(Vec<Box<DeclarationSpecifier>> specifiers,
                        Vec<Box<InitDeclarator>> declarators)
        : specifiers(std::move(specifiers)),
          declarators(std::move(declarators)) {}

    Vec<Box<DeclarationSpecifier>> specifiers;
    Vec<Box<InitDeclarator>> declarators;

    void accept(ASTVisitor& visitor) override;
};

class IdentifierDeclarator : public DirectDeclarator {
  public:
    std::string name;

    explicit IdentifierDeclarator(std::string n) : name(std::move(n)) {}

    void accept(ASTVisitor& visitor) override;
};

class ParenDeclarator : public DirectDeclarator {
  public:
    Box<Declarator> inner;

    explicit ParenDeclarator(std::unique_ptr<Declarator> d)
        : inner(std::move(d)) {}

    void accept(ASTVisitor& visitor) override;
};

class ArrayDeclarator : public DirectDeclarator {
  public:
    Box<DirectDeclarator> base;
    std::optional<Box<Expression>> size;

    ArrayDeclarator(std::unique_ptr<DirectDeclarator> b,
                    std::optional<std::unique_ptr<Expression>> s)
        : base(std::move(b)), size(std::move(s)) {}

    void accept(ASTVisitor& visitor) override;
};

class FunctionDeclarator : public DirectDeclarator {
  public:
    Box<DirectDeclarator> base;
    Vec<Box<ParameterDeclaration>> parameters;
    bool is_variadic;

    FunctionDeclarator(std::unique_ptr<DirectDeclarator> b,
                       std::vector<std::unique_ptr<ParameterDeclaration>> p,
                       bool v)
        : base(std::move(b)), parameters(std::move(p)), is_variadic(v) {}

    void accept(ASTVisitor& visitor) override;
};

class StructDeclarator : public ASTNode {
  public:
    StructDeclarator(std::optional<Box<Declarator>> declarator,
                     std::optional<Box<Expression>> bit_width)
        : declarator(std::move(declarator)), bit_width(std::move(bit_width)) {}

    std::optional<Box<Declarator>> declarator;
    std::optional<Box<Expression>> bit_width;

    void accept(ASTVisitor& visitor) override;
};

/*
Declaration of one or more struct members.
*/
class StructDeclaration : public ASTNode {
  public:
    StructDeclaration(Vec<Box<DeclarationSpecifier>> specifiers,
                      Vec<Box<StructDeclarator>> declarators)
        : specifiers(std::move(specifiers)),
          declarators(std::move(declarators)) {}

    Vec<Box<DeclarationSpecifier>> specifiers;
    Vec<Box<StructDeclarator>> declarators;

    void accept(ASTVisitor& visitor) override;
};

class Enumerator : public ASTNode {
  public:
    Enumerator(std::string name, std::optional<Box<Expression>> value)
        : name(std::move(name)), value(std::move(value)) {}

    std::string name;
    std::optional<Box<Expression>> value;

    void accept(ASTVisitor& visitor) override;
};

// Storage class specifiers.
class StorageClassSpecifier : public DeclarationSpecifier {
  public:
    enum SpecType { PUBLIC, STATIC, EXTERN };

    explicit StorageClassSpecifier(SpecType type) : type(type) {}

    SpecType type;

    void accept(ASTVisitor& visitor) override;
};

/*
A node denoting a struct or union and what members it contains.
*/
class StructOrUnionSpecifier : public ASTNode {
  public:
    enum Kind { STRUCT, UNION };

    Kind kind;
    std::optional<std::string> name;

    std::optional<Vec<Box<StructDeclaration>>> declarations;

    StructOrUnionSpecifier(
        Kind kind, std::optional<std::string> name,
        std::optional<Vec<Box<StructDeclaration>>> declarations)
        : kind(kind), name(std::move(name)),
          declarations(std::move(declarations)) {}

    void accept(ASTVisitor& visitor) override;
};

/*
A node denoting an enum and its contained variants.
*/
class EnumSpecifier : public ASTNode {
  public:
    EnumSpecifier(std::optional<std::string> name,
                  std::optional<Vec<Box<Enumerator>>> enumerators)
        : name(std::move(name)), enumerators(std::move(enumerators)) {}

    std::optional<std::string> name;

    std::optional<Vec<Box<Enumerator>>> enumerators;

    void accept(ASTVisitor& visitor) override;
};

class TypeSpecifier : public DeclarationSpecifier {
  public:
    enum Primitive {
        VOID,
        U0,
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
        BOOL
    };

    std::variant<Primitive, Box<StructOrUnionSpecifier>, Box<EnumSpecifier>>
        type;

    explicit TypeSpecifier(Primitive prim) : type(prim) {}

    explicit TypeSpecifier(Box<StructOrUnionSpecifier> s)
        : type(std::move(s)) {}

    explicit TypeSpecifier(Box<EnumSpecifier> e) : type(std::move(e)) {}

    void accept(ASTVisitor& visitor) override;
};

//* STATEMENTS

// The abstract Statement class that all statements inherit from.
class Statement : public ProgramItem {
  public:
    Statement() = default;
    ~Statement() = default;

    void accept(ASTVisitor& visitor) override;
};

class CompoundStatement : public Statement {
  public:
    CompoundStatement(Vec<Box<Declaration>> declarations,
                      Vec<Box<Statement>> statements)
        : declarations(std::move(declarations)),
          statements(std::move(statements)) {}

    Vec<Box<Declaration>> declarations;
    Vec<Box<Statement>> statements;

    void accept(ASTVisitor& visitor) override;
};

class ExpressionStatement : public Statement {
  public:
    explicit ExpressionStatement(std::optional<Box<Expression>> expression)
        : expression(std::move(expression)) {}

    std::optional<Box<Expression>> expression;

    void accept(ASTVisitor& visitor) override;
};

class CaseDefaultStatement : public Statement {
  public:
    enum Kind { CASE, CASE_RANGE, DEFAULT };

    CaseDefaultStatement(Kind kind,
                     std::optional<Box<Expression>> case_expr,
                     std::optional<Box<Expression>> case_range_end,
                     Box<Statement> statement)
        : kind(kind), case_expr(std::move(case_expr)),
          case_range_end(std::move(case_range_end)),
          statement(std::move(statement)) {}

    Kind kind;
    std::optional<Box<Expression>> case_expr;
    std::optional<Box<Expression>> case_range_end;
    Box<Statement> statement;

    void accept(ASTVisitor& visitor) override;
};

class LabeledStatement : public Statement {
  public:
    LabeledStatement(std::string label, Box<Statement> statement)
    : label(label), statement(std::move(statement)) {}

    std::string label;
    Box<Statement> statement;

    void accept(ASTVisitor& visitor) override;
};


class PrintStatement : public Statement {
  public:
    PrintStatement(std::string format_string, Vec<Box<Expression>> arguments)
        : format_string(std::move(format_string)),
          arguments(std::move(arguments)) {}

    std::string format_string;
    Vec<Box<Expression>> arguments;

    void accept(ASTVisitor& visitor) override;
};

class IfStatement : public Statement {
  public:
    IfStatement(Box<Expression> condition, Box<Statement> then_branch,
                std::optional<Box<Statement>> else_branch)
        : condition(std::move(condition)), then_branch(std::move(then_branch)),
          else_branch(std::move(else_branch)) {}

    Box<Expression> condition;
    Box<Statement> then_branch;
    std::optional<Box<Statement>> else_branch;

    void accept(ASTVisitor& visitor) override;
};

class SwitchStatement : public Statement {
  public:
    SwitchStatement(Box<Expression> condition, Box<Statement> body)
        : condition(std::move(condition)), body(std::move(body)) {}

    Box<Expression> condition;
    Box<Statement> body;

    void accept(ASTVisitor& visitor) override;
};

class WhileStatement : public Statement {
  public:
    WhileStatement(Box<Expression> condition, Box<Statement> body)
        : condition(std::move(condition)), body(std::move(body)) {}

    Box<Expression> condition;
    Box<Statement> body;

    void accept(ASTVisitor& visitor) override;
};

class DoWhileStatement : public Statement {
  public:
    DoWhileStatement(Box<Statement> body, Box<Expression> condition)
        : body(std::move(body)), condition(std::move(condition)) {}

    Box<Statement> body;
    Box<Expression> condition;

    void accept(ASTVisitor& visitor) override;
};

class ForStatement : public Statement {
  public:
    ForStatement(std::optional<Box<Expression>> init,
                 std::optional<Box<Expression>> condition,
                 std::optional<Box<Expression>> increment, Box<Statement> body)
        : init(std::move(init)), condition(std::move(condition)),
          increment(std::move(increment)), body(std::move(body)) {}

    std::optional<Box<Expression>> init;
    std::optional<Box<Expression>> condition;
    std::optional<Box<Expression>> increment;
    Box<Statement> body;

    void accept(ASTVisitor& visitor) override;
};

// FIXME: separate into separate goto, break, return statement classes
class JumpStatement : public Statement {
  public:
    enum Kind { GOTO, BREAK, RETURN };

    JumpStatement(Kind kind, std::string target_label,
                  std::optional<Box<Expression>> return_value)
        : kind(kind), target_label(std::move(target_label)),
          return_value(std::move(return_value)) {}

    Kind kind;
    std::string target_label;
    std::optional<Box<Expression>> return_value;

    void accept(ASTVisitor& visitor) override;
};

class TypeName : public ASTNode {
  public:
    TypeName(Vec<Box<DeclarationSpecifier>> specifiers,
             std::optional<Box<Declarator>> declarator)
        : specifiers(std::move(specifiers)), declarator(std::move(declarator)) {
    }

    Vec<Box<DeclarationSpecifier>> specifiers;
    std::optional<Box<Declarator>> declarator;

    void accept(ASTVisitor& visitor) override;
};

//* EXPRESSIONS

class BinaryExpression : public Expression {
  public:
    BinaryExpression(
        // Left operand.
        Box<Expression> left,
        // Right operand.
        Box<Expression> right,
        // The operator.
        TokenType op)
        : left(std::move(left)), right(std::move(right)), op(op) {}

    Box<Expression> left;
    Box<Expression> right;
    TokenType op;

    void accept(ASTVisitor& visitor) override;
};

class UnaryExpression : public Expression {
  public:
    UnaryExpression(Box<Expression> operand, TokenType op)
        : operand(std::move(operand)), op(op) {}

    Box<Expression> operand;
    TokenType op;

    void accept(ASTVisitor& visitor) override;
};

class AssignmentExpression : public Expression {
  public:
    AssignmentExpression(Box<Expression> left, Box<Expression> right,
                         TokenType op)
        : left(std::move(left)), right(std::move(right)), op(op) {}

    Box<Expression> left;
    Box<Expression> right;
    TokenType op;

    void accept(ASTVisitor& visitor) override;
};

class ConditionalExpression : public Expression {
  public:
    ConditionalExpression(Box<Expression> condition, Box<Expression> true_expr,
                          Box<Expression> false_expr)
        : condition(std::move(condition)), true_expr(std::move(true_expr)),
          false_expr(std::move(false_expr)) {}

    Box<Expression> condition;
    Box<Expression> true_expr;
    Box<Expression> false_expr;

    void accept(ASTVisitor& visitor) override;
};

class IdentifierExpression : public Expression {
  public:
    explicit IdentifierExpression(std::string name) : name(std::move(name)) {}

    std::string name;

    void accept(ASTVisitor& visitor) override;
};

// FIXME: Split off string into separate expression, unify remaining into union
class LiteralExpression : public Expression {
  public:
    enum Kind { INT, FLOAT, CHAR, BOOL, STRING };

    LiteralExpression(Kind kind, std::string value)
        : kind(kind), value(std::move(value)) {}

    Kind kind;
    std::string value;

    void accept(ASTVisitor& visitor) override;
};

class CallExpression : public Expression {
  public:
    CallExpression(Box<Expression> callee, Vec<Box<Expression>> arguments)
        : callee(std::move(callee)), arguments(std::move(arguments)) {}

    Box<Expression> callee;
    Vec<Box<Expression>> arguments;

    void accept(ASTVisitor& visitor) override;
};

class MemberAccessExpression : public Expression {
  public:
    MemberAccessExpression(Box<Expression> object, std::string member,
                           bool is_arrow)
        : object(std::move(object)), member(std::move(member)),
          is_arrow(is_arrow) {}

    Box<Expression> object;
    std::string member;
    bool is_arrow;

    void accept(ASTVisitor& visitor) override;
};

class ArraySubscriptExpression : public Expression {
  public:
    ArraySubscriptExpression(Box<Expression> array, Box<Expression> index)
        : array(std::move(array)), index(std::move(index)) {}

    Box<Expression> array;
    Box<Expression> index;

    void accept(ASTVisitor& visitor) override;
};

class PostfixExpression : public Expression {
  public:
    PostfixExpression(Box<Expression> operand, TokenType op)
        : operand(std::move(operand)), op(op) {}

    Box<Expression> operand;
    TokenType op;

    void accept(ASTVisitor& visitor) override;
};

class SizeofExpression : public Expression {
  public:
    std::variant<Box<Expression>, Box<TypeName>> operand;

    explicit SizeofExpression(Box<Expression> expr)
        : operand(std::move(expr)) {}

    explicit SizeofExpression(Box<TypeName> type) : operand(std::move(type)) {}

    void accept(ASTVisitor& visitor) override;
};

class Function : public ProgramItem {
  public:
    Function(Vec<Box<DeclarationSpecifier>> decl_spec_list,
             Box<Declarator> declarator, Box<CompoundStatement> statements)
        : decl_spec_list(std::move(decl_spec_list)),
          declarator(std::move(declarator)), statements(std::move(statements)) {
    }

    Function(Box<Declarator> declarator, Box<CompoundStatement> statements)
        : declarator(std::move(declarator)), statements(std::move(statements)) {
    }

    ~Function() = default;

    /*
    Any possible specifiers (e.g. public, int, etc.)
    */
    Vec<Box<DeclarationSpecifier>> decl_spec_list = {};
    /*
    The function name and its parameters.
    Note: If the declarator contains a pointer, the pointer applies to its return type.
    */
    Box<Declarator> declarator;
    Box<CompoundStatement> statements;

    void accept(ASTVisitor& visitor) override;
};

/*
The toplevel Program class.
*/
class Program : public ASTNode {
  public:
    Program() = default;
    ~Program() = default;

    // Program items.
    std::vector<std::unique_ptr<ProgramItem>> items;

    void accept(ASTVisitor& visitor) override;

    // Add a new program item.
    void add_item(std::unique_ptr<ProgramItem> item);
};


} // namespace ecc::ast

#endif // ECC_AST_H