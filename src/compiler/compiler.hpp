#ifndef ECC_COMPILER_H
#define ECC_COMPILER_H

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>

#include "ast/ast.hpp"
#include "ast/visitor.hpp"
#include "util.hpp"
#include "compiler/types.hpp"

namespace ecc::compiler {

using namespace ecc::util;
using namespace ecc::ast;
using namespace ecc::compiler::types;

using LLVMType = llvm::Type;

/*
Converts a Type to its corresponding LLVM type.
*/
LLVMType *map_to_llvm_type(Type * ty);

// todo: use LLVM DataLayout to handle alignment

class LLVMVisitor : public ast::ASTVisitor {
public:
    ~LLVMVisitor() = default;

    Box<llvm::Module> module;
    Box<llvm::LLVMContext> ctxt;
    Box<llvm::IRBuilder<>> builder;

    // Visitor method overrides

    void visit(Program& node) override;
    void visit(Function& node) override;

    void visit(VariableDeclaration& node) override;
    void visit(ParameterDeclaration& node) override;
    void visit(Declarator& node) override;
    void visit(ParenDeclarator& node) override;
    void visit(ArrayDeclarator& node) override;
    void visit(FunctionDeclarator& node) override;
    void visit(InitDeclarator& node) override;
    void visit(Pointer& node) override;
    void visit(StructDeclarator& node) override;
    void visit(StructDeclaration& node) override;
    void visit(Enumerator& node) override;
    void visit(StorageClassSpecifier& node) override;
    void visit(TypeSpecifier& node) override;
    void visit(TypeQualifier& node) override;
    void visit(EnumSpecifier& node) override;
    void visit(StructOrUnionSpecifier& node) override;
    void visit(Initializer& node) override;
    void visit(TypeName& node) override;
    void visit(IdentifierDeclarator& node) override;

    void visit(CompoundStatement& node) override;
    void visit(ExpressionStatement& node) override;
    void visit(CaseDefaultStatement& node) override;
    void visit(LabeledStatement& node) override;
    void visit(PrintStatement& node) override;
    void visit(IfStatement& node) override;
    void visit(SwitchStatement& node) override;
    void visit(WhileStatement& node) override;
    void visit(DoWhileStatement& node) override;
    void visit(ForStatement& node) override;
    void visit(JumpStatement& node) override;
    void visit(GotoStatement& node) override;
    void visit(BreakStatement& node) override;
    void visit(ReturnStatement& node) override;

    void visit(BinaryExpression& node) override;
    void visit(UnaryExpression& node) override;
    void visit(AssignmentExpression& node) override;
    void visit(ConditionalExpression& node) override;
    void visit(IdentifierExpression& node) override;
    void visit(LiteralExpression& node) override;
    void visit(StringExpression& node) override;
    void visit(CallExpression& node) override;
    void visit(MemberAccessExpression& node) override;
    void visit(ArraySubscriptExpression& node) override;
    void visit(PostfixExpression& node) override;
    void visit(SizeofExpression& node) override;
};

} // namespace ecc::compiler

#endif