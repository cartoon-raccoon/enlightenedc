#ifndef ECC_ELAB_H
#define ECC_ELAB_H

#include <variant>

#include "ast/ast.hpp"
#include "semantics/types.hpp"
#include "semantics/symbols.hpp"
#include "semantics/semantics.hpp"

namespace ecc::sema {

using namespace ecc;
using namespace util;

/*
The result of visiting an AST node.

Each variant is the result returned by visiting a specific AST node.
*/
using ElabResult = std::variant<
    // The base variant, when visit(ast::) does not return anything.
    std::monostate,
    // A simple string, for string literals, identifiers, etc.
    std::string,
    // The result of visiting a type specifier node.
    types::Type *,
    // The result of visiting a class or union specifier node.
    types::ClassType *,
    types::UnionType *
>;

/*
The class that performs the elaboration pass.
*/
class Elaborator : public BaseSemanticVisitor {
public:
    Elaborator(sym::SymbolTable& syms, types::TypeContext& types)
    : BaseSemanticVisitor(BaseSemanticVisitor::State::WRITE, syms, types) {}

    /*
    The result of the last visit(ast::) call. This is essentially the `return` value,
    placed here since visit calls cannot directly return values.
    */
    ElabResult last_result = std::monostate {};

    /*
    Takes the result of the last visit call, replacing it with `std::monostate`.
    */
    ElabResult take_last_result();

    void visit(ast::Function& node) override;

    void visit(ast::TypeDeclaration& node) override;
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
    void visit(ast::ClassOrUnionSpecifier& node) override;
    void visit(ast::PrimitiveSpecifier& node) override;
    void visit(ast::Initializer& node) override;
    void visit(ast::TypeName& node) override;
    void visit(ast::IdentifierDeclarator& node) override;

    void visit(ast::ExpressionStatement& node) override;
    void visit(ast::CaseDefaultStatement& node) override;
    void visit(ast::LabeledStatement& node) override;
    void visit(ast::WhileStatement& node) override;
    void visit(ast::DoWhileStatement& node) override;
    void visit(ast::ForStatement& node) override;
    void visit(ast::GotoStatement& node) override;
    void visit(ast::BreakStatement& node) override;
    void visit(ast::ReturnStatement& node) override;

protected:
    //void do_visit(ast::LabeledStatement& node) override;

    void do_visit(ast::ClassOrUnionSpecifier& node, types::ClassType *cls);

    void do_visit(ast::ClassOrUnionSpecifier& node, types::UnionType *unn);
};

}

#endif