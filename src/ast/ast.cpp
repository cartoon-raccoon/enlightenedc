#include "ast/ast.hpp"

#include "ast/visitor.hpp"
#include "util.hpp"

using namespace ecc::ast;

DO_ACCEPT(Program, ASTVisitor)
DO_ACCEPT(Function, ASTVisitor)
DO_ACCEPT(TypeDeclaration, ASTVisitor)
DO_ACCEPT(VariableDeclaration, ASTVisitor)
DO_ACCEPT(ParameterDeclaration, ASTVisitor)
DO_ACCEPT(InitDeclarator, ASTVisitor)
DO_ACCEPT(Pointer, ASTVisitor)
DO_ACCEPT(ClassDeclarator, ASTVisitor)
DO_ACCEPT(ClassDeclaration, ASTVisitor)
DO_ACCEPT(Enumerator, ASTVisitor)
DO_ACCEPT(StorageClassSpecifier, ASTVisitor)
DO_ACCEPT(TypeQualifier, ASTVisitor)
DO_ACCEPT(TypeIdentifier, ASTVisitor)
DO_ACCEPT(VoidSpecifier, ASTVisitor)
DO_ACCEPT(PrimitiveSpecifier, ASTVisitor)
DO_ACCEPT(Initializer, ASTVisitor)
DO_ACCEPT(TypeName, ASTVisitor)

DO_ACCEPT(CompoundStatement, ASTVisitor)
DO_ACCEPT(ExpressionStatement, ASTVisitor)
DO_ACCEPT(CaseStatement, ASTVisitor)
DO_ACCEPT(CaseRangeStatement, ASTVisitor)
DO_ACCEPT(DefaultStatement, ASTVisitor)
DO_ACCEPT(LabeledStatement, ASTVisitor)
DO_ACCEPT(PrintStatement, ASTVisitor)
DO_ACCEPT(IfStatement, ASTVisitor)
DO_ACCEPT(SwitchStatement, ASTVisitor)
DO_ACCEPT(WhileStatement, ASTVisitor)
DO_ACCEPT(DoWhileStatement, ASTVisitor)
DO_ACCEPT(ForStatement, ASTVisitor)
DO_ACCEPT(GotoStatement, ASTVisitor)
DO_ACCEPT(BreakStatement, ASTVisitor)
DO_ACCEPT(ContinueStatement, ASTVisitor)
DO_ACCEPT(ReturnStatement, ASTVisitor)

DO_ACCEPT(BinaryExpression, ASTVisitor)
DO_ACCEPT(CastExpression, ASTVisitor)
DO_ACCEPT(UnaryExpression, ASTVisitor)
DO_ACCEPT(AssignmentExpression, ASTVisitor)
DO_ACCEPT(ConditionalExpression, ASTVisitor)
DO_ACCEPT(IdentifierExpression, ASTVisitor)
DO_ACCEPT(ConstExpression, ASTVisitor)
DO_ACCEPT(LiteralExpression, ASTVisitor)
DO_ACCEPT(StringExpression, ASTVisitor)
DO_ACCEPT(CallExpression, ASTVisitor)
DO_ACCEPT(MemberAccessExpression, ASTVisitor)
DO_ACCEPT(ArraySubscriptExpression, ASTVisitor)
DO_ACCEPT(PostfixExpression, ASTVisitor)
DO_ACCEPT(SizeofExpression, ASTVisitor)
DO_ACCEPT(Declarator, ASTVisitor)
DO_ACCEPT(IdentifierDeclarator, ASTVisitor)
DO_ACCEPT(ParenDeclarator, ASTVisitor)
DO_ACCEPT(ArrayDeclarator, ASTVisitor)
DO_ACCEPT(FunctionDeclarator, ASTVisitor)
DO_ACCEPT(ClassSpecifier, ASTVisitor)
DO_ACCEPT(UnionSpecifier, ASTVisitor)

void Program::add_item(std::unique_ptr<ProgramItem> item) {
    items.push_back(std::move(item));
}

std::string ecc::ast::storage_to_string(StorageClassSpecifier::SpecType ty) {
    using S = StorageClassSpecifier::SpecType;
    switch (ty) {
    case S::PUBLIC:
        return "public";
    case S::STATIC:
        return "static";
    case S::EXTERN:
        return "extern";
    case S::EXTERNC:
        return "extern \"C\"";
    }
    return "";
}

std::string ecc::ast::qualifier_to_string(TypeQualifier::QualType qual) {
    using Q = TypeQualifier::QualType;
    switch (qual) {
    case Q::CONST:
        return "const";
    }
    return "";
}
