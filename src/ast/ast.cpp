#include "ast/ast.hpp"
#include "ast/visitor.hpp"
#include "eval/exec.hpp"

using namespace ecc::ast;

void Program::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void Function::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void TypeDeclaration::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void VariableDeclaration::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void ParameterDeclaration::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void InitDeclarator::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void Pointer::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void ClassDeclarator::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void ClassDeclaration::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void Enumerator::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void StorageClassSpecifier::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void TypeQualifier::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void EnumSpecifier::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void TypeIdentifier::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void VoidSpecifier::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void PrimitiveSpecifier::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void Initializer::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void TypeName::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void CompoundStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void ExpressionStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void CaseStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void CaseRangeStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void DefaultStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void LabeledStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void PrintStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void IfStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void SwitchStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void WhileStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void DoWhileStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void ForStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void GotoStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void BreakStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void ContinueStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void ReturnStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void BinaryExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void CastExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void UnaryExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void AssignmentExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void ConditionalExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void IdentifierExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void ConstExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void LiteralExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void StringExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void CallExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void MemberAccessExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void ArraySubscriptExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void PostfixExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void SizeofExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void Declarator::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void IdentifierDeclarator::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void ParenDeclarator::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void ArrayDeclarator::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void FunctionDeclarator::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void ClassSpecifier::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void UnionSpecifier::accept(ASTVisitor& visitor) { visitor.visit(*this); }

exec::Value ConstExpression::accept(exec::Evaluator& eval) { return eval.eval(this); }

exec::Value BinaryExpression::accept(exec::Evaluator& eval) { return eval.eval(this); }

exec::Value CastExpression::accept(exec::Evaluator& eval) { return eval.eval(this); }

exec::Value UnaryExpression::accept(exec::Evaluator& eval) { return eval.eval(this); }

exec::Value AssignmentExpression::accept(exec::Evaluator& eval) { return eval.eval(this); }

exec::Value ConditionalExpression::accept(exec::Evaluator& eval) { return eval.eval(this); }

exec::Value IdentifierExpression::accept(exec::Evaluator& eval) { return eval.eval(this); }

exec::Value LiteralExpression::accept(exec::Evaluator& eval) { return eval.eval(this); }

exec::Value StringExpression::accept(exec::Evaluator& eval) { return eval.eval(this); }

exec::Value CallExpression::accept(exec::Evaluator& eval) { return eval.eval(this); }

exec::Value MemberAccessExpression::accept(exec::Evaluator& eval) { return eval.eval(this); }

exec::Value ArraySubscriptExpression::accept(exec::Evaluator& eval) { return eval.eval(this); }

exec::Value PostfixExpression::accept(exec::Evaluator& eval) { return eval.eval(this); }

exec::Value SizeofExpression::accept(exec::Evaluator& eval) { return eval.eval(this); }

void Program::add_item(std::unique_ptr<ProgramItem> item) {
    items.push_back(std::move(item));
}
