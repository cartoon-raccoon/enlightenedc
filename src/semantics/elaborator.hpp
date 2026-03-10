#ifndef ECC_ELAB_H
#define ECC_ELAB_H

#include <optional>
#include <stdexcept>
#include <utility>
#include <variant>

#include "ast/ast.hpp"
#include "semantics/types.hpp"
#include "semantics/symbols.hpp"
#include "semantics/semantics.hpp"

namespace ecc::sema {

using namespace ecc;
using namespace util;

// Helper struct for building declarators.
struct DeclaratorBuilder {
    std::string name;
    types::TypeBuilder ty_bldr;
};

/*
The result of visiting an AST node.

Each variant is the result returned by visiting a specific AST node.
*/
using ElabResult = std::variant<
    // The base variant, when visit() does not return anything.
    std::monostate,
    // A simple string, for string literals, identifiers, etc.
    std::string,
    // For building up declarators.
    Box<DeclaratorBuilder>,
    // The result of visiting a type specifier node.
    types::ClassType *,
    types::UnionType *,
    types::EnumType *,
    types::PrimitiveType *,
    types::PointerType *,
    // The result of visiting a TypeQualifier node.
    ast::TypeQualifier::QualType,
    // The result of visiting a StorageClassSpecifier node.
    ast::StorageClassSpecifier::SpecType
>;


using CmpdStmtDoVisitParam = std::optional<
    std::pair<
        sym::Symbol *, // The function symbol to tie this compound statement to.
        Vec<std::pair<std::string, Box<sym::Symbol>>> // The new symbols to add to the new scope.
    >
>;

/*
Any parameters to be passed to a do_visit call (through accept).
*/
using ElabVisitParam = std::variant<
    // The base variant, when the do_visit call does not take parameters.
    std::monostate,
    // A simple string, for anything.
    std::string,
    DeclaratorBuilder *,
    // For passing a function's information into the compound statement.
    CmpdStmtDoVisitParam,
    // For passing types for population.
    types::ClassType *,
    types::UnionType *,
    types::EnumType *,
    types::PrimitiveType *,
    types::Type *
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

    ElabVisitParam dovisit_param = std::monostate {};

    /*
    Takes the result of the last visit call, replacing it with `std::monostate`.
    */
    template <typename T>
    T take_last_result() {
        T ret;
        try {
            ret = std::move(std::get<T>(last_result));
            last_result = std::monostate();

        } catch (std::bad_variant_access e) {
            throw std::runtime_error("got wrong type for take_last_result: " + std::string(e.what()));
        }

        return std::move(ret);
    }

    template<typename T>
    T take_dovisit_param() {
        T ret;
        try {
            ret = std::move(std::get<T>(dovisit_param));
            dovisit_param = std::monostate();

        } catch (std::bad_variant_access e) {
            throw std::runtime_error("got wrong type for take_dovisit_param: " + std::string(e.what()));
        }

        return std::move(ret);
    }

private:
    struct SpecifierInfo {

    };

    Box<SpecifierInfo> parse_and_verify_speclist(Vec<Box<ast::DeclarationSpecifier>>&);

protected:

    void do_visit(ast::Function& node) override;

    void do_visit(ast::TypeDeclaration& node) override;
    void do_visit(ast::VariableDeclaration& node) override;
    void do_visit(ast::ParameterDeclaration& node) override;
    void do_visit(ast::Declarator& node) override;
    void do_visit(ast::ParenDeclarator& node) override;
    void do_visit(ast::ArrayDeclarator& node) override;
    void do_visit(ast::FunctionDeclarator& node) override;
    void do_visit(ast::InitDeclarator& node) override;
    void do_visit(ast::Pointer& node) override;
    void do_visit(ast::ClassDeclarator& node) override;
    void do_visit(ast::ClassDeclaration& node) override;
    void do_visit(ast::Enumerator& node) override;
    void do_visit(ast::StorageClassSpecifier& node) override;
    void do_visit(ast::TypeQualifier& node) override;
    void do_visit(ast::EnumSpecifier& node) override;
    void do_visit(ast::ClassSpecifier& node) override;
    void do_visit(ast::UnionSpecifier& node) override;
    void do_visit(ast::PrimitiveSpecifier& node) override;
    void do_visit(ast::Initializer& node) override;
    void do_visit(ast::TypeName& node) override;
    void do_visit(ast::IdentifierDeclarator& node) override;

    void do_visit(ast::ExpressionStatement& node) override;
    void do_visit(ast::CompoundStatement& node) override;
    void do_visit(ast::LabeledStatement& node) override;
    void do_visit(ast::WhileStatement& node) override;
    void do_visit(ast::DoWhileStatement& node) override;
    void do_visit(ast::ForStatement& node) override;
    void do_visit(ast::GotoStatement& node) override;
};

}

#endif