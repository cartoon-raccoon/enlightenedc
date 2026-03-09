#ifndef ECC_AST_VISITOR_H
#define ECC_AST_VISITOR_H

#include "ast/ast.hpp"

namespace ecc::ast {

// An abstract class defining the interface for an AST visitor.
// An ASTVisitor object visits each node on the AST, operating on
// it as it goes.
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    virtual void visit(Program& node) = 0;
    virtual void visit(Function& node) = 0;

    virtual void visit(TypeDeclaration& node) = 0;
    virtual void visit(VariableDeclaration& node) = 0;
    virtual void visit(ParameterDeclaration& node) = 0;
    virtual void visit(Declarator& node) = 0;
    virtual void visit(ParenDeclarator& node) = 0;
    virtual void visit(ArrayDeclarator& node) = 0;
    virtual void visit(FunctionDeclarator& node) = 0;
    virtual void visit(InitDeclarator& node) = 0;
    virtual void visit(Pointer& node) = 0;
    virtual void visit(ClassDeclarator& node) = 0;
    virtual void visit(ClassDeclaration& node) = 0;
    virtual void visit(Enumerator& node) = 0;
    virtual void visit(StorageClassSpecifier& node) = 0;
    virtual void visit(TypeQualifier& node) = 0;
    virtual void visit(EnumSpecifier& node) = 0;
    virtual void visit(ClassSpecifier& node) = 0;
    virtual void visit(UnionSpecifier& node) = 0;
    virtual void visit(PrimitiveSpecifier& node) = 0;
    virtual void visit(Initializer& node) = 0;
    virtual void visit(TypeName& node) = 0;
    virtual void visit(IdentifierDeclarator& node) = 0;

    virtual void visit(CompoundStatement& node) = 0;
    virtual void visit(ExpressionStatement& node) = 0;
    virtual void visit(CaseDefaultStatement& node) = 0;
    virtual void visit(LabeledStatement& node) = 0;
    virtual void visit(PrintStatement& node) = 0;
    virtual void visit(IfStatement& node) = 0;
    virtual void visit(SwitchStatement& node) = 0;
    virtual void visit(WhileStatement& node) = 0;
    virtual void visit(DoWhileStatement& node) = 0;
    virtual void visit(ForStatement& node) = 0;
    virtual void visit(GotoStatement& node) = 0;
    virtual void visit(BreakStatement& node) = 0;
    virtual void visit(ReturnStatement& node) = 0;

    virtual void visit(BinaryExpression& node) = 0;
    virtual void visit(CastExpression& node) = 0;
    virtual void visit(UnaryExpression& node) = 0;
    virtual void visit(AssignmentExpression& node) = 0;
    virtual void visit(ConditionalExpression& node) = 0;
    virtual void visit(IdentifierExpression& node) = 0;
    virtual void visit(ConstExpression& node) = 0;
    virtual void visit(LiteralExpression& node) = 0;
    virtual void visit(StringExpression& node) = 0;
    virtual void visit(CallExpression& node) = 0;
    virtual void visit(MemberAccessExpression& node) = 0;
    virtual void visit(ArraySubscriptExpression& node) = 0;
    virtual void visit(PostfixExpression& node) = 0;
    virtual void visit(SizeofExpression& node) = 0;
};

} // namespace ecc::ast

#endif