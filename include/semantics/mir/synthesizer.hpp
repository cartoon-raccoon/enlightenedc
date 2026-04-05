#pragma once

#ifndef ECC_MIR_SYNTH_H
#define ECC_MIR_SYNTH_H

#include <concepts>
#include <stdexcept>
#include <utility>
#include <variant>

#include "ast/ast.hpp"
#include "semantics/mir/mir.hpp"
#include "semantics/semantics.hpp"
#include "semantics/symbols.hpp"
#include "semantics/types.hpp"
#include "util.hpp"

namespace ecc::sema {

using namespace ecc;
using namespace util;

// Helper struct for building declarators.
struct DeclaratorBuilder {
    Optional<std::string> name;
    types::TypeBuilder ty_bldr;
};

// A struct for returning types and their associated names from a TypeSpecifier.
template <typename Ty>
    requires std::derived_from<Ty, typename types::Type>
struct TypeSpecRet {
    Optional<sym::TypeSymbol *> symbol;
    Ty *type;
};

// The result of visiting an InitDeclarator node.
struct InitDecltrRet {
    Optional<std::string> name;
    types::Type *type;
    Optional<Box<sema::mir::InitializerMIR>> init_mir;
};

// The result of visiting an Initializer node.
struct InitializerRet {
    // A new type to apply if needed.
    // Used only in array size inference.
    Optional<types::ArrayType *> new_type;
    Box<sema::mir::InitializerMIR> init_mir;
};

// The result of visiting a compound statement from a function.
using CmpdStmtFromFuncRes = std::pair<Box<sema::mir::CompoundStmtMIR>, sema::sym::Scope *>;

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
    TypeSpecRet<types::ClassType>, TypeSpecRet<types::UnionType>, TypeSpecRet<types::EnumType>,
    types::VoidType *, types::PrimitiveType *, types::PointerType *, types::Type *,
    sym::TypeSymbol *,
    // The result of visiting a ParameterDeclaration node.
    types::FuncParam,
    // The result of visiting a TypeQualifier node.
    ast::TypeQualifier::QualType,
    // The result of visiting a StorageClassSpecifier node.
    ast::StorageClassSpecifier::SpecType,

    Box<sema::mir::ProgItemMIR>, Box<sema::mir::FunctionMIR>,
    // The return type of visiting a CompoundStatement node from a Function node.
    CmpdStmtFromFuncRes,
    /*
    The results of visiting various Declaration, Statement, and Expression nodes.
    We do not use the specific types, as we cannot match on those when returning,
    due to how std::variant's visit and get_if functions work.
    */
    Box<sema::mir::DeclMIR>, Box<sema::mir::StmtMIR>, Box<sema::mir::ExprMIR>,
    Box<sema::mir::InitializerMIR>,
    // The return type of visiting an InitDeclarator.
    InitDecltrRet,
    // The return type of visiting an Initializer.
    InitializerRet>;

using CmpdStmtDoVisitParam =
    Optional<std::pair<sym::FuncSymbol *, // The function symbol to tie this compound statement to.
                       Vec<Box<sym::VarSymbol>> // The new symbols to add to the new scope.
                       >>;

/*
Any parameters to be passed to a do_visit call (through accept).
*/
using ElabVisitParam = std::variant<
    // The base variant, when the do_visit call does not take parameters.
    std::monostate,
    // A simple string, for anything.
    std::string, DeclaratorBuilder *,
    // For passing a function's information into the compound statement.
    CmpdStmtDoVisitParam,
    // For passing types for population.
    types::ClassType *, types::UnionType *, types::EnumType *, types::PrimitiveType *,
    types::BaseType *, types::Type *>;

/**
The class that lowers the AST to MIR, populating the TypeContext and SymbolTable.
*/
class MIRSynthesizer : public BaseASTSemaVisitor, public NoMove {
public:
    MIRSynthesizer(sym::SymbolTable& syms, types::TypeContext& types, mir::ProgramMIR& mir)
        : BaseASTSemaVisitor(BaseSemanticVisitor::State::WRITE), prog_mir(mir), types(types),
          syms(syms) {}

    types::TypeContext& types;

    sym::SymbolTable& syms;

    mir::ProgramMIR& prog_mir;

    Vec<Box<EccSemError>> errors;

    /**
    Run the MIRSynthesizer on the provided AST.
    */
    void generate_mir(ast::Program& prog);

protected:
    /*
    The result of the last visit(ast::) call. This is essentially the `return` value,
    placed here since visit calls cannot directly return values.
    */
    ElabResult last_result = std::monostate{};

    ElabVisitParam dovisit_param = std::monostate{};

    ScopeGuard<ast::ASTNode> enter_scope(sym::FuncSymbol *assoc = nullptr) override {
        return ScopeGuard<ast::ASTNode>(state, syms, assoc);
    }
    /*
    Takes the result of the last visit call, replacing it with `std::monostate`.
    */
    template <typename T>
        requires VariantMember<T, ElabResult>
    T take_last_result() {
        T ret;
        try {
            ret         = std::move(std::get<T>(last_result));
            last_result = std::monostate();

        } catch (std::bad_variant_access e) {
            throw std::runtime_error("got wrong type for take_last_result: " +
                                     std::string(e.what()));
        }

        return std::move(ret);
    }

    template <typename T>
        requires VariantMember<T, ElabVisitParam>
    T take_dovisit_param() {
        T ret;
        try {
            ret           = std::move(std::get<T>(dovisit_param));
            dovisit_param = std::monostate();

        } catch (std::bad_variant_access e) {
            throw std::runtime_error("got wrong type for take_dovisit_param: " +
                                     std::string(e.what()));
        }

        return std::move(ret);
    }

    template <typename E, typename... Args>
        requires std::derived_from<E, EccSemError>
    void add_error(Args... args) {
        Box<EccSemError> err = std::make_unique<E>(args...);
        errors.push_back(std::move(err));
    }

    /* DO_VISIT OVERRIDES */
protected:
    void do_visit(ast::Program& node) override;
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
    void do_visit(ast::TypeIdentifier& node) override;
    void do_visit(ast::VoidSpecifier& node) override;
    void do_visit(ast::PrimitiveSpecifier& node) override;
    void do_visit(ast::Initializer& node) override;
    void do_visit(ast::TypeName& node) override;
    void do_visit(ast::IdentifierDeclarator& node) override;

    void do_visit(ast::CompoundStatement& node) override;
    void do_visit(ast::ExpressionStatement& node) override;
    void do_visit(ast::CaseStatement& node) override;
    void do_visit(ast::CaseRangeStatement& node) override;
    void do_visit(ast::DefaultStatement& node) override;
    void do_visit(ast::LabeledStatement& node) override;
    void do_visit(ast::PrintStatement& node) override;
    void do_visit(ast::IfStatement& node) override;
    void do_visit(ast::SwitchStatement& node) override;
    void do_visit(ast::WhileStatement& node) override;
    void do_visit(ast::DoWhileStatement& node) override;
    void do_visit(ast::ForStatement& node) override;
    void do_visit(ast::GotoStatement& node) override;
    void do_visit(ast::BreakStatement& node) override;
    void do_visit(ast::ContinueStatement& node) override;
    void do_visit(ast::ReturnStatement& node) override;

    void do_visit(ast::BinaryExpression& node) override;
    void do_visit(ast::CastExpression& node) override;
    void do_visit(ast::UnaryExpression& node) override;
    void do_visit(ast::AssignmentExpression& node) override;
    void do_visit(ast::ConditionalExpression& node) override;
    void do_visit(ast::IdentifierExpression& node) override;
    void do_visit(ast::ConstExpression& node) override;
    void do_visit(ast::LiteralExpression& node) override;
    void do_visit(ast::StringExpression& node) override;
    void do_visit(ast::CallExpression& node) override;
    void do_visit(ast::MemberAccessExpression& node) override;
    void do_visit(ast::ArraySubscriptExpression& node) override;
    void do_visit(ast::PostfixExpression& node) override;
    void do_visit(ast::SizeofExpression& node) override;

private:
    struct SpecifierInfo {
        types::BaseType *type = nullptr;
        Optional<sym::TypeSymbol *> symbol;
        bool is_public                       = false;
        bool is_static                       = false;
        bool is_const                        = false;
        sym::PhysicalSymbol::Linkage linkage = sym::PhysicalSymbol::Linkage::INTERNAL;
    };

    Box<SpecifierInfo> parse_speclist(Vec<Box<ast::DeclarationSpecifier>>&, Location);
};

} // namespace ecc::sema

#endif