#ifndef ECC_AST_H
#define ECC_AST_H

#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include "frontend/tokens.hpp"
#include "eval/value.hpp"
#include "util.hpp"

using namespace ecc;
using namespace ecc::util;

namespace ecc::exec {
    class Evaluator;
}

/* Class definitions of AST nodes and subclasses. */
namespace ecc::ast {

class ASTVisitor;

// The abstract class representing an AST node.
//
// Each AST node (binary, unary expr, statement, etc.) defines its own subclass
// that inherits from this main superclass.
class ASTNode {
public:

    util::Location loc;

    enum NodeKind {
        TYPE_QUAL,
        STORAGE_SPEC,
        POINTER,
        DIRECT_DECLTR,
        INITIALIZER,
        DECLARATOR,
        INIT_DECLTR,
        PARAM_DECL,
        TYPE_DECL,
        VAR_DECL,
        IDENT_DECLTR,
        PAREN_DECLTR,
        ARR_DECLTR,
        FUNC_DECLTR,
        CLASS_DECLTR,
        CLASS_DECL,
        ENUMERATOR,
        CLASS_SPEC,
        UNION_SPEC,
        ENUM_SPEC,
        VOID_SPEC,
        PRIM_SPEC,
        COMP_STMT,
        EXPR_STMT,
        CASE_STMT,
        CASE_RG_STMT,
        DEF_STMT,
        LABEL_STMT,
        PRINT_STMT,
        IF_STMT,
        SWITCH_STMT,
        WHILE_STMT,
        DO_WHILE_STMT,
        FOR_STMT,
        GOTO_STMT,
        BREAK_STMT,
        RET_STMT,
        TYPE_NAME,
        CONST_EXPR,
        BIN_EXPR,
        CAST_EXPR,
        UN_EXPR,
        ASSGN_EXPR,
        COND_EXPR,
        IDENT_EXPR,
        LIT_EXPR,
        STR_EXPR,
        CALL_EXPR,
        ACCESS_EXPR,
        SUBSCR_EXPR,
        POSTF_EXPR,
        SIZEOF_EXPR,
        FUNC,
        PROG,
    } kind;

    ASTNode(NodeKind kind, Location loc) : kind(kind), loc(loc) {}
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
    ProgramItem(NodeKind kind, Location loc) : ASTNode(kind, loc)  {}
    ~ProgramItem() = default;

    virtual void accept(ASTVisitor& visitor) = 0;
};

// The abstract Expression class that all expressions inherit from.
class Expression : public ASTNode {
public:
    Expression(NodeKind kind, Location loc) : ASTNode(kind, loc)  {}
    virtual ~Expression() = default;

    // Whether or not the expression can be computed at compile time.
    //virtual bool is_compiletime_computable() = 0;

    virtual void accept(ASTVisitor& visitor) = 0;

    virtual exec::Value accept(ecc::exec::Evaluator& eval) = 0;
};

/*
A wrapper to indicate that the contained expression must be computable at compile time.

Any Expression wrapped in a ConstExpression can be treated as if it is computable at
compile time, and any expression that is not cannot be.
*/
class ConstExpression : public Expression {
public:
    ConstExpression(Box<Expression> expr)
        : Expression(CONST_EXPR, expr->loc),
        inner(std::move(expr)) {}

    Box<Expression> inner;

    void accept(ASTVisitor& visitor) override;

    exec::Value accept(exec::Evaluator& eval) override;
};

//* DECLARATIONS

/*
The Declaration abstract class that all declarations inherit from.
*/
class Declaration : public ProgramItem {
public:
    Declaration(NodeKind kind, Location loc) : ProgramItem(kind, loc)  {}
    ~Declaration() = default;

    virtual void accept(ASTVisitor& visitor) = 0;
};

/*
Abstract class denoting a storage class or type specifier, or a type qualifier.
*/
class DeclarationSpecifier : public ASTNode {
public:
    DeclarationSpecifier(NodeKind kind, Location loc) : ASTNode(kind, loc)  {}
    virtual ~DeclarationSpecifier() = default;

    virtual void accept(ASTVisitor& visitor) = 0;
};

class TypeQualifier : public DeclarationSpecifier {
public:
    enum QualType { CONST };

    TypeQualifier(Location loc, QualType qual)
        : DeclarationSpecifier(TYPE_QUAL, loc), qual(qual) {}

    QualType qual;

    void accept(ASTVisitor& visitor) override;
};

// Storage class specifiers (public, static, extern).
class StorageClassSpecifier : public DeclarationSpecifier {
public:
    enum SpecType { PUBLIC, STATIC, EXTERN, EXTERNC };

    StorageClassSpecifier(Location loc, SpecType type)
        : DeclarationSpecifier(STORAGE_SPEC, loc), type(type) {}

    SpecType type;

    void accept(ASTVisitor& visitor) override;
};

class Pointer : public ASTNode {
public:
    Pointer(Location loc, Vec<Box<TypeQualifier>> qualifiers,
            std::optional<Box<Pointer>> nested)
        : ASTNode(POINTER, loc), 
        qualifiers(std::move(qualifiers)), 
        nested(std::move(nested)) {}

    Vec<Box<TypeQualifier>> qualifiers;
    std::optional<Box<Pointer>> nested;

    void accept(ASTVisitor& visitor) override;
};

/*
A non-pointer declarator abstract class.
*/
class DirectDeclarator : public ASTNode {
public:
    DirectDeclarator(NodeKind kind, Location loc) : ASTNode(kind, loc)  {}
    ~DirectDeclarator() = default;

    virtual void accept(ASTVisitor& visitor) = 0;
};

/*
An initializer for a variable.

Compound-type variables use the Vec<Box<Initializer>> variant of `initializer`,
whereas primitive type variables use the Box<Expression> variant.
*/
class Initializer : public ASTNode {
public:
    Initializer(Location loc, Box<Expression> expr)
        : ASTNode(INITIALIZER, loc), initializer(std::move(expr)) {}

    Initializer(Location loc, Vec<Box<Initializer>> list)
        : ASTNode(INITIALIZER, loc), initializer(std::move(list)) {}

    std::variant<Box<Expression>, Vec<Box<Initializer>>> initializer;

    void accept(ASTVisitor& visitor) override;
};

/*
A general declarator containing a DirectDeclarator and an optional Pointer.
*/
class Declarator : public ASTNode {
public:
    Declarator(Location loc, std::optional<Box<Pointer>> pointer,
               std::optional<Box<DirectDeclarator>> direct)
        : ASTNode(DECLARATOR, loc), 
        pointer(std::move(pointer)), 
        direct(std::move(direct)) {}

    std::optional<Box<Pointer>> pointer;
    std::optional<Box<DirectDeclarator>> direct;

    void accept(ASTVisitor& visitor) override;
};

/*
A declarator creating one or more new variables, with optional initializers. 
*/
class InitDeclarator : public ASTNode {
public:
    InitDeclarator(Location loc, Box<Declarator> declarator,
                   std::optional<Box<Initializer>> initializer)
        : ASTNode(INIT_DECLTR, loc),
        declarator(std::move(declarator)),
        initializer(std::move(initializer)) {}

    Box<Declarator> declarator;
    std::optional<Box<Initializer>> initializer;

    void accept(ASTVisitor& visitor) override;
};

/*
A declaration of a single function parameter.
*/
class ParameterDeclaration : public Declaration {
public:
    ParameterDeclaration(Location loc,
                         Vec<Box<DeclarationSpecifier>> specifiers,
                         std::optional<Box<Declarator>> declarator,
                         std::optional<Box<Expression>> default_value)
        : Declaration(PARAM_DECL, loc),
        specifiers(std::move(specifiers)), 
        declarator(std::move(declarator)),
        default_value(std::move(default_value)) {}

    Vec<Box<DeclarationSpecifier>> specifiers;
    std::optional<Box<Declarator>> declarator;
    std::optional<Box<Expression>> default_value;

    void accept(ASTVisitor& visitor) override;
};

/*
A Declaration of a type (i.e. a declaration of only specifiers, no InitDeclarators).
*/
class TypeDeclaration : public Declaration {
public:
    TypeDeclaration(Location loc, Vec<Box<DeclarationSpecifier>> specifiers)
        : Declaration(TYPE_DECL, loc),
        specifiers(std::move(specifiers)) {}

    Vec<Box<DeclarationSpecifier>> specifiers;

    void accept(ASTVisitor& visitor) override;
};

/*
A variable declaration (e.g. `U32 x = 5;`, `class Flags {U16 x; bool y} = {3, true}`).
*/
class VariableDeclaration : public Declaration {
public:
    VariableDeclaration(Location loc,
                        Vec<Box<DeclarationSpecifier>> specifiers,
                        Vec<Box<InitDeclarator>> declarators)
        : Declaration(VAR_DECL, loc),
        specifiers(std::move(specifiers)),
        declarators(std::move(declarators)) {}

    Vec<Box<DeclarationSpecifier>> specifiers;
    Vec<Box<InitDeclarator>> declarators;

    void accept(ASTVisitor& visitor) override;
};

class IdentifierDeclarator : public DirectDeclarator {
public:
    std::string name;

    IdentifierDeclarator(Location loc, std::string n) 
        : DirectDeclarator(IDENT_DECLTR, loc),
        name(std::move(n)) {}

    void accept(ASTVisitor& visitor) override;
};

/*
A declarator surrounded by parentheses (e.g. `(* decl)`).

The parentheses force any pointers to bind to the name before any
other operator (e.g. FunctionDeclarator, ArrayDeclarator).

So, for example, `U32 *somefunction()` defines the symbol `somefunction`
as a function that returns a pointer to a U32, while `U32 (*somefunction) ()`
defines the symbol `somefunction` as a pointer to a function that returns an
integer. Similarly, `U32 *arr[5]` defines `arr` as an array of 5 U32 pointers,
whereas U32 (* arr) [5] defines `arr` as a pointer to an array of 5 U32s.

The case of arrays has implications on pointer arithmetic. In the first case,
`arr + 1` is equivalent to arr[1], so `arr` will increase by 4 bytes, the size
of a U32. However, on the second case, `arr + 1` is equivalent to `arr + 5`,
since the base type of `arr` in this case is `U32 [5]`, whereas the base type
of `arr` in the former is `U32`.

*/
class ParenDeclarator : public DirectDeclarator {
public:
    ParenDeclarator(Location loc, Box<Declarator> d)
        : DirectDeclarator(PAREN_DECLTR, loc),
        inner(std::move(d)) {}

    Box<Declarator> inner;

    void accept(ASTVisitor& visitor) override;
};

class ArrayDeclarator : public DirectDeclarator {
public:
    ArrayDeclarator(Location loc,
                    Box<DirectDeclarator> b,
                    std::optional<Box<ConstExpression>> s)
        : DirectDeclarator(ARR_DECLTR, loc),
        base(std::move(b)), 
        size(std::move(s)) {}

    Box<DirectDeclarator> base;
    std::optional<Box<ConstExpression>> size;

    void accept(ASTVisitor& visitor) override;
};

class FunctionDeclarator : public DirectDeclarator {
public:
    FunctionDeclarator(Location loc,
                       Box<DirectDeclarator> b,
                       std::vector<Box<ParameterDeclaration>> p,
                       bool v)
        : DirectDeclarator(FUNC_DECLTR, loc),
        base(std::move(b)),
        parameters(std::move(p)),
        is_variadic(v) {}

    Box<DirectDeclarator> base;
    Vec<Box<ParameterDeclaration>> parameters;
    bool is_variadic;

    void accept(ASTVisitor& visitor) override;
};

/*
A declarator representing a member of a class.
*/
class ClassDeclarator : public ASTNode {
public:
    ClassDeclarator(Location loc,
                    std::optional<Box<Declarator>> declarator,
                    std::optional<Box<Expression>> bit_width)
        : ASTNode(CLASS_DECLTR, loc),
        declarator(std::move(declarator)),
        bit_width(std::move(bit_width)) {}

    std::optional<Box<Declarator>> declarator;
    std::optional<Box<Expression>> bit_width;

    void accept(ASTVisitor& visitor) override;
};

/*
Declaration of one or more class members. Each declaration contains
one or more ClassDeclarators.
*/
class ClassDeclaration : public Declaration {
public:
    ClassDeclaration(Location loc,
                     Vec<Box<DeclarationSpecifier>> specifiers,
                     Vec<Box<ClassDeclarator>> declarators)
        : Declaration(CLASS_DECL, loc),
        specifiers(std::move(specifiers)),
        declarators(std::move(declarators)) {}

    Vec<Box<DeclarationSpecifier>> specifiers;
    Vec<Box<ClassDeclarator>> declarators;

    void accept(ASTVisitor& visitor) override;
};

/*
An abstract class for a type specifier, usually for a primitive type,
or some user-defined compound type.
*/
class TypeSpecifier : public DeclarationSpecifier {
public:
    TypeSpecifier(NodeKind kind, Location loc)
    : DeclarationSpecifier(kind, loc) {}

    virtual void accept(ASTVisitor& visitor) = 0;
};

enum ClassOrUnion {
    CLASS,
    UNION,
};

class ClassSpecifier : public TypeSpecifier {
public:
    ClassSpecifier(Location loc,
                   std::optional<std::string> name,
                   std::optional<Vec<std::string>> parents,
                   std::optional<Vec<Box<ClassDeclaration>>> declarations)
        : TypeSpecifier(CLASS_SPEC, loc),
        name(std::move(name)),
        parents(std::move(parents)),
        declarations(std::move(declarations)) {}

    std::optional<std::string> name;
    // Identifiers of parent classes.
    std::optional<Vec<std::string>> parents;
    // Declarations of members.
    std::optional<Vec<Box<ClassDeclaration>>> declarations;

    void accept(ASTVisitor& visitor) override;
};

class UnionSpecifier : public TypeSpecifier {
public:
    UnionSpecifier(Location loc,
                   std::optional<std::string> name,
                   std::optional<Vec<Box<ClassDeclaration>>> declarations)
        : TypeSpecifier(UNION_SPEC, loc),
        name(std::move(name)),
        declarations(std::move(declarations)) {}

    std::optional<std::string> name;

    std::optional<Vec<Box<ClassDeclaration>>> declarations;

    void accept(ASTVisitor& visitor) override;
};

/*
A declaration of an enumerator within an enum.
*/
class Enumerator : public ASTNode {
public:
    Enumerator(Location loc,
               std::string name, std::optional<Box<ConstExpression>> value)
        : ASTNode(ENUMERATOR, loc),
        name(std::move(name)),
        value(std::move(value)) {}

    std::string name;
    std::optional<Box<ConstExpression>> value;

    void accept(ASTVisitor& visitor) override;
};


/*
A node denoting an enum and its contained variants.
*/
class EnumSpecifier : public TypeSpecifier {
public:
    EnumSpecifier(Location loc,
                  std::optional<std::string> name,
                  std::optional<Vec<Box<Enumerator>>> enumerators)
        : TypeSpecifier(ENUM_SPEC, loc),
        name(std::move(name)),
        enumerators(std::move(enumerators)) {}

    std::optional<std::string> name;
    std::optional<Vec<Box<Enumerator>>> enumerators;

    void accept(ASTVisitor& visitor) override;
};

class VoidSpecifier : public TypeSpecifier {
public:
    VoidSpecifier(Location loc) : TypeSpecifier(VOID_SPEC, loc) {}

    void accept(ASTVisitor& visitor) override;
};

class PrimitiveSpecifier : public TypeSpecifier {
public:
    enum PrimKind {
        U8,
        U16,
        U32,
        U64,
        I8,
        I16,
        I32,
        I64,
        F64,
        BOOL
    };

    PrimitiveSpecifier(Location loc, PrimKind pkind)
    : TypeSpecifier(PRIM_SPEC, loc), pkind(pkind) {}

    PrimKind pkind;

    void accept(ASTVisitor& visitor) override;
};

//* STATEMENTS

// The abstract Statement class that all statements inherit from.
class Statement : public ProgramItem {
public:
    Statement(NodeKind kind, Location loc) : ProgramItem(kind, loc) {}
    ~Statement() = default;

    void accept(ASTVisitor& visitor) = 0;
};

// A block of mixed declarations and statements, surrounded by braces.
class CompoundStatement : public Statement {
public:
    CompoundStatement(Location loc,
                      Vec<Box<ProgramItem>> items)
        : Statement(COMP_STMT, loc),
        items(std::move(items)) {}

    Vec<Box<ProgramItem>> items;

    void accept(ASTVisitor& visitor) override;
};

class ExpressionStatement : public Statement {
public:
    ExpressionStatement(Location loc,
                        std::optional<Box<Expression>> expression)
        : Statement(EXPR_STMT, loc),
        expression(std::move(expression)) {}

    std::optional<Box<Expression>> expression;

    void accept(ASTVisitor& visitor) override;
};

class CaseStatement : public Statement {
public:
    CaseStatement(Location loc,
                  Box<ConstExpression> case_expr,
                  Box<Statement> statement)
        : Statement(CASE_STMT, loc),
        case_expr(std::move(case_expr)),
        statement(std::move(statement)) {}

    Box<ConstExpression> case_expr;
    Box<Statement> statement;

    void accept(ASTVisitor& visitor) override;
};

class CaseRangeStatement : public Statement {
public:
    CaseRangeStatement(Location loc,
                       Box<ConstExpression> range_start,
                       Box<ConstExpression> range_end,
                       Box<Statement> statement)
        : Statement(CASE_RG_STMT, loc),
        range_start(std::move(range_start)),
        range_end(std::move(range_end)),
        statement(std::move(statement)) {}

    Box<ConstExpression> range_start;
    Box<ConstExpression> range_end;
    Box<Statement> statement;

    void accept(ASTVisitor& visitor) override;
};

class DefaultStatement : public Statement {
public:
    DefaultStatement(Location loc,
                     Box<Statement> statement)
        : Statement(DEF_STMT, loc),
        statement(std::move(statement)) {}

    Box<Statement> statement;

    void accept(ASTVisitor& visitor) override;
};

class LabeledStatement : public Statement {
public:
    LabeledStatement(Location loc,
                     std::string label,
                     Box<Statement> statement)
        : Statement(LABEL_STMT, loc),
        label(label), 
        statement(std::move(statement)) {}

    std::string label;
    Box<Statement> statement;

    void accept(ASTVisitor& visitor) override;
};


class PrintStatement : public Statement {
public:
    PrintStatement(Location loc,
                   std::string format_string,
                   Vec<Box<Expression>> arguments)
        : Statement(PRINT_STMT, loc),
        format_string(std::move(format_string)),
        arguments(std::move(arguments)) {}

    std::string format_string;
    Vec<Box<Expression>> arguments;

    void accept(ASTVisitor& visitor) override;
};

class IfStatement : public Statement {
public:
    IfStatement(Location loc,
                Box<Expression> condition,
                Box<Statement> then_branch,
                std::optional<Box<Statement>> else_branch)
        : Statement(IF_STMT, loc),
        condition(std::move(condition)),
        then_branch(std::move(then_branch)),
        else_branch(std::move(else_branch)) {}

    Box<Expression> condition;
    Box<Statement> then_branch;
    std::optional<Box<Statement>> else_branch;

    void accept(ASTVisitor& visitor) override;
};

class SwitchStatement : public Statement {
public:
    SwitchStatement(Location loc,
                    Box<Expression> condition, 
                    Box<Statement> body)
        : Statement(SWITCH_STMT, loc),
        condition(std::move(condition)), 
        body(std::move(body)) {}

    Box<Expression> condition;
    Box<Statement> body;

    void accept(ASTVisitor& visitor) override;
};

class WhileStatement : public Statement {
public:
    WhileStatement(Location loc,
                   Box<Expression> condition, 
                   Box<Statement> body)
        : Statement(WHILE_STMT, loc),
        condition(std::move(condition)), 
        body(std::move(body)) {}

    Box<Expression> condition;
    Box<Statement> body;

    void accept(ASTVisitor& visitor) override;
};

class DoWhileStatement : public Statement {
public:
    DoWhileStatement(Location loc,
                     Box<Statement> body, 
                     Box<Expression> condition)
        : Statement(DO_WHILE_STMT, loc),
        body(std::move(body)), 
        condition(std::move(condition)) {}

    Box<Statement> body;
    Box<Expression> condition;

    void accept(ASTVisitor& visitor) override;
};

class ForStatement : public Statement {
public:
    using ForInit = std::variant<Box<Expression>, Box<VariableDeclaration>>;

    ForStatement(Location loc,
                 std::optional<ForInit> init,
                 std::optional<Box<Expression>> condition,
                 std::optional<Box<Expression>> increment, 
                 Box<Statement> body)
        : Statement(FOR_STMT, loc),
        init(std::move(init)),
        condition(std::move(condition)),
        increment(std::move(increment)), 
        body(std::move(body)) {}

    std::optional<ForInit> init;
    std::optional<Box<Expression>> condition;
    std::optional<Box<Expression>> increment;
    Box<Statement> body;

    void accept(ASTVisitor& visitor) override;
};

class JumpStatement : public Statement {
public:
    JumpStatement(NodeKind kind, Location loc) : Statement(kind, loc) {}

    virtual void accept(ASTVisitor& visitor) = 0;
};

class GotoStatement : public JumpStatement {
public:
    GotoStatement(Location loc, std::string target_label) 
        : JumpStatement(GOTO_STMT, loc),
        target_label(target_label) {}

    std::string target_label;

    void accept(ASTVisitor& visitor) override;
};

class BreakStatement : public JumpStatement {
public:
    BreakStatement(Location loc) : JumpStatement(BREAK_STMT, loc) {}

    void accept(ASTVisitor& visitor) override;
};

class ReturnStatement : public JumpStatement {
public:
    ReturnStatement(Location loc,
                    std::optional<Box<Expression>> return_value)
        : JumpStatement(RET_STMT, loc),
        return_value(std::move(return_value)) {}

    std::optional<Box<Expression>> return_value;

    void accept(ASTVisitor& visitor) override;
};

class TypeName : public ASTNode {
public:
    TypeName(Location loc,
             Vec<Box<DeclarationSpecifier>> specifiers,
             std::optional<Box<Declarator>> declarator)
        : ASTNode(TYPE_NAME, loc),
        specifiers(std::move(specifiers)), 
        declarator(std::move(declarator)) {
    }

    Vec<Box<DeclarationSpecifier>> specifiers;
    std::optional<Box<Declarator>> declarator;

    void accept(ASTVisitor& visitor) override;
};

//* EXPRESSIONS

class BinaryExpression : public Expression {
public:
    BinaryExpression(
        Location loc,
        Box<Expression> left,
        Box<Expression> right,
        tokens::BinaryOp op
    )
        : Expression(BIN_EXPR, loc),
        left(std::move(left)), 
        right(std::move(right)), 
        op(op) {}

    Box<Expression> left;
    Box<Expression> right;
    tokens::BinaryOp op;

    void accept(ASTVisitor& visitor) override;

    exec::Value accept(exec::Evaluator& ev) override;
};

class CastExpression : public Expression {
public:
    CastExpression(Location loc,
                   Box<Expression> inner,
                   Box<TypeName> type_name)
        : Expression(CAST_EXPR, loc),
        inner(std::move(inner)),
        type_name(std::move(type_name)) {}

    Box<Expression> inner;
    Box<TypeName> type_name;

    void accept(ASTVisitor& visitor) override;

    exec::Value accept(exec::Evaluator& ev) override;
};

class UnaryExpression : public Expression {
public:
    UnaryExpression(Location loc,
                    Box<Expression> operand, 
                    tokens::UnaryOp op)
        : Expression(UN_EXPR, loc),
        operand(std::move(operand)),
        op(op) {}

    Box<Expression> operand;
    tokens::UnaryOp op;

    void accept(ASTVisitor& visitor) override;

    exec::Value accept(exec::Evaluator& ev) override;
};

class AssignmentExpression : public Expression {
public:
    AssignmentExpression(Location loc,
                         Box<Expression> left, 
                         Box<Expression> right,
                         tokens::AssignOp op)
        : Expression(ASSGN_EXPR, loc),
        left(std::move(left)), 
        right(std::move(right)),
        op(op) {}

    Box<Expression> left;
    Box<Expression> right;
    tokens::AssignOp op;

    void accept(ASTVisitor& visitor) override;

    exec::Value accept(exec::Evaluator& ev) override;
};

class ConditionalExpression : public Expression {
public:
    ConditionalExpression(Location loc,
                          Box<Expression> condition, 
                          Box<Expression> true_expr,
                          Box<Expression> false_expr)
        : Expression(COND_EXPR, loc),
        condition(std::move(condition)), 
        true_expr(std::move(true_expr)),
        false_expr(std::move(false_expr)) {}

    Box<Expression> condition;
    Box<Expression> true_expr;
    Box<Expression> false_expr;

    void accept(ASTVisitor& visitor) override;

    exec::Value accept(exec::Evaluator& ev) override;
};

class IdentifierExpression : public Expression {
public:
    IdentifierExpression(Location loc, std::string name) 
        : Expression(IDENT_EXPR, loc),
        name(std::move(name)) {}

    std::string name;

    void accept(ASTVisitor& visitor) override;

    exec::Value accept(exec::Evaluator& ev) override;
};

class LiteralExpression : public Expression {
public:
    enum LiteralKind { INT, FLOAT, CHAR, BOOL };

    union Value {
        int i_val;
        double f_val;
        char c_val;
        bool b_val;
    };

    LiteralExpression(Location loc, 
                      LiteralKind kind, 
                      Value value)
        : Expression(LIT_EXPR, loc),
        kind(kind), 
        value(value) {}

    LiteralKind kind;
    Value value;

    void accept(ASTVisitor& visitor) override;

    exec::Value accept(exec::Evaluator& ev) override;
};

class StringExpression : public Expression {
public:
    StringExpression(Location loc,
                     std::string value)
        : Expression(STR_EXPR, loc),
        value(value) {}

    std::string value;

    void accept(ASTVisitor& visitor) override;

    exec::Value accept(exec::Evaluator& ev) override;
};

class CallExpression : public Expression {
public:
    CallExpression(Location loc,
                   Box<Expression> callee, 
                   Vec<Box<Expression>> arguments)
        : Expression(CALL_EXPR, loc),
        callee(std::move(callee)),
        arguments(std::move(arguments)) {}

    Box<Expression> callee;
    Vec<Box<Expression>> arguments;

    void accept(ASTVisitor& visitor) override;

    exec::Value accept(exec::Evaluator& ev) override;
};

class MemberAccessExpression : public Expression {
public:
    MemberAccessExpression(Location loc,
                           Box<Expression> object,
                           std::string member,
                           bool is_arrow)
        : Expression(ACCESS_EXPR, loc),
        object(std::move(object)), 
        member(std::move(member)),
        is_arrow(is_arrow) {}

    Box<Expression> object;
    std::string member;
    bool is_arrow;

    void accept(ASTVisitor& visitor) override;

    exec::Value accept(exec::Evaluator& ev) override;

};

class ArraySubscriptExpression : public Expression {
public:
    ArraySubscriptExpression(Location loc,
                             Box<Expression> array, 
                             Box<Expression> index)
        : Expression(SUBSCR_EXPR, loc),
        array(std::move(array)), 
        index(std::move(index)) {}

    Box<Expression> array;
    Box<Expression> index;

    void accept(ASTVisitor& visitor) override;

    exec::Value accept(exec::Evaluator& ev) override;

};

class PostfixExpression : public Expression {
public:
    PostfixExpression(Location loc,
                      Box<Expression> operand,
                      tokens::PostfixOp op)
        : Expression(POSTF_EXPR, loc),
        operand(std::move(operand)),
        op(op) {}

    Box<Expression> operand;
    tokens::PostfixOp op;

    void accept(ASTVisitor& visitor) override;

    exec::Value accept(exec::Evaluator& ev) override;

};

class SizeofExpression : public Expression {
public:

    SizeofExpression(Location loc, Box<Expression> expr)
    : Expression(SIZEOF_EXPR, loc), operand(std::move(expr)) {}

    SizeofExpression(Location loc, Box<TypeName> type)
    : Expression(SIZEOF_EXPR, loc), operand(std::move(type)) {}

    std::variant<Box<Expression>, Box<TypeName>> operand;

    void accept(ASTVisitor& visitor) override;

    exec::Value accept(exec::Evaluator& ev) override;

};

class Function : public ProgramItem {
public:
    Function(Location loc, 
             Vec<Box<DeclarationSpecifier>> decl_spec_list,
             Box<Declarator> declarator, 
             Box<CompoundStatement> body)
        : ProgramItem(FUNC, loc),
        decl_spec_list(std::move(decl_spec_list)),
        declarator(std::move(declarator)), body(std::move(body)) {}

    Function(Location loc,
             Box<Declarator> declarator,
             Box<CompoundStatement> body)
        : ProgramItem(FUNC, loc),
        declarator(std::move(declarator)), 
        body(std::move(body)) {
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
    Box<CompoundStatement> body;

    void accept(ASTVisitor& visitor) override;
};

/*
The toplevel Program class.
*/
class Program : public ASTNode {
public:
    Program(std::string *filename) : ASTNode(PROG, Location(filename))  {}
    ~Program() = default;

    // Program items.
    std::vector<std::unique_ptr<ProgramItem>> items;

    void accept(ASTVisitor& visitor) override;

    // Add a new program item.
    void add_item(std::unique_ptr<ProgramItem> item);
};


} // namespace ecc::ast

#endif // ECC_AST_H