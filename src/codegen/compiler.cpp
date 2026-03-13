#include "compiler.hpp"

using namespace ecc::compiler;
using namespace ecc::sema::types;
using namespace ecc::ast;

void LLVMVisitor::visit(Program& node) {}
void LLVMVisitor::visit(Function& node) {}

void LLVMVisitor::visit(TypeDeclaration& node) {}
void LLVMVisitor::visit(VariableDeclaration& node) {}
void LLVMVisitor::visit(ParameterDeclaration& node) {}
void LLVMVisitor::visit(Declarator& node) {}
void LLVMVisitor::visit(ParenDeclarator& node) {}
void LLVMVisitor::visit(ArrayDeclarator& node) {}
void LLVMVisitor::visit(FunctionDeclarator& node) {}
void LLVMVisitor::visit(InitDeclarator& node) {}
void LLVMVisitor::visit(Pointer& node) {}
void LLVMVisitor::visit(ClassDeclarator& node) {}
void LLVMVisitor::visit(ClassDeclaration& node) {}
void LLVMVisitor::visit(Enumerator& node) {}
void LLVMVisitor::visit(StorageClassSpecifier& node) {}
void LLVMVisitor::visit(TypeQualifier& node) {}
void LLVMVisitor::visit(EnumSpecifier& node) {}
void LLVMVisitor::visit(PrimitiveSpecifier& node) {}
void LLVMVisitor::visit(ClassSpecifier& node) {}
void LLVMVisitor::visit(UnionSpecifier& node) {}
void LLVMVisitor::visit(Initializer& node) {}
void LLVMVisitor::visit(TypeName& node) {}
void LLVMVisitor::visit(IdentifierDeclarator& node) {}

void LLVMVisitor::visit(CompoundStatement& node) {}
void LLVMVisitor::visit(ExpressionStatement& node) {}
void LLVMVisitor::visit(CaseStatement& node) {}
void LLVMVisitor::visit(CaseRangeStatement& node) {}
void LLVMVisitor::visit(DefaultStatement& node) {}
void LLVMVisitor::visit(LabeledStatement& node) {}
void LLVMVisitor::visit(PrintStatement& node) {}
void LLVMVisitor::visit(IfStatement& node) {}
void LLVMVisitor::visit(SwitchStatement& node) {}
void LLVMVisitor::visit(WhileStatement& node) {}
void LLVMVisitor::visit(DoWhileStatement& node) {}
void LLVMVisitor::visit(ForStatement& node) {}
void LLVMVisitor::visit(GotoStatement& node) {}
void LLVMVisitor::visit(BreakStatement& node) {}
void LLVMVisitor::visit(ReturnStatement& node) {}

void LLVMVisitor::visit(BinaryExpression& node) {}
void LLVMVisitor::visit(CastExpression& node) {}
void LLVMVisitor::visit(UnaryExpression& node) {}
void LLVMVisitor::visit(AssignmentExpression& node) {}
void LLVMVisitor::visit(ConditionalExpression& node) {}
void LLVMVisitor::visit(IdentifierExpression& node) {}
void LLVMVisitor::visit(ConstExpression& node) {}
void LLVMVisitor::visit(LiteralExpression& node) {}
void LLVMVisitor::visit(StringExpression& node) {}
void LLVMVisitor::visit(CallExpression& node) {}
void LLVMVisitor::visit(MemberAccessExpression& node) {}
void LLVMVisitor::visit(ArraySubscriptExpression& node) {}
void LLVMVisitor::visit(PostfixExpression& node) {}
void LLVMVisitor::visit(SizeofExpression& node) {}