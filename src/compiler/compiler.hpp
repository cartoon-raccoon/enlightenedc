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
#include "compiler/semantics.hpp"

using namespace ecc;

namespace ecc::compiler {

using LLVMType = llvm::Type;

// todo: use LLVM DataLayout to handle alignment

class LLVMVisitor : public ast::ASTVisitor {
public:
    LLVMVisitor(compiler::SymbolTable& syms, compiler::types::TypeContext& tys)
    : syms(syms), tys(tys) {}
    ~LLVMVisitor() = default;

    Box<llvm::Module> module;
    Box<llvm::LLVMContext> ctxt;
    Box<llvm::IRBuilder<>> builder;

    compiler::SymbolTable& syms;
    compiler::types::TypeContext& tys;

    /*
    Converts the 
    */
    LLVMType *map_to_llvm_type(types::Type *ty);

    // Visitor method overrides

    void visit(ast::Program& node) override;
    void visit(ast::Function& node) override;

    void visit(ast::VariableDeclaration& node) override;
    void visit(ast::ParameterDeclaration& node) override;
    void visit(ast::Declarator& node) override;
    void visit(ast::ParenDeclarator& node) override;
    void visit(ast::ArrayDeclarator& node) override;
    void visit(ast::FunctionDeclarator& node) override;
    void visit(ast::InitDeclarator& node) override;
    void visit(ast::Pointer& node) override;
    void visit(ast::ClassDeclarator& node) override;
    void visit(ast::ClassDeclaration& node) override;
    void visit(ast::Enumerator& node) override;
    void visit(ast::StorageClassSpecifier& node) override;
    void visit(ast::TypeQualifier& node) override;
    void visit(ast::EnumSpecifier& node) override;
    void visit(ast::PrimitiveSpecifier& node) override;
    void visit(ast::ClassOrUnionSpecifier& node) override;
    void visit(ast::Initializer& node) override;
    void visit(ast::TypeName& node) override;
    void visit(ast::IdentifierDeclarator& node) override;

    void visit(ast::CompoundStatement& node) override;
    void visit(ast::ExpressionStatement& node) override;
    void visit(ast::CaseDefaultStatement& node) override;
    void visit(ast::LabeledStatement& node) override;
    void visit(ast::PrintStatement& node) override;
    void visit(ast::IfStatement& node) override;
    void visit(ast::SwitchStatement& node) override;
    void visit(ast::WhileStatement& node) override;
    void visit(ast::DoWhileStatement& node) override;
    void visit(ast::ForStatement& node) override;
    void visit(ast::GotoStatement& node) override;
    void visit(ast::BreakStatement& node) override;
    void visit(ast::ReturnStatement& node) override;

    void visit(ast::BinaryExpression& node) override;
    void visit(ast::CastExpression& node) override;
    void visit(ast::UnaryExpression& node) override;
    void visit(ast::AssignmentExpression& node) override;
    void visit(ast::ConditionalExpression& node) override;
    void visit(ast::IdentifierExpression& node) override;
    void visit(ast::LiteralExpression& node) override;
    void visit(ast::StringExpression& node) override;
    void visit(ast::CallExpression& node) override;
    void visit(ast::MemberAccessExpression& node) override;
    void visit(ast::ArraySubscriptExpression& node) override;
    void visit(ast::PostfixExpression& node) override;
    void visit(ast::SizeofExpression& node) override;
};

} // namespace ecc::compiler

#endif