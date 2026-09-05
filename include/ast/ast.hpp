#pragma once

#ifndef ECC_AST_H
#define ECC_AST_H

#include <utility>
#include <variant>

#include "abstract/visitor.hpp"
#include "allocator/alloc.hpp"
#include "allocator/chunk.hpp"
#include "ast/visitor.hpp"
#include "ds/arenavec.hpp"
#include "location.hpp"
#include "tokens.hpp"
#include "util.hpp"

using namespace ecc;
using namespace ecc::util;
using namespace ecc::location;

/* Class definitions of AST nodes and subclasses. */
namespace ecc::ast {

template <typename DerivedT, typename BaseT>
using ASTVisitable = Visitable<DerivedT, BaseT, ASTVisitor>;

// The abstract class representing an AST node.
//
// Each AST node (binary, unary expr, statement, etc.) defines its own subclass
// that inherits from this main superclass.
class ASTNode : public NoCopy {
public:
    Location loc;

    enum class NodeKind : uint8_t {
        ATTR,
        ATTR_ARG,
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
        TYPE_IDENT,
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
        CONT_STMT,
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
        REINT_EXPR,
        SUBSCR_EXPR,
        POSTF_EXPR,
        SIZEOF_EXPR,
        FUNC,
        PROG,
    };

    NodeKind kind;

    ASTNode(NodeKind kind, Location loc) : loc(loc), kind(kind) {}
    virtual ~ASTNode() = default;

    // Accept an AST visitor.
    virtual void accept(ASTVisitor& visitor) = 0;
};

/*
A single `name` or `name = "value"` entry inside an attribute (e.g. the `packed` in
`#[packed]`, or the `deprecated = "reason"` in `#[deprecated = "reason"]`).
*/
class AttributeArg : public ASTVisitable<AttributeArg, ASTNode> {
public:
    AttributeArg(Location loc, std::string name, Optional<std::string> value)
        : ASTVisitable<AttributeArg, ASTNode>(NodeKind::ATTR_ARG, loc), name(std::move(name)),
          value(std::move(value)) {}

    std::string name;
    Optional<std::string> value;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::ATTR_ARG; }
};

/*
A single `#[arg, arg = "value", ...]` attribute attached to a top-level ProgramItem.
*/
class Attribute : public ASTVisitable<Attribute, ASTNode> {
public:
    Attribute(Location loc, ds::ArenaVec<Chunk<AttributeArg>> args)
        : ASTVisitable<Attribute, ASTNode>(NodeKind::ATTR, loc), args(std::move(args)) {}

    ds::ArenaVec<Chunk<AttributeArg>> args;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::ATTR; }
};

//* PROGRAM ITEMS

/*
Abstract class denoting a program item: declaration, statement, or function definition.
*/
class ProgramItem : public ASTNode {
public:
    ProgramItem(NodeKind kind, Location loc) : ASTNode(kind, loc) {}

    // Attributes attached to this item (e.g. `#[packed]`). Only populated for the
    // Function/Declaration alternatives of `program_item`; empty otherwise.
    ds::ArenaVec<Chunk<Attribute>> attributes;

    static bool classof(const ASTNode *node) {
        switch (node->kind) {
        case NodeKind::PARAM_DECL:
        case NodeKind::TYPE_DECL:
        case NodeKind::VAR_DECL:
        case NodeKind::CLASS_DECL:
        case NodeKind::COMP_STMT:
        case NodeKind::EXPR_STMT:
        case NodeKind::CASE_STMT:
        case NodeKind::CASE_RG_STMT:
        case NodeKind::DEF_STMT:
        case NodeKind::LABEL_STMT:
        case NodeKind::PRINT_STMT:
        case NodeKind::IF_STMT:
        case NodeKind::SWITCH_STMT:
        case NodeKind::WHILE_STMT:
        case NodeKind::DO_WHILE_STMT:
        case NodeKind::FOR_STMT:
        case NodeKind::GOTO_STMT:
        case NodeKind::BREAK_STMT:
        case NodeKind::CONT_STMT:
        case NodeKind::RET_STMT:
        case NodeKind::FUNC:
            return true;
        default:
            return false;
        }
    }
};

// The abstract Expression class that all expressions inherit from.
class Expression : public ASTNode {
public:
    Expression(NodeKind kind, Location loc) : ASTNode(kind, loc) {}

    static bool classof(const ASTNode *node) {
        switch (node->kind) {
        case NodeKind::CONST_EXPR:
        case NodeKind::BIN_EXPR:
        case NodeKind::CAST_EXPR:
        case NodeKind::UN_EXPR:
        case NodeKind::ASSGN_EXPR:
        case NodeKind::COND_EXPR:
        case NodeKind::IDENT_EXPR:
        case NodeKind::LIT_EXPR:
        case NodeKind::STR_EXPR:
        case NodeKind::CALL_EXPR:
        case NodeKind::ACCESS_EXPR:
        case NodeKind::REINT_EXPR:
        case NodeKind::SUBSCR_EXPR:
        case NodeKind::POSTF_EXPR:
        case NodeKind::SIZEOF_EXPR:
            return true;
        default:
            return false;
        }
    }
};

/*
A wrapper to indicate that the contained expression must be computable at compile time.

Any Expression wrapped in a ConstExpression can be treated as if it is computable at
compile time, and any expression that is not cannot be.
*/
class ConstExpression : public ASTVisitable<ConstExpression, Expression> {
public:
    ConstExpression(Chunk<Expression> expr)
        : ASTVisitable<ConstExpression, Expression>(NodeKind::CONST_EXPR, expr->loc),
          inner(std::move(expr)) {}

    Chunk<Expression> inner;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::CONST_EXPR; }
};

//* DECLARATIONS

/*
The Declaration abstract class that all declarations inherit from.
*/
class Declaration : public ProgramItem {
public:
    Declaration(NodeKind kind, Location loc) : ProgramItem(kind, loc) {}

    static bool classof(const ASTNode *node) {
        switch (node->kind) {
        case NodeKind::PARAM_DECL:
        case NodeKind::TYPE_DECL:
        case NodeKind::VAR_DECL:
        case NodeKind::CLASS_DECL:
            return true;
        default:
            return false;
        }
    }
};

/*
Abstract class denoting a storage class or type specifier, or a type qualifier.
*/
class DeclarationSpecifier : public ASTNode {
public:
    DeclarationSpecifier(NodeKind kind, Location loc) : ASTNode(kind, loc) {}

    static bool classof(const ASTNode *node) {
        switch (node->kind) {
        case NodeKind::TYPE_QUAL:
        case NodeKind::STORAGE_SPEC:
        case NodeKind::CLASS_SPEC:
        case NodeKind::UNION_SPEC:
        case NodeKind::ENUM_SPEC:
        case NodeKind::TYPE_IDENT:
        case NodeKind::VOID_SPEC:
        case NodeKind::PRIM_SPEC:
            return true;
        default:
            return false;
        }
    }
};

class TypeQualifier : public ASTVisitable<TypeQualifier, DeclarationSpecifier> {
public:
    enum QualType : uint8_t { CONST };

    TypeQualifier(Location loc, QualType qual)
        : ASTVisitable<TypeQualifier, DeclarationSpecifier>(NodeKind::TYPE_QUAL, loc), qual(qual) {}

    QualType qual;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::TYPE_QUAL; }
};

// Storage class specifiers (public, static, extern).
class StorageClassSpecifier : public ASTVisitable<StorageClassSpecifier, DeclarationSpecifier> {
public:
    enum SpecType : uint8_t { PUBLIC, STATIC, EXTERN, EXTERNC };

    StorageClassSpecifier(Location loc, SpecType type)
        : ASTVisitable<StorageClassSpecifier, DeclarationSpecifier>(NodeKind::STORAGE_SPEC, loc),
          type(type) {}

    SpecType type;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::STORAGE_SPEC; }
};

class Pointer : public ASTVisitable<Pointer, ASTNode> {
public:
    Pointer(
        Location loc, ds::ArenaVec<Chunk<TypeQualifier>> qualifiers,
        Optional<Chunk<Pointer>> nested)
        : ASTVisitable<Pointer, ASTNode>(NodeKind::POINTER, loc), qualifiers(std::move(qualifiers)),
          nested(std::move(nested)) {}

    ds::ArenaVec<Chunk<TypeQualifier>> qualifiers;
    Optional<Chunk<Pointer>> nested;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::POINTER; }
};

/*
A non-pointer declarator abstract class.
*/
class DirectDeclarator : public ASTNode {
public:
    DirectDeclarator(NodeKind kind, Location loc) : ASTNode(kind, loc) {}

    static bool classof(const ASTNode *node) {
        switch (node->kind) {
        case NodeKind::IDENT_DECLTR:
        case NodeKind::PAREN_DECLTR:
        case NodeKind::ARR_DECLTR:
        case NodeKind::FUNC_DECLTR:
            return true;
        default:
            return false;
        }
    }
};

/*
An initializer for a variable.

Compound-type variables use the ds::ArenaVec<Chunk<Initializer>> variant of `initializer`,
whereas primitive type variables use the Chunk<Expression> variant.
*/
class Initializer : public ASTVisitable<Initializer, ASTNode> {
public:
    struct Member {
        std::string member;
        Chunk<Initializer> initializer;
    };

    struct Index {
        Chunk<ConstExpression> idx;
        Chunk<Initializer> initializer;
    };

    using InitVariant = std::variant<
        Chunk<Expression>, Chunk<Member>, Chunk<Index>, ds::ArenaVec<Chunk<Initializer>>>;

    Initializer(Location loc, Chunk<Expression> expr)
        : ASTVisitable<Initializer, ASTNode>(NodeKind::INITIALIZER, loc),
          initializer(std::move(expr)) {}

    Initializer(Location loc, std::string mem, Chunk<Initializer> init)
        : ASTVisitable<Initializer, ASTNode>(NodeKind::INITIALIZER, loc),
          initializer(make_chunk<Member>(std::move(mem), std::move(init))) {}

    Initializer(Location loc, Chunk<ConstExpression> idx, Chunk<Initializer> init)
        : ASTVisitable<Initializer, ASTNode>(NodeKind::INITIALIZER, loc),
          initializer(make_chunk<Index>(std::move(idx), std::move(init))) {}

    Initializer(Location loc, ds::ArenaVec<Chunk<Initializer>> list)
        : ASTVisitable<Initializer, ASTNode>(NodeKind::INITIALIZER, loc),
          initializer(std::move(list)) {}

    InitVariant initializer;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::INITIALIZER; }
};

/*
A general declarator containing a DirectDeclarator and an optional Pointer.
*/
class Declarator : public ASTVisitable<Declarator, ASTNode> {
public:
    Declarator(
        Location loc, Optional<Chunk<Pointer>> pointer, Optional<Chunk<DirectDeclarator>> direct)
        : ASTVisitable<Declarator, ASTNode>(NodeKind::DECLARATOR, loc), pointer(std::move(pointer)),
          direct(std::move(direct)) {}

    Optional<Chunk<Pointer>> pointer;
    Optional<Chunk<DirectDeclarator>> direct;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::DECLARATOR; }
};

/*
A declarator creating one or more new variables, with optional initializers.
*/
class InitDeclarator : public ASTVisitable<InitDeclarator, ASTNode> {
public:
    InitDeclarator(
        Location loc, Chunk<Declarator> declarator, Optional<Chunk<Initializer>> initializer)
        : ASTVisitable<InitDeclarator, ASTNode>(NodeKind::INIT_DECLTR, loc),
          declarator(std::move(declarator)), initializer(std::move(initializer)) {}

    Chunk<Declarator> declarator;
    Optional<Chunk<Initializer>> initializer;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::INIT_DECLTR; }
};

/*
A declaration of a single function parameter.
*/
class ParameterDeclaration : public ASTVisitable<ParameterDeclaration, Declaration> {
public:
    ParameterDeclaration(
        Location loc, ds::ArenaVec<Chunk<DeclarationSpecifier>> specifiers,
        Optional<Chunk<Declarator>> declarator, Optional<Chunk<ConstExpression>> default_value)
        : ASTVisitable<ParameterDeclaration, Declaration>(NodeKind::PARAM_DECL, loc),
          specifiers(std::move(specifiers)), declarator(std::move(declarator)),
          default_value(std::move(default_value)) {}

    ds::ArenaVec<Chunk<DeclarationSpecifier>> specifiers;
    Optional<Chunk<Declarator>> declarator;
    Optional<Chunk<ConstExpression>> default_value;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::PARAM_DECL; }
};

/*
A Declaration of a type (i.e. a declaration of only specifiers, no InitDeclarators).
*/
class TypeDeclaration : public ASTVisitable<TypeDeclaration, Declaration> {
public:
    TypeDeclaration(Location loc, ds::ArenaVec<Chunk<DeclarationSpecifier>> specifiers)
        : ASTVisitable<TypeDeclaration, Declaration>(NodeKind::TYPE_DECL, loc),
          specifiers(std::move(specifiers)) {}

    ds::ArenaVec<Chunk<DeclarationSpecifier>> specifiers;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::TYPE_DECL; }
};

/*
A variable declaration (e.g. `U32 x = 5;`, `class Flags {U16 x; bool y} = {3, true}`).
*/
class VariableDeclaration : public ASTVisitable<VariableDeclaration, Declaration> {
public:
    VariableDeclaration(
        Location loc, ds::ArenaVec<Chunk<DeclarationSpecifier>> specifiers,
        ds::ArenaVec<Chunk<InitDeclarator>> declarators)
        : ASTVisitable<VariableDeclaration, Declaration>(NodeKind::VAR_DECL, loc),
          specifiers(std::move(specifiers)), declarators(std::move(declarators)) {}

    ds::ArenaVec<Chunk<DeclarationSpecifier>> specifiers;
    ds::ArenaVec<Chunk<InitDeclarator>> declarators;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::VAR_DECL; }
};

class IdentifierDeclarator : public ASTVisitable<IdentifierDeclarator, DirectDeclarator> {
public:
    std::string name;

    IdentifierDeclarator(Location loc, std::string n)
        : ASTVisitable<IdentifierDeclarator, DirectDeclarator>(NodeKind::IDENT_DECLTR, loc),
          name(std::move(n)) {}

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::IDENT_DECLTR; }
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
class ParenDeclarator : public ASTVisitable<ParenDeclarator, DirectDeclarator> {
public:
    ParenDeclarator(Location loc, Chunk<Declarator> decl)
        : ASTVisitable<ParenDeclarator, DirectDeclarator>(NodeKind::PAREN_DECLTR, loc),
          inner(std::move(decl)) {}

    Chunk<Declarator> inner;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::PAREN_DECLTR; }
};

class ArrayDeclarator : public ASTVisitable<ArrayDeclarator, DirectDeclarator> {
public:
    ArrayDeclarator(
        Location loc, Chunk<DirectDeclarator> base, Optional<Chunk<ConstExpression>> size)
        : ASTVisitable<ArrayDeclarator, DirectDeclarator>(NodeKind::ARR_DECLTR, loc),
          base(std::move(base)), size(std::move(size)) {}

    Chunk<DirectDeclarator> base;
    Optional<Chunk<ConstExpression>> size;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::ARR_DECLTR; }
};

class FunctionDeclarator : public ASTVisitable<FunctionDeclarator, DirectDeclarator> {
public:
    FunctionDeclarator(
        Location loc, Chunk<DirectDeclarator> base,
        ds::ArenaVec<Chunk<ParameterDeclaration>> params, bool is_variadic)
        : ASTVisitable<FunctionDeclarator, DirectDeclarator>(NodeKind::FUNC_DECLTR, loc),
          base(std::move(base)), parameters(std::move(params)), is_variadic(is_variadic) {}

    Chunk<DirectDeclarator> base;
    ds::ArenaVec<Chunk<ParameterDeclaration>> parameters;
    bool is_variadic;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::FUNC_DECLTR; }
};

/*
A declarator representing a member of a class.
*/
class ClassDeclarator : public ASTVisitable<ClassDeclarator, ASTNode> {
public:
    ClassDeclarator(
        Location loc, Optional<Chunk<Declarator>> declarator, Optional<Chunk<Expression>> bit_width)
        : ASTVisitable<ClassDeclarator, ASTNode>(NodeKind::CLASS_DECLTR, loc),
          declarator(std::move(declarator)), bit_width(std::move(bit_width)) {}

    Optional<Chunk<Declarator>> declarator;
    Optional<Chunk<Expression>> bit_width;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::CLASS_DECLTR; }
};

/*
Declaration of one or more class members. Each declaration contains
one or more ClassDeclarators.
*/
class ClassDeclaration : public ASTVisitable<ClassDeclaration, Declaration> {
public:
    ClassDeclaration(
        Location loc, ds::ArenaVec<Chunk<DeclarationSpecifier>> specifiers,
        ds::ArenaVec<Chunk<ClassDeclarator>> declarators)
        : ASTVisitable<ClassDeclaration, Declaration>(NodeKind::CLASS_DECL, loc),
          specifiers(std::move(specifiers)), declarators(std::move(declarators)) {}

    ds::ArenaVec<Chunk<DeclarationSpecifier>> specifiers;
    ds::ArenaVec<Chunk<ClassDeclarator>> declarators;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::CLASS_DECL; }
};

/*
An abstract class for a type specifier, usually for a primitive type,
or some user-defined compound type.
*/
class TypeSpecifier : public DeclarationSpecifier {
public:
    TypeSpecifier(NodeKind kind, Location loc) : DeclarationSpecifier(kind, loc) {}

    static bool classof(const ASTNode *node) {
        switch (node->kind) {
        case NodeKind::PRIM_SPEC:
        case NodeKind::CLASS_SPEC:
        case NodeKind::UNION_SPEC:
        case NodeKind::ENUM_SPEC:
        case NodeKind::TYPE_IDENT:
        case NodeKind::VOID_SPEC:
            return true;
        default:
            return false;
        }
    }
};

class PrimitiveSpecifier : public ASTVisitable<PrimitiveSpecifier, TypeSpecifier> {
public:
    PrimitiveSpecifier(Location loc, tokens::PrimType pkind)
        : ASTVisitable<PrimitiveSpecifier, TypeSpecifier>(NodeKind::PRIM_SPEC, loc), pkind(pkind) {}

    tokens::PrimType pkind;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::PRIM_SPEC; }
};

class ClassSpecifier : public ASTVisitable<ClassSpecifier, TypeSpecifier> {
public:
    ClassSpecifier(
        Location loc, Optional<std::string> name, Optional<ds::ArenaVec<std::string>> parents,
        Optional<ds::ArenaVec<Chunk<ClassDeclaration>>> declarations)
        : ASTVisitable<ClassSpecifier, TypeSpecifier>(NodeKind::CLASS_SPEC, loc),
          name(std::move(name)), parents(std::move(parents)),
          declarations(std::move(declarations)) {}

    Optional<std::string> name;
    // Identifiers of parent classes.
    Optional<ds::ArenaVec<std::string>> parents;
    // Declarations of members.
    Optional<ds::ArenaVec<Chunk<ClassDeclaration>>> declarations;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::CLASS_SPEC; }
};

class UnionSpecifier : public ASTVisitable<UnionSpecifier, TypeSpecifier> {
public:
    UnionSpecifier(
        Location loc, Optional<std::string> name, Optional<tokens::PrimType> type_rep,
        Optional<ds::ArenaVec<Chunk<ClassDeclaration>>> declarations)
        : ASTVisitable<UnionSpecifier, TypeSpecifier>(NodeKind::UNION_SPEC, loc),
          name(std::move(name)), type_rep(type_rep), declarations(std::move(declarations)) {}

    Optional<std::string> name;

    Optional<tokens::PrimType> type_rep;

    Optional<ds::ArenaVec<Chunk<ClassDeclaration>>> declarations;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::UNION_SPEC; }
};

/*
A declaration of an enumerator within an enum.
*/
class Enumerator : public ASTVisitable<Enumerator, ASTNode> {
public:
    Enumerator(Location loc, std::string name, Optional<Chunk<ConstExpression>> value)
        : ASTVisitable<Enumerator, ASTNode>(NodeKind::ENUMERATOR, loc), name(std::move(name)),
          value(std::move(value)) {}

    std::string name;
    Optional<Chunk<ConstExpression>> value;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::ENUMERATOR; }
};

/*
A node denoting an enum and its contained variants.
*/
class EnumSpecifier : public ASTVisitable<EnumSpecifier, TypeSpecifier> {
public:
    EnumSpecifier(
        Location loc, Optional<std::string> name,
        Optional<ds::ArenaVec<Chunk<Enumerator>>> enumerators)
        : ASTVisitable<EnumSpecifier, TypeSpecifier>(NodeKind::ENUM_SPEC, loc),
          name(std::move(name)), enumerators(std::move(enumerators)) {}

    EnumSpecifier(
        Location loc, Optional<std::string> name,
        Optional<ds::ArenaVec<Chunk<Enumerator>>> enumerators, tokens::PrimType underlying)
        : ASTVisitable<EnumSpecifier, TypeSpecifier>(NodeKind::ENUM_SPEC, loc),
          name(std::move(name)), enumerators(std::move(enumerators)), underlying(underlying) {}

    Optional<std::string> name;
    Optional<ds::ArenaVec<Chunk<Enumerator>>> enumerators;

    /**
    The underlying type of the enum, if applicable.
    */
    Optional<tokens::PrimType> underlying;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::ENUM_SPEC; }
};

class TypeIdentifier : public ASTVisitable<TypeIdentifier, TypeSpecifier> {
public:
    TypeIdentifier(Location loc, std::string ident)
        : ASTVisitable<TypeIdentifier, TypeSpecifier>(NodeKind::TYPE_IDENT, loc),
          identifier(std::move(ident)) {}

    std::string identifier;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::TYPE_IDENT; }
};

class VoidSpecifier : public ASTVisitable<VoidSpecifier, TypeSpecifier> {
public:
    VoidSpecifier(Location loc)
        : ASTVisitable<VoidSpecifier, TypeSpecifier>(NodeKind::VOID_SPEC, loc) {}

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::VOID_SPEC; }
};

//* STATEMENTS

// The abstract Statement class that all statements inherit from.
class Statement : public ProgramItem {
public:
    Statement(NodeKind kind, Location loc) : ProgramItem(kind, loc) {}

    static bool classof(const ASTNode *node) {
        switch (node->kind) {
        case NodeKind::COMP_STMT:
        case NodeKind::EXPR_STMT:
        case NodeKind::CASE_STMT:
        case NodeKind::CASE_RG_STMT:
        case NodeKind::DEF_STMT:
        case NodeKind::LABEL_STMT:
        case NodeKind::PRINT_STMT:
        case NodeKind::IF_STMT:
        case NodeKind::SWITCH_STMT:
        case NodeKind::WHILE_STMT:
        case NodeKind::DO_WHILE_STMT:
        case NodeKind::FOR_STMT:
        case NodeKind::GOTO_STMT:
        case NodeKind::BREAK_STMT:
        case NodeKind::CONT_STMT:
        case NodeKind::RET_STMT:
            return true;
        default:
            return false;
        }
    }
};

// A block of mixed declarations and statements, surrounded by braces.
class CompoundStatement : public ASTVisitable<CompoundStatement, Statement> {
public:
    CompoundStatement(Location loc, ds::ArenaVec<Chunk<ProgramItem>> items)
        : ASTVisitable<CompoundStatement, Statement>(NodeKind::COMP_STMT, loc),
          items(std::move(items)) {}

    ds::ArenaVec<Chunk<ProgramItem>> items;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::COMP_STMT; }
};

class ExpressionStatement : public ASTVisitable<ExpressionStatement, Statement> {
public:
    ExpressionStatement(Location loc, Optional<Chunk<Expression>> expression)
        : ASTVisitable<ExpressionStatement, Statement>(NodeKind::EXPR_STMT, loc),
          expression(std::move(expression)) {}

    Optional<Chunk<Expression>> expression;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::EXPR_STMT; }
};

class CaseStatement : public ASTVisitable<CaseStatement, Statement> {
public:
    CaseStatement(Location loc, Chunk<ConstExpression> case_expr, Chunk<Statement> statement)
        : ASTVisitable<CaseStatement, Statement>(NodeKind::CASE_STMT, loc),
          case_expr(std::move(case_expr)), statement(std::move(statement)) {}

    Chunk<ConstExpression> case_expr;
    Chunk<Statement> statement;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::CASE_STMT; }
};

class CaseRangeStatement : public ASTVisitable<CaseRangeStatement, Statement> {
public:
    CaseRangeStatement(
        Location loc, Chunk<ConstExpression> range_start, Chunk<ConstExpression> range_end,
        Chunk<Statement> statement)
        : ASTVisitable<CaseRangeStatement, Statement>(NodeKind::CASE_RG_STMT, loc),
          range_start(std::move(range_start)), range_end(std::move(range_end)),
          statement(std::move(statement)) {}

    Chunk<ConstExpression> range_start;
    Chunk<ConstExpression> range_end;
    Chunk<Statement> statement;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::CASE_RG_STMT; }
};

class DefaultStatement : public ASTVisitable<DefaultStatement, Statement> {
public:
    DefaultStatement(Location loc, Chunk<Statement> statement)
        : ASTVisitable<DefaultStatement, Statement>(NodeKind::DEF_STMT, loc),
          statement(std::move(statement)) {}

    Chunk<Statement> statement;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::DEF_STMT; }
};

class LabeledStatement : public ASTVisitable<LabeledStatement, Statement> {
public:
    LabeledStatement(Location loc, std::string label, Chunk<Statement> statement)
        : ASTVisitable<LabeledStatement, Statement>(NodeKind::LABEL_STMT, loc),
          label(std::move(label)), statement(std::move(statement)) {}

    std::string label;
    Chunk<Statement> statement;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::LABEL_STMT; }
};

class PrintStatement : public ASTVisitable<PrintStatement, Statement> {
public:
    PrintStatement(
        Location loc, std::string format_string, ds::ArenaVec<Chunk<Expression>> arguments)
        : ASTVisitable<PrintStatement, Statement>(NodeKind::PRINT_STMT, loc),
          format_string(std::move(format_string)), arguments(std::move(arguments)) {}

    std::string format_string;
    ds::ArenaVec<Chunk<Expression>> arguments;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::PRINT_STMT; }
};

class IfStatement : public ASTVisitable<IfStatement, Statement> {
public:
    IfStatement(
        Location loc, Chunk<Expression> condition, Chunk<Statement> then_branch,
        Optional<Chunk<Statement>> else_branch)
        : ASTVisitable<IfStatement, Statement>(NodeKind::IF_STMT, loc),
          condition(std::move(condition)), then_branch(std::move(then_branch)),
          else_branch(std::move(else_branch)) {}

    Chunk<Expression> condition;
    Chunk<Statement> then_branch;
    Optional<Chunk<Statement>> else_branch;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::IF_STMT; }
};

class SwitchStatement : public ASTVisitable<SwitchStatement, Statement> {
public:
    SwitchStatement(Location loc, Chunk<Expression> condition, Chunk<Statement> body)
        : ASTVisitable<SwitchStatement, Statement>(NodeKind::SWITCH_STMT, loc),
          condition(std::move(condition)), body(std::move(body)) {}

    Chunk<Expression> condition;
    Chunk<Statement> body;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::SWITCH_STMT; }
};

class WhileStatement : public ASTVisitable<WhileStatement, Statement> {
public:
    WhileStatement(Location loc, Chunk<Expression> condition, Chunk<Statement> body)
        : ASTVisitable<WhileStatement, Statement>(NodeKind::WHILE_STMT, loc),
          condition(std::move(condition)), body(std::move(body)) {}

    Chunk<Expression> condition;
    Chunk<Statement> body;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::WHILE_STMT; }
};

class DoWhileStatement : public ASTVisitable<DoWhileStatement, Statement> {
public:
    DoWhileStatement(Location loc, Chunk<Statement> body, Chunk<Expression> condition)
        : ASTVisitable<DoWhileStatement, Statement>(NodeKind::DO_WHILE_STMT, loc),
          body(std::move(body)), condition(std::move(condition)) {}

    Chunk<Statement> body;
    Chunk<Expression> condition;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::DO_WHILE_STMT; }
};

class ForStatement : public ASTVisitable<ForStatement, Statement> {
public:
    using ForInit = std::variant<Chunk<Expression>, Chunk<VariableDeclaration>>;

    ForStatement(
        Location loc, Optional<ForInit> init, Optional<Chunk<Expression>> condition,
        Optional<Chunk<Expression>> increment, Chunk<Statement> body)
        : ASTVisitable<ForStatement, Statement>(NodeKind::FOR_STMT, loc), init(std::move(init)),
          condition(std::move(condition)), increment(std::move(increment)), body(std::move(body)) {}

    Optional<ForInit> init;
    Optional<Chunk<Expression>> condition;
    Optional<Chunk<Expression>> increment;
    Chunk<Statement> body;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::FOR_STMT; }
};

class JumpStatement : public Statement {
public:
    JumpStatement(NodeKind kind, Location loc) : Statement(kind, loc) {}

    static bool classof(const ASTNode *node) {
        switch (node->kind) {
        case NodeKind::GOTO_STMT:
        case NodeKind::BREAK_STMT:
        case NodeKind::CONT_STMT:
        case NodeKind::RET_STMT:
            return true;
        default:
            return false;
        }
    }
};

class GotoStatement : public ASTVisitable<GotoStatement, JumpStatement> {
public:
    GotoStatement(Location loc, std::string target_label)
        : ASTVisitable<GotoStatement, JumpStatement>(NodeKind::GOTO_STMT, loc),
          target_label(std::move(target_label)) {}

    std::string target_label;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::GOTO_STMT; }
};

class BreakStatement : public ASTVisitable<BreakStatement, JumpStatement> {
public:
    BreakStatement(Location loc)
        : ASTVisitable<BreakStatement, JumpStatement>(NodeKind::BREAK_STMT, loc) {}

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::BREAK_STMT; }
};

class ContinueStatement : public ASTVisitable<ContinueStatement, JumpStatement> {
public:
    ContinueStatement(Location loc)
        : ASTVisitable<ContinueStatement, JumpStatement>(NodeKind::CONT_STMT, loc) {}

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::CONT_STMT; }
};

class ReturnStatement : public ASTVisitable<ReturnStatement, JumpStatement> {
public:
    ReturnStatement(Location loc, Optional<Chunk<Expression>> return_value)
        : ASTVisitable<ReturnStatement, JumpStatement>(NodeKind::RET_STMT, loc),
          return_value(std::move(return_value)) {}

    Optional<Chunk<Expression>> return_value;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::RET_STMT; }
};

class TypeName : public ASTVisitable<TypeName, ASTNode> {
public:
    TypeName(
        Location loc, ds::ArenaVec<Chunk<DeclarationSpecifier>> specifiers,
        Optional<Chunk<Declarator>> declarator)
        : ASTVisitable<TypeName, ASTNode>(NodeKind::TYPE_NAME, loc),
          specifiers(std::move(specifiers)), declarator(std::move(declarator)) {}

    ds::ArenaVec<Chunk<DeclarationSpecifier>> specifiers;
    Optional<Chunk<Declarator>> declarator;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::TYPE_NAME; }
};

//* EXPRESSIONS

class BinaryExpression : public ASTVisitable<BinaryExpression, Expression> {
public:
    BinaryExpression(
        Location loc, Chunk<Expression> left, Chunk<Expression> right, tokens::BinaryOp op)
        : ASTVisitable<BinaryExpression, Expression>(NodeKind::BIN_EXPR, loc),
          left(std::move(left)), right(std::move(right)), op(op) {}

    Chunk<Expression> left;
    Chunk<Expression> right;
    tokens::BinaryOp op;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::BIN_EXPR; }
};

class CastExpression : public ASTVisitable<CastExpression, Expression> {
public:
    CastExpression(Location loc, Chunk<Expression> inner, Chunk<TypeName> type_name)
        : ASTVisitable<CastExpression, Expression>(NodeKind::CAST_EXPR, loc),
          inner(std::move(inner)), type_name(std::move(type_name)) {}

    Chunk<Expression> inner;
    Chunk<TypeName> type_name;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::CAST_EXPR; }
};

class UnaryExpression : public ASTVisitable<UnaryExpression, Expression> {
public:
    UnaryExpression(Location loc, Chunk<Expression> operand, tokens::UnaryOp op)
        : ASTVisitable<UnaryExpression, Expression>(NodeKind::UN_EXPR, loc),
          operand(std::move(operand)), op(op) {}

    Chunk<Expression> operand;
    tokens::UnaryOp op;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::UN_EXPR; }
};

class AssignmentExpression : public ASTVisitable<AssignmentExpression, Expression> {
public:
    AssignmentExpression(
        Location loc, Chunk<Expression> left, Chunk<Expression> right, tokens::AssignOp op)
        : ASTVisitable<AssignmentExpression, Expression>(NodeKind::ASSGN_EXPR, loc),
          left(std::move(left)), right(std::move(right)), op(op) {}

    Chunk<Expression> left;
    Chunk<Expression> right;
    tokens::AssignOp op;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::ASSGN_EXPR; }
};

class ConditionalExpression : public ASTVisitable<ConditionalExpression, Expression> {
public:
    ConditionalExpression(
        Location loc, Chunk<Expression> condition, Chunk<Expression> true_expr,
        Chunk<Expression> false_expr)
        : ASTVisitable<ConditionalExpression, Expression>(NodeKind::COND_EXPR, loc),
          condition(std::move(condition)), true_expr(std::move(true_expr)),
          false_expr(std::move(false_expr)) {}

    Chunk<Expression> condition;
    Chunk<Expression> true_expr;
    Chunk<Expression> false_expr;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::COND_EXPR; }
};

class IdentifierExpression : public ASTVisitable<IdentifierExpression, Expression> {
public:
    IdentifierExpression(Location loc, std::string name)
        : ASTVisitable<IdentifierExpression, Expression>(NodeKind::IDENT_EXPR, loc),
          name(std::move(name)) {}

    std::string name;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::IDENT_EXPR; }
};

class LiteralExpression : public ASTVisitable<LiteralExpression, Expression> {
public:
    enum LiteralKind : uint8_t { INT, FLOAT, CHAR, BOOL };

    union Value {
        uint64_t i_val;
        double f_val;
        char c_val;
        bool b_val;
    };

    LiteralExpression(Location loc, LiteralKind kind, Value value)
        : ASTVisitable<LiteralExpression, Expression>(NodeKind::LIT_EXPR, loc), kind(kind),
          value(value) {}

    LiteralKind kind;
    Value value;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::LIT_EXPR; }
};

class StringExpression : public ASTVisitable<StringExpression, Expression> {
public:
    StringExpression(Location loc, std::string value)
        : ASTVisitable<StringExpression, Expression>(NodeKind::STR_EXPR, loc),
          value(std::move(value)) {}

    std::string value;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::STR_EXPR; }
};

class CallExpression : public ASTVisitable<CallExpression, Expression> {
public:
    CallExpression(
        Location loc, Chunk<Expression> callee, ds::ArenaVec<Chunk<Expression>> arguments)
        : ASTVisitable<CallExpression, Expression>(NodeKind::CALL_EXPR, loc),
          callee(std::move(callee)), arguments(std::move(arguments)) {}

    Chunk<Expression> callee;
    ds::ArenaVec<Chunk<Expression>> arguments;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::CALL_EXPR; }
};

class MemberAccessExpression : public ASTVisitable<MemberAccessExpression, Expression> {
public:
    MemberAccessExpression(
        Location loc, Chunk<Expression> object, std::string member, bool is_arrow)
        : ASTVisitable<MemberAccessExpression, Expression>(NodeKind::ACCESS_EXPR, loc),
          object(std::move(object)), member(std::move(member)), is_arrow(is_arrow) {}

    Chunk<Expression> object;
    std::string member;
    bool is_arrow;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::ACCESS_EXPR; }
};

/**
An expression for reinterpreting a primitive type as an array of smaller primitives.

For example,
```
// declare a U64
U64 i = 69;
// reinterpret the U64 as U32[2], and directly access the second U32.
i.U32[1] = 4;
```
*/
class ReinterpretExpression : public ASTVisitable<ReinterpretExpression, Expression> {
public:
    ReinterpretExpression(
        Location loc, Chunk<Expression> object, tokens::PrimType target, bool is_arrow)
        : ASTVisitable<ReinterpretExpression, Expression>(NodeKind::REINT_EXPR, loc),
          object(std::move(object)), target(target), is_arrow(is_arrow) {}

    Chunk<Expression> object;
    tokens::PrimType target;
    bool is_arrow;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::REINT_EXPR; }
};

class ArraySubscriptExpression : public ASTVisitable<ArraySubscriptExpression, Expression> {
public:
    ArraySubscriptExpression(Location loc, Chunk<Expression> array, Chunk<Expression> index)
        : ASTVisitable<ArraySubscriptExpression, Expression>(NodeKind::SUBSCR_EXPR, loc),
          array(std::move(array)), index(std::move(index)) {}

    Chunk<Expression> array;
    Chunk<Expression> index;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::SUBSCR_EXPR; }
};

class PostfixExpression : public ASTVisitable<PostfixExpression, Expression> {
public:
    PostfixExpression(Location loc, Chunk<Expression> operand, tokens::PostfixOp op)
        : ASTVisitable<PostfixExpression, Expression>(NodeKind::POSTF_EXPR, loc),
          operand(std::move(operand)), op(op) {}

    Chunk<Expression> operand;
    tokens::PostfixOp op;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::POSTF_EXPR; }
};

class SizeofExpression : public ASTVisitable<SizeofExpression, Expression> {
public:
    SizeofExpression(Location loc, Chunk<Expression> expr)
        : ASTVisitable<SizeofExpression, Expression>(NodeKind::SIZEOF_EXPR, loc),
          operand(std::move(expr)) {}

    SizeofExpression(Location loc, Chunk<TypeName> type)
        : ASTVisitable<SizeofExpression, Expression>(NodeKind::SIZEOF_EXPR, loc),
          operand(std::move(type)) {}

    std::variant<Chunk<Expression>, Chunk<TypeName>> operand;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::SIZEOF_EXPR; }
};

class Function : public ASTVisitable<Function, ProgramItem> {
public:
    Function(
        Location loc, ds::ArenaVec<Chunk<DeclarationSpecifier>> decl_spec_list,
        Chunk<Declarator> declarator, Chunk<CompoundStatement> body)
        : ASTVisitable<Function, ProgramItem>(NodeKind::FUNC, loc),
          decl_spec_list(std::move(decl_spec_list)), declarator(std::move(declarator)),
          body(std::move(body)) {}

    Function(Location loc, Chunk<Declarator> declarator, Chunk<CompoundStatement> body)
        : ASTVisitable<Function, ProgramItem>(NodeKind::FUNC, loc),
          declarator(std::move(declarator)), body(std::move(body)) {}

    /*
    Any possible specifiers (e.g. public, int, etc.)
    */
    ds::ArenaVec<Chunk<DeclarationSpecifier>> decl_spec_list;
    /*
    The function name and its parameters.
    Note: If the declarator contains a pointer, the pointer applies to its return type.
    */
    Chunk<Declarator> declarator;
    Chunk<CompoundStatement> body;

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::FUNC; }
};

/*
The toplevel Program class.
*/
class Program : public ASTVisitable<Program, ASTNode> {
public:
    Program(std::string *filename)
        : ASTVisitable<Program, ASTNode>(NodeKind::PROG, Location(filename)) {}

    // Program items.
    ds::ArenaVec<Chunk<ProgramItem>> items;

    // Add a new program item.
    void add_item(Chunk<ProgramItem> item);

    static bool classof(const ASTNode *node) { return node->kind == NodeKind::PROG; }
};

std::string storage_to_string(StorageClassSpecifier::SpecType ty);

std::string qualifier_to_string(TypeQualifier::QualType qual);

} // namespace ecc::ast

#endif // ECC_AST_H
