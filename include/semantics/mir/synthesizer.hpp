#pragma once

#ifndef ECC_MIR_SYNTH_H
#define ECC_MIR_SYNTH_H

#include <concepts>
#include <utility>
#include <variant>

#include "allocator/chunk.hpp"
#include "ast/ast.hpp"
#include "config.hpp"
#include "ds/arenavec.hpp"
#include "semantics/mir/mir.hpp"
#include "semantics/semantics.hpp"
#include "semantics/symbols.hpp"
#include "semantics/symdata.hpp"
#include "semantics/linkage.hpp"
#include "semantics/types.hpp"
#include "prelude.hpp"

namespace ecc::sema {

using namespace ecc;
using namespace util;

// Helper struct for building declarators.
struct DeclaratorBuilder {
    Optional<StringRef> name;
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
    Optional<StringRef> name;
    types::Type *type;
    Optional<Chunk<sema::mir::InitializerMIR>> init_mir;
};

// The result of visiting an Initializer node.
struct InitializerRet {
    // A new type to apply if needed.
    // Used only in array size inference.
    Optional<types::ArrayType *> new_type;
    Chunk<sema::mir::InitializerMIR> init_mir;
};

// The result of visiting a compound statement from a function.
struct CmpdStmtFromFuncRes {
    // The processed function body.
    Chunk<sema::mir::CompoundStmtMIR> body;
    // The scope of the body.
    sema::sym::Scope *funcscope;
    // The inserted params.
    Vec<sym::VarSymbol *> inserted_params;
};

/*
The result of visiting an AST node.

Each variant is the result returned by visiting a specific AST node.
*/
using VisitResult = std::variant<
    // The base variant, when visit() does not return anything.
    std::monostate,
    // A simple string, for string literals, identifiers, etc.
    StringRef,
    // The result of evaluating a ConstExpression.
    eval::Value,
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
    // The result of visiting a LangLinkageSpecifier node.
    ast::LangLinkageSpecifier::Lang,

    Chunk<sema::mir::ProgItemMIR>, Chunk<sema::mir::FunctionMIR>,
    // The return type of visiting a CompoundStatement node from a Function node.
    CmpdStmtFromFuncRes,
    /*
    The results of visiting various Declaration, Statement, and Expression nodes.
    We do not use the specific types, as we cannot match on those when returning,
    due to how std::variant's visit and get_if functions work.
    */
    Chunk<sema::mir::DeclMIR>, Chunk<sema::mir::StmtMIR>, Chunk<sema::mir::ExprMIR>,
    Chunk<sema::mir::InitializerMIR>,
    // The return type of visiting an InitDeclarator.
    InitDecltrRet,
    // The return type of visiting an Initializer.
    InitializerRet>;

struct FuncBodyVisitParam {
    sym::FuncSymbol *sym;
    ast::FunctionBody *body;
    Vec<sym::InsertVarArgs> params;
};

/*
Any parameters to be passed to a do_visit call (through accept).
*/
using VisitParam = std::variant<
    // The base variant, when the do_visit call does not take parameters.
    std::monostate,
    // A simple string, for anything.
    StringRef, DeclaratorBuilder *,
    // For passing types for population.
    types::RecordType *, types::EnumType *, types::PrimitiveType *, types::BaseType *,
    types::Type *, Pair<types::BaseType *, bool>,
    // The ProgItemMIR that an Attribute/AttributeArg is attached to, so do_visit can
    // downcast it (via its NodeKind) and validate/apply the attribute per item kind.
    mir::ProgItemMIR *>;

/**
The class that lowers the AST to MIR, populating the TypeContext and SymbolTable.
*/
class MIRSynthesizer : public BaseASTSemaVisitor, public Fallible, public NoMove {
    struct SpecifierInfo {
        types::BaseType *type = nullptr;
        Optional<sym::TypeSymbol *> symbol;
        bool is_const        = false;
        bool is_constexpr    = false;
        sema::Linkage linkage = sema::Linkage::NONE;
        sema::LangLinkage langlink = sema::LangLinkage::NONE;
        sym::StorageDuration duration = sym::StorageDuration::AUTO;
    };

    enum class DeclSpecCtxt : uint8_t { FILE, BLOCK, MEMBER, };

    enum class SpecMode : uint8_t { ANON, DEFINE, FORWARD, REF, };

public:
    MIRSynthesizer(
        sym::SymbolTable& syms, types::TypeContext& types, mir::ProgramMIR& mir,
        RuntimeConfig& rtcfg)
        : BaseASTSemaVisitor(BaseSemanticVisitor::State::WRITE, sym::SymbolTableWalker(syms)), 
        types(types), prog_mir(mir), rtcfg(rtcfg) {}

    types::TypeContext& types;

    mir::ProgramMIR& prog_mir;

    RuntimeConfig& rtcfg;

    /**
    Run the MIRSynthesizer on the provided AST.
    */
    void generate_mir(ast::Program& prog);

protected:
    /*
    The result of the last visit(ast::) call. This is essentially the `return` value,
    placed here since visit calls cannot directly return values.
    */
    VisitResult last_result = std::monostate{};

    VisitParam dovisit_param = std::monostate{};
    /*
    Takes the result of the last visit call, replacing it with `std::monostate`.
    */
    template <typename T>
        requires VariantMember<T, VisitResult>
    T take_last_result() {
        T ret;
        try {
            ret         = std::move(std::get<T>(last_result));
            last_result = std::monostate();

        } catch (std::bad_variant_access e) {
            ECC_UNREACHABLE(
                "got wrong type for take_last_result: " + std::string(e.what()));
        }

        return ret;
    }

    template <typename T>
        requires VariantMember<T, VisitParam>
    T take_dovisit_param() {
        T ret;
        try {
            ret           = std::move(std::get<T>(dovisit_param));
            dovisit_param = std::monostate();

        } catch (std::bad_variant_access e) {
            ECC_UNREACHABLE(
                "got wrong type for take_dovisit_param: " + std::string(e.what()));
        }

        return ret;
    }

    /* DO_VISIT OVERRIDES */
protected:
    void do_visit(ast::Program& node) override;
    // `dovisit_param` holds the `mir::ProgItemMIR *` that this attribute/arg is attached
    // to (set by the callers below), for per-item-kind validation.
    void do_visit(ast::AttributeArg& node) override;
    void do_visit(ast::Attribute& node) override;
    void do_visit(ast::Function& node) override;

    void do_visit(ast::TypeDeclaration& node) override;
    void do_visit(ast::ConstexprDeclaration& node) override;
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
    void do_visit(ast::LangLinkageSpecifier& node) override;
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
    void do_visit(ast::NullptrExpression& node) override;
    void do_visit(ast::CallExpression& node) override;
    void do_visit(ast::MemberAccessExpression& node) override;
    void do_visit(ast::ReinterpretExpression& node) override;
    void do_visit(ast::ArraySubscriptExpression& node) override;
    void do_visit(ast::PostfixExpression& node) override;
    void do_visit(ast::SizeofExpression& node) override;

    CmpdStmtFromFuncRes parse_function_body(FuncBodyVisitParam params);

    Chunk<mir::FunctionMIR> parse_vardecl_func(
        ast::VariableDeclaration&, InitDecltrRet ret, SpecifierInfo specinfo,
        types::FunctionType *type);

    eval::Value parse_constexpr_init(mir::InitializerMIR& init, types::Type *type);

    void check_attribute(mir::FunctionMIR *function, ast::AttributeArg& node);
    void check_attribute(mir::TypeDeclMIR *typedecl, ast::AttributeArg& node);

private:
    SpecifierInfo parse_speclist(ds::ArenaVec<Chunk<ast::DeclarationSpecifier>>&, DeclSpecCtxt);

    /**
    Check if a type specifier is a standalone declaration (i.e. part of a TypeDeclaration).
    */
    bool is_standalone_decl() {
        if (ctxt_stack.size() < 2) return false;
        return isa<ast::TypeDeclaration>(ctxt_stack[ctxt_stack.size() - 2]);
    }
};

} // namespace ecc::sema

#endif