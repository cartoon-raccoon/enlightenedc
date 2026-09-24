#include "semantics/mir/synthesizer.hpp"

#include <cmath>
#include <memory>
#include <variant>

#include "ast/ast.hpp"
#include "builtins.hpp"
#include "ds/arenavec.hpp"
#include "error.hpp"
#include "eval/consteval.hpp"
#include "eval/value.hpp"
#include "semantics/attributes.hpp"
#include "semantics/linkage.hpp"
#include "semantics/mir/mir.hpp"
#include "semantics/semerr.hpp"
#include "semantics/symbols.hpp"
#include "semantics/symdata.hpp"
#include "semantics/typeerr.hpp"
#include "semantics/types.hpp"
#include "semantics/validator.hpp"
#include "prelude.hpp"

using namespace ecc::ds;

#define dv_return(val)                \
    do {                              \
        last_result = std::move(val); \
        return;                       \
    } while (0)

#define dv_return_void()                \
    do {                                \
        last_result = std::monostate{}; \
        return;                         \
    } while (0)

#define dv_call_noparam(obj)              \
    do {                                  \
        dovisit_param = std::monostate{}; \
        (obj)->accept(*this);             \
    } while (0)

#define dv_call(param, obj)               \
    do {                                  \
        dovisit_param = std::move(param); \
        (obj)->accept(*this);             \
    } while (0)

using namespace ecc::ast;
using namespace ecc::sema;
using namespace ecc::sema::types;
using namespace ecc::sema::sym;
using namespace ecc::sema::mir;
using namespace ecc::tokens;
using namespace ecc::eval;

void MIRSynthesizer::generate_mir(Program& prog) {
    prog.accept(*this);
}

MIRSynthesizer::SpecifierInfo
MIRSynthesizer::parse_speclist(ArenaVec<Chunk<ast::DeclarationSpecifier>>& speclist, Scope *scope) {
    using NK = ASTNode::NodeKind;

    SpecifierInfo specinfo;

    // fixme: accumulate everything, then check at the end

    for (auto& decl_spec : speclist) {
        decl_spec->accept(*this);
        switch (decl_spec->kind) {
        case NK::TYPE_QUAL: {
            auto qualtype = take_last_result<TypeQualifier::QualType>();
            switch (qualtype) {
            case TypeQualifier::QualType::CONST:
                if (specinfo.is_const) {
                    add_error<EccSemError>("duplicate const qualifier", decl_spec->loc);
                } else {
                    specinfo.is_const = true;
                }
            }
        } break;

        case NK::STORAGE_SPEC: {
            auto spectype = take_last_result<StorageClassSpecifier::SpecType>();
            switch (spectype) {
            case StorageClassSpecifier::PUBLIC:
                if (rtcfg.std == Config::Std::ENLIGHTENEDC) {
                    add_error<EccSemError>(
                        "usage of `public` is only allowed under the HolyC standard", decl_spec->loc);
                }
                if (specinfo.is_constexpr) {
                    add_error<EccSemError>("constexpr cannot have external linkage");
                }
                if (specinfo.linkage != Linkage::NONE) {
                    add_error<EccSemError>("multiple storage class specifiers");
                } else {
                    specinfo.linkage = Linkage::EXTERNAL;
                }
                break;

            case StorageClassSpecifier::STATIC:
                // we allow constexpr to exist alongside static, for futureproofing
                if (specinfo.linkage != Linkage::NONE) {
                    add_error<EccSemError>("multiple storage class specifiers");
                } else {
                    if (scope->is_global()) {
                        // file scope static is internal linkage
                        specinfo.linkage = Linkage::INTERNAL;
                    } else {
                        // block scope static is static duration, no linkage
                        specinfo.duration = StorageDuration::STATIC;
                    }
                }
                break;

            case StorageClassSpecifier::CONSTEXPR:
                // set constexpr to true unconditionally, so subsequent checks can catch it
                specinfo.is_constexpr = true;

                if (specinfo.linkage == Linkage::EXTERNAL) {
                    add_error<EccSemError>("constexpr cannot have external linkage", decl_spec->loc);
                    break;
                }
                if (scope->is_global()) {
                    // file scope constexpr is internal linkage
                    specinfo.linkage = Linkage::INTERNAL;
                    // block scope constexpr is no linkage
                }
                break;

            case StorageClassSpecifier::EXTERN:
                if (specinfo.is_constexpr) {
                    add_error<EccSemError>("constexpr cannot be marked extern", decl_spec->loc);
                    break;
                }
                if (specinfo.linkage != Linkage::NONE) {
                    add_error<EccSemError>("multiple storage class specifiers", decl_spec->loc);
                } else {
                    specinfo.linkage = Linkage::EXTERNAL;
                }
                break;

            case StorageClassSpecifier::EXTERNC:
                if (specinfo.is_constexpr) {
                    add_error<EccSemError>("constexpr cannot be marked extern", decl_spec->loc);
                    break;
                }
                if (specinfo.linkage != Linkage::NONE) {
                    add_error<EccSemError>("multiple storage class specifiers", decl_spec->loc);
                } else {
                    specinfo.linkage = Linkage::EXTERNAL;
                    specinfo.langlink = LangLinkage::C;
                }
                break;
            }
        } break;

        case NK::TYPE_IDENT: {
            specinfo.symbol = take_last_result<TypeSymbol *>();
            // No need to check for nullptr here because it is guaranteed,
            // if the ident didn't exist it would have thrown
            specinfo.type = (*specinfo.symbol)->type;
            break;
        }

        case NK::CLASS_SPEC: {
            auto typespecret = take_last_result<TypeSpecRet<ClassType>>();
            specinfo.type    = typespecret.type;
            specinfo.symbol  = typespecret.symbol;
            break;
        }

        case NK::UNION_SPEC: {
            auto typespecret = take_last_result<TypeSpecRet<UnionType>>();
            specinfo.type    = typespecret.type;
            specinfo.symbol  = typespecret.symbol;
            break;
        }

        case NK::ENUM_SPEC: {
            auto typespecret = take_last_result<TypeSpecRet<EnumType>>();
            specinfo.type    = typespecret.type;
            specinfo.symbol  = typespecret.symbol;
            break;
        }

        case NK::VOID_SPEC:
            specinfo.type = take_last_result<VoidType *>();
            break;

        case NK::PRIM_SPEC:
            specinfo.type = take_last_result<PrimitiveType *>();
            break;

        default:
            ECC_UNREACHABLE(
                "encountered a non-declaration specifier while parsing specifiers");
        }
    }

    if (scope->is_global() && specinfo.linkage == Linkage::NONE) {
        // file-scope unspecifieds get external linkage implicitly
        specinfo.linkage = Linkage::EXTERNAL;
    }
    ECC_ASSERT_N(specinfo.type);

    return specinfo;
}

void MIRSynthesizer::do_visit(Program& node) {
    bsv_dbprint("visiting Program node: ", node.loc);

    for (auto& item : node.items) {
        dv_call_noparam(item);
        std::visit(
            match{
                [&](Chunk<DeclMIR>& decl) mutable { prog_mir.add_item(std::move(decl)); },
                [&](Chunk<StmtMIR>& stmt) mutable { prog_mir.add_item(std::move(stmt)); },
                [&](Chunk<FunctionMIR>& func) mutable { prog_mir.add_item(std::move(func)); },
                [&](std::monostate&) {
                    // ignore and continue
                },
                [&](auto&) {
                    ECC_UNREACHABLE("unexpected item while parsing programitems");
                }},
            last_result);
        last_result = std::monostate{};
    }

    dv_return_void();
}

void MIRSynthesizer::do_visit(AttributeArg& node) {
    bsv_dbprint("visiting AttributeArg node: ", node.loc);

    auto *progitem = take_dovisit_param<ProgItemMIR *>();
    if (auto *function = dyncast<FunctionMIR>(progitem); function) {
        check_attribute(function, node);
    } else if (auto *typedecl = dyncast<TypeDeclMIR>(progitem); typedecl) {
        check_attribute(typedecl, node);
    } else {
        ECC_UNREACHABLE("invalid ProgItem for visiting AttributeArg");
    }
}

using namespace ecc::sema::attr;

void MIRSynthesizer::check_attribute(FunctionMIR *function, AttributeArg& node) {
    const auto *attrdata = find_attr(node.name);
    if (attrdata == nullptr) {
        add_error<InvalidAttributeError>(node.loc, node.name);
        throw UnableToContinue();
    }

    if (attrdata->target != AttributeTarget::FUNCTION) {
        add_error<InvalidAttributeError>(
            node.loc, node.name, InvalidAttributeError::Target::Function);
        throw UnableToContinue();
    }

    if (attrdata->takes_value && !node.value) {
        add_error<InvalidAttributeError>(
            node.loc, InvalidAttributeError::Kind::ValueNotProvided, node.name);
        throw UnableToContinue();
    } else if (!attrdata->takes_value && node.value) {
        add_error<InvalidAttributeError>(
            node.loc, InvalidAttributeError::Kind::ProvidedValue, node.name, *node.value);
        throw UnableToContinue();
    }

    try {
        attrdata->action(*function, node.value);
    } catch (InvalidAttributeError& err) {
        add_error<InvalidAttributeError>(err);
    }
}

void MIRSynthesizer::check_attribute(TypeDeclMIR *typedecl, AttributeArg& node) {
    const auto *attrdata = find_attr(node.name);
    if (attrdata == nullptr) {
        add_error<InvalidAttributeError>(node.loc, node.name);
        throw UnableToContinue();
    }

    if (attrdata->target != AttributeTarget::TYPE) {
        add_error<InvalidAttributeError>(node.loc, node.name, InvalidAttributeError::Target::Type);
        throw UnableToContinue();
    }

    if (attrdata->takes_value && !node.value) {
        add_error<InvalidAttributeError>(
            node.loc, InvalidAttributeError::Kind::ValueNotProvided, node.name);
        throw UnableToContinue();
    } else if (!attrdata->takes_value && node.value) {
        add_error<InvalidAttributeError>(
            node.loc, InvalidAttributeError::Kind::ProvidedValue, node.name, *node.value);
        throw UnableToContinue();
    }

    try {
        attrdata->action(*typedecl, node.value);
    } catch (InvalidAttributeError& err) {
        add_error<InvalidAttributeError>(err);
    }
}

void MIRSynthesizer::do_visit(Attribute& node) {
    bsv_dbprint("visiting Attribute node: ", node.loc);
    auto *progitem = take_dovisit_param<ProgItemMIR *>();
    for (auto& arg : node.args) {
        dv_call(progitem, arg);
    }
}

void MIRSynthesizer::do_visit(Function& node) {
    bsv_dbprint("visiting Function node: ", node.loc);

    // Parse and construct specifier info
    VisitParam param       = dovisit_param;
    SpecifierInfo specinfo = parse_speclist(node.decl_spec_list, syms.current);
    dovisit_param          = param;

    if (specinfo.is_constexpr) {
        add_error<EccSemError>("function cannot be marked constexpr", node.declarator->loc);
        throw UnableToContinue();
    }

    BaseType *return_base = specinfo.type;

    if (!node.declarator->direct) {
        add_error<EccSemError>(
            "function declaration but missing direct declarator", node.declarator->loc);
        throw UnableToContinue();
    }
    if (node.declarator->direct.value()->kind != ASTNode::NodeKind::FUNC_DECLTR) {
        add_error<EccSemError>(
            "function declaration but declarator is not function", node.declarator->loc);
        throw UnableToContinue();
    }

    // Visit the Declarator to construct the type builder.
    dv_call_noparam(node.declarator);
    auto builder = take_last_result<Box<DeclaratorBuilder>>();
    builder->ty_bldr.set_base(return_base, specinfo.is_const);

    // The latest function parameters.
    Vec<FuncParam> last_func_params;

    // Extract the base type from our builder.
    Type *curr = builder->ty_bldr.finalize(last_func_params);

    FunctionType *functype = curr->as_function();
    if (!functype) {
        add_error<EccSemError>("unable to resolve declarator to function declarator", node.loc);
        throw UnableToContinue();
    }
    functype = functype->with_lang_linkage(specinfo.langlink);

    if (!builder->name) {
        add_error<EccSemError>("unable to parse name from declarator", node.declarator->loc);
        throw UnableToContinue();
    }

    bsv_dbprint("MIRSynthesizer: parsing Function params");
    bool found_default = false;
    Vec<InsertVarArgs> params;
    for (FuncParam& param : last_func_params) {

        if (!param.name) {
            add_error<EccSemError>("parameter in function declaration has no name", param.loc);
            throw UnableToContinue();
        }

        Type *sym_type = param.type;

        if (param.value) {
            found_default = true;
            InsertVarArgs paramsym = {param.loc, *param.name, sym_type, *param.value};
            params.push_back(std::move(paramsym));
        } else {
            if (found_default) {
                add_error<EccSemError>(
                    "parameters without default values must be before all default ones", param.loc);
                throw UnableToContinue();
            }
            // just add without value and continue for now
            InsertVarArgs paramsym = {param.loc, *param.name, sym_type};
            params.push_back(std::move(paramsym));
        }
    }

    // Insert func before visiting the body, so the function can call itself.
    InsertFuncArgs funcargs = {node.loc, *builder->name, functype};
    funcargs.has_body = true;
    funcargs.linkage = specinfo.linkage;

    FuncSymbol *funcsym;
    try {
        funcsym = syms.insert_func(std::move(funcargs));
    } catch (Symbol *previous) {
        add_error<SymbolAlrDecldError>(
            std::format("function \"{}\" was previously declared", previous->get_name()), node.declarator->loc,
            previous->get_loc());
        throw UnableToContinue();
    }

    FuncBodyVisitParam cmpdstmtp({funcsym, node.body.get(), std::move(params)});

    auto res = parse_function_body(std::move(cmpdstmtp));

    ECC_ASSERT_N(funcsym);
    for (auto *insd_param : res.inserted_params) {
        if (insd_param->has_value()) {
            funcsym->add_default_param(insd_param, *insd_param->get_value());
        } else {
            funcsym->add_parameter(insd_param);
        }
    }

    res.funcscope->set_assoc(funcsym, true);

    if (functype->returntype()->is_void()) {
        if (res.body->items.empty() || !isa<ReturnStmtMIR>(res.body->items.back())) {
            res.body->add_item(make_chunk<ReturnStmtMIR>(Location(), res.funcscope));
        }
    }

    Chunk<FunctionMIR> func = make_chunk<FunctionMIR>(
        node.loc, node.declarator->loc, funcsym, syms.current, res.funcscope, std::move(res.body));

    for (auto& attr : node.attributes) {
        dv_call(func.get(), attr);
    }

    dv_return(func);
}

CmpdStmtFromFuncRes MIRSynthesizer::parse_function_body(FuncBodyVisitParam params) {
    syms.push_scope();

    Vec<VarSymbol *> inserted_params;
    for (auto& sym : params.params) {
        inserted_params.push_back(syms.insert_var(std::move(sym)));
    }

    ArenaVec<Chunk<ProgItemMIR>> progitems;

    // Variadic function and HolyC standard: insert implicit argc, argv
    if (params.sym->get_signature()->is_variadic() && rtcfg.std == Config::Std::HOLYC) {
        Type *argc_type = types.get_size_type(false);
        auto argc_insertargs = InsertVarArgs(Location {}, EC_IMPLICIT_ARGC, argc_type);

        Type *argv_type = types.get_pointer(types.get_pointer(types.get_void()));
        auto argv_insertargs = InsertVarArgs(Location {}, EC_IMPLICIT_ARGV, argv_type);

        VarSymbol *argc_sym = syms.insert_implicit(argc_insertargs);
        VarSymbol *argv_sym = syms.insert_implicit(argv_insertargs);

        auto argc_vardecl = make_chunk<VarDeclMIR>(Location {}, syms.current);
        argc_vardecl->add_decl(argc_sym);

        auto argv_vardecl = make_chunk<VarDeclMIR>(Location {}, syms.current);
        argv_vardecl->add_decl(argv_sym);

        progitems.push_back(std::move(argc_vardecl));
        progitems.push_back(std::move(argv_vardecl));
    }

    for (auto& item : params.body->items) {
        dv_call_noparam(item);
        std::visit(
            match{
                [&](Chunk<DeclMIR>& decl) mutable { progitems.push_back(std::move(decl)); },
                [&](Chunk<StmtMIR>& stmt) mutable { progitems.push_back(std::move(stmt)); },
                [&](Chunk<FunctionMIR>& func) mutable { progitems.push_back(std::move(func)); },
                [](std::monostate&) {
                    // ignore and continue
                },
                [](auto&) {
                    ECC_UNREACHABLE("unexpected type while parsing program items");
                }},
            last_result);
        last_result = std::monostate{};
    }

    Scope *assoc_scope = syms.current;

    syms.pop_scope();

    Chunk<CompoundStmtMIR> cmpdmir =
        make_chunk<CompoundStmtMIR>(params.body->loc, std::move(progitems), syms.current);
    return { std::move(cmpdmir), assoc_scope, std::move(inserted_params)};
}

void MIRSynthesizer::do_visit(TypeDeclaration& node) {
    bsv_dbprint("visiting TypeDeclaration node: ", node.loc);

    auto specinfo = parse_speclist(node.specifiers, syms.current);

    if (specinfo.symbol) {
        TypeSymbol *symptr  = (*specinfo.symbol);
        Chunk<DeclMIR> decl = make_chunk<TypeDeclMIR>(node.loc, symptr);

        for (auto& attr : node.attributes) {
            dv_call(decl.get(), attr);
        }

        if (specinfo.is_const || specinfo.is_constexpr) {
            add_error<EccSemError>(
                "type declaration cannot be marked const or constexpr", node.loc);
        }

        dv_return(decl);
    } else {
        dv_return_void();
    }
}

void MIRSynthesizer::do_visit(ConstexprDeclaration& node) {
    bsv_dbprint("visiting ConstexprDeclaration node: ", node.loc);

    auto specinfo = parse_speclist(node.specifiers, syms.current);

    ECC_ASSERT(specinfo.is_constexpr, "visiting ConstexprDeclaration but specinfo is not constexpr");

    if (!node.attributes.empty()) {
        // todo: warn that attributes on constexprs are ignored
    }

    for (auto& declarator : node.declarators) {
        auto param = Pair(specinfo.type, specinfo.is_const);
        dv_call(param, declarator);

        auto ret = take_last_result<InitDecltrRet>();

        if (!ret.name) {
            add_error<EccSemError>("constexpr declaration with no name", declarator->loc);
            throw UnableToContinue();
        }

        if (!ret.type->is_primitive() && !ret.type->is_pointer()) {
            add_error<InvalidConstexprError>(
                InvalidConstexprError::Kind::InvalidType, declarator->loc);
            throw UnableToContinue();
        }

        Type *symtype = types.get_const(ret.type);

        eval::Value val;
        if (ret.init_mir) {
            val = parse_constexpr_init(**ret.init_mir, ret.type);
        } else {
            add_error<InvalidConstexprError>(InvalidConstexprError::Kind::NoInitializer, declarator->loc);
            throw UnableToContinue();
        }

        InsertVarArgs args {declarator->loc, *ret.name, symtype, val};

        try {
            syms.insert_var(std::move(args));
        } catch (Symbol *existing) {
            add_error<SymbolAlrDecldError>(
                std::format("symbol {} already previously declared", existing->get_name()), declarator->loc,
                existing->get_loc());
            throw UnableToContinue();
        }
    }

    dv_return_void();
}

Value MIRSynthesizer::parse_constexpr_init(InitializerMIR& init, Type *type) {
    ExprMIR *init_expr = init.as_expr();
    if (!init_expr) {
        add_error<InvalidInitializerError>(
            "initializer to a constexpr must be an expression", init.loc);
        throw UnableToContinue();
    }

    if (!init_expr->is_const_foldable(true)) {
        add_error<InvalidCompileTimeEval>(
            "constexpr initializers must be compile-time evaluable", init_expr->loc);
        throw UnableToContinue();
    }

    sema::ExprValidator exprv(types, syms);
    try {
        init_expr->accept(exprv);
    } catch (UnableToContinue& e) {
        drain(exprv);
        throw e;
    }

    if (exprv.has_diagnostics()) {
        bool has_errors = exprv.has_errors();
        drain(exprv);
        if (has_errors)
            throw UnableToContinue();
    }


    eval::ConstEvaluator evalr(syms, types);

    eval::Value val;
    try {
        val = init_expr->eval(evalr);
    } catch (InvalidCompileTimeEval& err) {
        err.add_loc(init.loc);
        add_error<InvalidCompileTimeEval>(err);
        throw UnableToContinue();
    } catch (EvalSemanticError& err) {
        err.add_loc(init.loc);
        add_error<EvalSemanticError>(err);
        throw UnableToContinue();
    }

    if (type->is_primitive()) {
        PrimitiveType *primtype = type->as_primitive();
        bool exceeds_limits = false;
        if (primtype->is_bool()) {
            exceeds_limits = val.cast<uint64_t>() > 1;
        } else if (primtype->is_integer() && primtype->is_signed()) {
            // fixme: if val is a float exceeding i64::MAX, this cast is UB
            int64_t v = val.cast<int64_t>();
            // Comparing to an unsigned here is safe, as i64 is guaranteed to fit within u64.
            exceeds_limits = v > static_cast<int64_t>(*primtype->int_max()) || v < *primtype->int_min();
        } else if (primtype->is_integer()) {
            exceeds_limits = val.cast<uint64_t>() > *primtype->int_max();
        } else { // float
            exceeds_limits = std::abs(val.cast<double>()) > *primtype->flt_max();
        }
    
        // todo: warn on float-to-int truncation
    
        if (exceeds_limits) {
            add_error<InvalidConstexprError>(
                InvalidConstexprError::Kind::ExceedsLimits, init_expr->loc);
            throw UnableToContinue();
        }
    
        return val.pr_cast(primtype->get_primkind());
    } else if (type->is_pointer()) {
        return val.cast_to_pointer(type->as_pointer()->stride());
    } else {
        ECC_UNREACHABLE("type passed to this function should only be pointer or primitive");
    }
}

void MIRSynthesizer::do_visit(VariableDeclaration& node) {
    bsv_dbprint("visiting VariableDeclaration node: ", node.loc);

    auto specinfo = parse_speclist(node.specifiers, syms.current);

    ECC_ASSERT(!specinfo.is_constexpr, "visiting VariableDeclaration but specinfo is constexpr");

    Chunk<VarDeclMIR> var_decl = make_chunk<VarDeclMIR>(node.loc, syms.current);

    for (auto& declarator : node.declarators) {
        // call accept on our declarator
        auto initdeclparam = Pair(specinfo.type, specinfo.is_const);
        dv_call(initdeclparam, declarator);

        // take the last result; should be InitDecltrRet
        auto ret = take_last_result<InitDecltrRet>();

        if (!ret.name) {
            add_error<EccSemError>("variable declaration with no name", declarator->loc);
            throw UnableToContinue();
        }

        // we cannot check for completeness here, because type inference is not yet done
        // it has to be checked in the validator.

        Type *symtype = ret.type;

        if (symtype->is_function()) {
            // variable declaration with function type means this is a forward declaration of a
            // function

            if (node.declarators.size() > 1) {
                add_error<EccSemError>(
                    "function declaration cannot be combined with other declarators",
                    declarator->loc);
                throw UnableToContinue();
            }

            if (specinfo.linkage == Linkage::EXTERNAL && syms.current != syms.global()) {
                // reject
                add_error<EccSemError>(
                    "extern function declaration must be at global scope", declarator->loc);
                throw UnableToContinue();
            }

            if (ret.init_mir) {
                add_error<EccSemError>(
                    "function declaration cannot have an initializer", declarator->loc);
                throw UnableToContinue();
            }

            if (syms.current != syms.global()) {
                add_error<EccSemError>(
                    "function declarations must be at global scope", declarator->loc);
                throw UnableToContinue();
            }

            FunctionType *functype = symtype->as_function();

            if (functype->returntype()->is_const()) {
                //todo: return type is const, warn that it is ignored (call expressions are rvalues)
            }

            functype = functype->with_lang_linkage(specinfo.langlink);

            auto funcmir = parse_vardecl_func(node, std::move(ret), specinfo, functype);

            dv_return(funcmir);
        } else {
            if (!syms.current->is_global() && specinfo.linkage == Linkage::EXTERNAL) {
                // non-global external declaration
            }

            InsertVarArgs args = {declarator->loc, *ret.name, symtype, specinfo.linkage};
            args.duration = specinfo.duration;

            VarSymbol *symptr = nullptr;
            try {
                symptr = syms.insert_var(std::move(args));
            } catch (Symbol *existing) {
                add_error<SymbolAlrDecldError>(
                    std::format("symbol {} already previously declared", existing->get_name()), declarator->loc,
                    existing->get_loc());
                throw UnableToContinue();
            }

            // sym should be valid, since to get here builder's name had to exist
            ECC_ASSERT(symptr, "unexpected null pointer when parsing variable declarator");

            // extract the initializer mir
            if (ret.init_mir) {
                Chunk<InitializerMIR> init_mir = std::move(*ret.init_mir);
                var_decl->add_decl(symptr, std::move(init_mir));
            } else {
                var_decl->add_decl(symptr);
            }
        }
    } // end for

    Chunk<DeclMIR> decl = std::move(var_decl);

    for (auto& attr : node.attributes) {
        dv_call(decl.get(), attr);
    }

    dv_return(decl);

}

Chunk<mir::FunctionMIR> MIRSynthesizer::parse_vardecl_func(
    VariableDeclaration& node, InitDecltrRet ret, SpecifierInfo specinfo, FunctionType *type) {

    InsertFuncArgs args = {node.loc, *ret.name, type};
    args.has_body = false;
    args.linkage = specinfo.linkage;

    FuncSymbol *funcptr = nullptr;
    try {
        funcptr = syms.insert_func(std::move(args));
    } catch (Symbol *existing) {
        add_error<SymbolAlrDecldError>(
            std::format("symbol {} already previously declared", existing->get_name()), node.loc,
            existing->get_loc());
        throw UnableToContinue();
    }

    Chunk<FunctionMIR> funcmir =
        make_chunk<FunctionMIR>(node.loc, node.loc, funcptr, syms.current, nullptr, nullptr);

    return funcmir;
}

void MIRSynthesizer::do_visit(InitDeclarator& node) {
    bsv_dbprint("visiting InitDeclarator node: ", node.loc);

    auto param = take_dovisit_param<Pair<types::BaseType *, bool>>();

    dv_call_noparam(node.declarator);
    // pull builder before we visit the initializer
    Box<DeclaratorBuilder> builder = take_last_result<Box<DeclaratorBuilder>>();

    builder->ty_bldr.set_base(param.first, param.second);
    Type *complete = builder->ty_bldr.finalize();

    // todo: construct the variable here instead of at the variable decl node
    if (node.initializer) {
        // call accept on our initializer
        dv_call(complete, *node.initializer);
        auto init_ret = take_last_result<InitializerRet>();
        Type *ret_type;
        if (init_ret.new_type) {
            ret_type = *init_ret.new_type;
        } else {
            ret_type = complete;
        }
        InitDecltrRet ret = {builder->name, ret_type, std::move(init_ret.init_mir)};
        dv_return(ret);
    } else {
        InitDecltrRet ret = {builder->name, complete, {}};
        dv_return(ret);
    }
}

void MIRSynthesizer::do_visit(Declarator& node) {
    Box<DeclaratorBuilder> builder;
    if (node.direct) {
        dv_call_noparam(node.direct.value());
        builder = take_last_result<Box<DeclaratorBuilder>>();
    } else {
        // no direct declarator, assume abstract
        builder = make_box<DeclaratorBuilder>(std::nullopt, types.builder());
    }
    if (node.pointer.has_value()) {
        dv_call(builder.get(), node.pointer.value());
    }

    dv_return(builder);
}

void MIRSynthesizer::do_visit(ParenDeclarator& node) {
    bsv_dbprint("visiting ParenDeclarator node: ", node.loc);
    dv_call_noparam(node.inner);

    auto ret = take_last_result<Box<DeclaratorBuilder>>();
    dv_return(ret);
}

void MIRSynthesizer::do_visit(ArrayDeclarator& node) {
    bsv_dbprint("visiting ArrayDeclarator node: ", node.loc);

    Box<DeclaratorBuilder> builder;
    if (node.base) {
        dv_call_noparam(node.base);
        builder = take_last_result<Box<DeclaratorBuilder>>();
    } else {
        // Abstract array declarator (e.g. `I8 []` as a parameter type): no inner declarator.
        builder = make_box<DeclaratorBuilder>(std::nullopt, types.builder());
    }

    Optional<uint64_t> size{};
    if (node.size) {
        bsv_dbprint("array declarator has size, checking for compile time computability");
        dv_call_noparam(*node.size);
        Value size_val = take_last_result<Value>();

        if (size_val < 0) {
            add_error<EccSemError>("array size cannot be a negative value", (*node.size)->loc);
            throw UnableToContinue();
        }

        size = size_val.cast<uint64_t>();

    } else {
        // fixme: context check here for if size is optional?
    }

    if (size) {
        builder->ty_bldr.add_array(*size);
    } else {
        builder->ty_bldr.add_array();
    }

    dv_return(builder);
}

void MIRSynthesizer::do_visit(FunctionDeclarator& node) {
    bsv_dbprint("visiting FunctionDeclarator node: ", node.loc);

    Box<DeclaratorBuilder> builder;
    if (node.base) {
        dv_call_noparam(node.base);
        builder = take_last_result<Box<DeclaratorBuilder>>();
    } else {
        // Abstract function declarator (e.g. `U0 ()` as a parameter type): no inner declarator.
        builder = make_box<DeclaratorBuilder>(std::nullopt, types.builder());
    }

    Vec<FuncParam> parameters;

    // name pool to check for duplicate parameter names
    StringRefSet name_pool;

    for (auto& param : node.parameters) {
        dv_call_noparam(param);
        FuncParam parsed = take_last_result<FuncParam>();

        if (parsed.name) {
            if (name_pool.contains(*parsed.name)) {
                add_error<EccSemError>(
                    "duplicate parameter name in function declarator", parsed.loc);
                throw UnableToContinue();
            } else {
                name_pool.insert(*parsed.name);
            }
        }
        parameters.push_back(std::move(parsed));
    }

    builder->ty_bldr.add_function(node.loc, parameters, node.is_variadic);

    dv_return(builder);
}

void MIRSynthesizer::do_visit(ParameterDeclaration& node) {
    /*
    dovisit_param: monostate
    last_result: FuncParam
    */
    bsv_dbprint("visiting ParameterDeclarator node: ", node.loc);
    SpecifierInfo specinfo = parse_speclist(node.specifiers, syms.current);

    FuncParam ret;
    if (node.declarator) {
        dv_call_noparam(*node.declarator);
        auto builder = take_last_result<Box<DeclaratorBuilder>>();
        builder->ty_bldr.set_base(specinfo.type, specinfo.is_const);

        Type *final_type = builder->ty_bldr.finalize();
        // If the parameter type is an array, decay it to a pointer (as in C). Route through
        // TypeContext::decay_array so the transient (possibly unsized) ArrayType built here is
        // released instead of lingering in the type context with a live ref count.
        if (final_type->is_array()) {
            final_type = types.decay_array(final_type->as_array());
        }

        if (builder->name) {
            ret = {final_type, builder->name, node.loc, {}};
        } else {
            ret = {final_type, {}, node.loc, {}};
        }
    } else {
        Type *ret_type = specinfo.type;
        if (specinfo.is_const) {
            ret_type = types.get_const(ret_type);
        }
        ret = {ret_type, {}, node.loc, {}};
    }

    if (node.default_value) {
        if (!ret.type->is_primitive() && !ret.type->is_pointer()) {
            add_error<InvalidDefaultParamError>(node.loc, ret.type);
            throw UnableToContinue();
        }

        dv_call_noparam(*node.default_value);
        Value val = take_last_result<Value>();
        if (ret.type->is_primitive()) {
            PrimitiveType *prim = ret.type->as_primitive();
            try {
                val = val.pr_cast(prim->get_primkind());
            } catch (InvalidCompileTimeEval& err) {
                err.add_loc((*node.default_value)->loc);
                add_error<InvalidCompileTimeEval>(err);
                throw UnableToContinue();
            }
        } else if (ret.type->is_pointer()) {
            if (!val.is_pointer()) {
                add_error<InvalidDefaultParamError>(node.loc, val);
                throw UnableToContinue();
            }
            auto *ptr = ret.type->as_pointer();
            if (ptr->get_base()->is_complete()) {
                val = val.cast_to_pointer(ptr->get_base()->alloc_size());
            } else {
                val = val.cast_to_pointer();
            }
        }
        ret.value = val;
    }

    dv_return(ret);
}

void MIRSynthesizer::do_visit(IdentifierDeclarator& node) {
    /*
    Our base case for declarator type building.

    Return a newly constructed declarator builder.
    */
    bsv_dbprint("visiting IdentifierDeclarator node: ", node.loc);

    auto ret = make_box<DeclaratorBuilder>(node.name, types.builder());
    dv_return(ret);
}

void MIRSynthesizer::do_visit(Pointer& node) {
    // dovisit_param: DeclaratorBuilder *
    // last_result: monostate

    bsv_dbprint("visiting Pointer node: ", node.loc);

    auto *builder = take_dovisit_param<DeclaratorBuilder *>();

    bool is_const = false;
    for (auto& qual : node.qualifiers) {
        dv_call_noparam(qual);
        auto qualtype = take_last_result<TypeQualifier::QualType>();
        switch (qualtype) {
        case TypeQualifier::QualType::CONST:
            is_const = true;
            break;
        }
    }

    if (node.nested) {
        dv_call(builder, node.nested.value());
        builder->ty_bldr.add_pointer(is_const);
        dv_return_void();
    } else {
        builder->ty_bldr.add_pointer(is_const);
        dv_return_void();
    }
}

void MIRSynthesizer::do_visit(StorageClassSpecifier& node) {
    bsv_dbprint("visiting StorageClassSpecifier node: ", node.loc);
    /* terminal node */
    dv_return(node.type);
}

void MIRSynthesizer::do_visit(VoidSpecifier& node) {
    bsv_dbprint("visiting VoidSpecifier node: ", node.loc);
    dv_return(types.get_void());
}

void MIRSynthesizer::do_visit(TypeIdentifier& node) {
    bsv_dbprint("visiting TypeIdentifier node: ", node.loc);
    TypeSymbol *typesym = syms.lookup_type(node.identifier);
    if (!typesym) {
        add_error<TypeNotDefinedError>(node.identifier, node.loc);
        throw UnableToContinue();
    }
    dv_return(typesym);
}

void MIRSynthesizer::do_visit(PrimitiveSpecifier& node) {
    bsv_dbprint("visiting PrimitiveSpecifier node: ", node.loc);
    /* terminal node */
    dv_return(types.get_primitive(node.pkind));
}

void MIRSynthesizer::do_visit(TypeQualifier& node) {
    bsv_dbprint("visiting TypeQualifier node: ", node.loc);
    /* terminal node */
    dv_return(node.qual);
}

void MIRSynthesizer::do_visit(EnumSpecifier& node) {
    bsv_dbprint("visiting EnumSpecifier node: ", node.loc);
    EnumType *enm = nullptr;
    try {
        if (node.name) {
            enm = types.get_enum(node.loc, *(node.name), syms.current);
        } else {
            enm = types.get_enum(node.loc, syms.current);
        }
    } catch (UserType *prev_def) {
        add_error<TypeDecldAsOtherError>(
            "enum already declared as another type", node.loc, prev_def->decl_loc);
        throw UnableToContinue();
    }

    Optional<TypeSymbol *> retsym = {};
    // If class has name, compute symbol to add
    if (node.name) {
        bsv_dbprint("enum has name, inserting typesymbol if needed");
        TypeSymbol *enmsym = syms.lookup_type(*node.name, true);
        if (!enmsym) {
            InsertTypeArgs args = {node.loc, *node.name, enm};
            retsym = syms.insert_type(args);
        } else {
            retsym = enmsym;
        }
    }

    TypeSpecRet<EnumType> ret({}, enm);

    if (node.enumerators) {
        if (enm->is_complete()) {
            throw TypeAlrDefinedError("enum was previously defined", node.loc, enm->def_loc);
        }
        if (node.underlying) {
            PrimitiveType *underlying = types.get_primitive(*node.underlying);
            if (!underlying->is_integral()) {
                add_error<InvalidEnumUnderlyingError>(node.loc);
            }
            enm->set_underlying(underlying);
        }

        for (auto& enumtr : *node.enumerators) {
            dv_call(enm, enumtr);
        }

        try {
            enm->finish(node.loc);
        } catch (TypeSemError& e) {
            if (!e.has_loc()) {
                e.add_loc(enm->def_loc);
            }
            add_typesem_error(e.clone());
            throw UnableToContinue();
        }
    }

    dv_return(ret);
}

void MIRSynthesizer::do_visit(Enumerator& node) {
    bsv_dbprint("visiting Enumerator node with name ", node.name);
    EnumType *enm = take_dovisit_param<EnumType *>();

    if (auto *mem = enm->find(node.name)) {
        add_error<EnumeratorAlrDecldError>(
            std::format("symbol \"{}\" previously declared", node.name), node.loc, mem->loc);
        throw UnableToContinue();
    }

    Value value;
    if (node.value) {
        dv_call_noparam(*node.value);
        value = take_last_result<Value>();

        int64_t val = value.cast<int64_t>();

        enm->add_enumerator(node.name, val, node.loc);
    } else {
        value = enm->add_enumerator(node.name, node.loc);
    }

    value = value.pr_cast(enm->get_underlying()->get_primkind());

    InsertVarArgs args = {node.loc, node.name, enm, value};

    try {
        syms.insert_var(std::move(args));
    } catch (Symbol *existing) {
        add_error<EnumeratorSymCollision>(node.loc, existing->get_loc(), node.name);
        throw UnableToContinue();
    }

    dv_return_void();
}

void MIRSynthesizer::do_visit(ClassSpecifier& node) {
    bsv_dbprint("visiting ClassSpecifier node: ", node.loc);

    Scope *declared_scope = syms.current->get_outer();
    ClassType *cls = nullptr;
    try {
        if (node.name) {
            cls = types.get_class(node.loc, *(node.name), declared_scope);
        } else {
            cls = types.get_class(node.loc, declared_scope);
        }
    } catch (UserType *prev_def) {
        add_error<TypeDecldAsOtherError>(
            "class already declared as another type", node.loc, prev_def->decl_loc);
        throw UnableToContinue();
    }

    Optional<TypeSymbol *> retsym = {};
    // If class has name, compute symbol to add
    if (node.name) {
        bsv_dbprint("class has name, inserting typesymbol if needed");
        TypeSymbol *clssym = syms.lookup_type_from(declared_scope, *node.name, true);
        if (!clssym) {
            InsertTypeArgs args = {node.loc, *node.name, cls};
            retsym = syms.insert_type_at(declared_scope, args);
        } else {
            retsym = clssym;
        }
    }

    TypeSpecRet<ClassType> ret(retsym, cls);

    if (node.declarations) {
        if (cls->is_complete()) {
            // error: class was previously defined
            add_error<TypeAlrDefinedError>("class was previously defined", node.loc, cls->def_loc);
            throw UnableToContinue();
        }

        if (node.parents) {
            ECC_ASSERT_N(!(*node.parents).empty());
            if ((*node.parents).size() > 1) {
                add_error<EccSemError>("multiple inheritance is not allowed", node.loc);
            }

            TypeSymbol *parent = syms.lookup_type((*node.parents)[0]);
            if (!parent) {
                add_error<TypeNotDefinedError>("parent class not found", node.loc);
                throw UnableToContinue();
            }

            if (!parent->type->is_class()) {
                add_error<InvalidInheritanceError>(parent->type, node.loc);
                throw UnableToContinue();
            }

            if (!parent->type->as_class()->is_complete()) {
                // We can unwrap the name without worrying about an empty option,
                // since it was looked up by name.
                add_error<IncompleteTypeUseError>(*parent->type->get_name(), node.loc);
                throw UnableToContinue();
            }

            if (cls->has_parent()) {
                add_error<EccSemError>("class parents already specified", node.loc);
                throw UnableToContinue();
            }

            cls->add_parent(parent->type->as_class());
        }

        // class is defined here, populate its members and mark it complete
        for (auto& decl : *node.declarations) {
            dv_call((RecordType *)cls, decl);
        }

        cls->finish(node.loc);
    }

    dv_return(ret);
}

void MIRSynthesizer::do_visit(UnionSpecifier& node) {
    bsv_dbprint("visiting UnionSpecifier node ", node.loc);

    Scope *declared_scope = syms.current->get_outer();
    UnionType *unn = nullptr;
    try {
        if (node.name) {
            unn = types.get_union(node.loc, *(node.name), declared_scope);
        } else {
            unn = types.get_union(node.loc, declared_scope);
        }
    } catch (UserType *prev_def) {
        add_error<TypeDecldAsOtherError>(
            "union already declared as another type", node.loc, prev_def->decl_loc);
        throw UnableToContinue();
    }

    Optional<TypeSymbol *> retsym = {};
    // If class has name, compute symbol to add
    if (node.name) {
        bsv_dbprint("union has name, inserting typesymbol if needed");
        TypeSymbol *unnsym = syms.lookup_type_from(declared_scope, *node.name, true);
        if (!unnsym) {
            InsertTypeArgs args = {node.loc, *node.name, unn};
            // Use syms.current->get_outer(), because UnionSpecifier introduces a new scope,
            // but we need the TypeSymbol to be bound to the outer scope.
            retsym = syms.insert_type_at(declared_scope, args);
        } else {
            retsym = unnsym;
        }
    }

    TypeSpecRet<UnionType> ret(retsym, unn);

    // declarations are present, start definition
    if (node.declarations) {
        if (unn->is_complete()) {
            // error: union was previously defined
            add_error<TypeAlrDefinedError>("union was previously defined", node.loc, unn->def_loc);
            throw UnableToContinue();
        }
        if (node.type_rep) {
            PrimitiveType *typerep = types.get_primitive(*node.type_rep);
            if (!typerep->is_integer()) {
                // todo: issue warning
            }
            unn->set_type_rep(typerep);
        }
        // union is defined here, populate its members and mark it complete
        for (auto& decl : *node.declarations) {
            dv_call((RecordType *)unn, decl);
        }

        try {
            unn->finish(node.loc);
        } catch (TypeSemError& e) {
            if (!e.has_loc()) {
                e.add_loc(unn->def_loc);
            }
            add_typesem_error(e.clone());
            throw UnableToContinue();
        }
    }

    dv_return(ret);
}

void MIRSynthesizer::do_visit(ClassDeclaration& node) {
    bsv_dbprint("visiting ClassDeclaration node: ", node.loc);

    // save our current param, as it may get clobbered while parsing specifiers
    RecordType *recordty   = take_dovisit_param<RecordType *>();
    SpecifierInfo specinfo = parse_speclist(node.specifiers, syms.current);

    if (specinfo.is_constexpr) {
        add_error<EccSemError>("member declarations cannot be marked constexpr", node.loc);
        throw UnableToContinue();
    }

    if (specinfo.linkage != Linkage::NONE) {
        add_error<EccSemError>("member declarations cannot have linkage", node.loc);
        throw UnableToContinue();
    }

    if (recordty->is_class()) {
        bsv_dbprint("parsing ClassDeclaration for ClassType ", recordty->id());
    } else if (recordty->is_union()) {
        bsv_dbprint("parsing ClassDeclaration for UnionType ", recordty->id());
    }

    for (auto& decltr : node.declarators) {
        dv_call_noparam(decltr);
        try {
            std::visit(
                match{
                    [&](Box<DeclaratorBuilder>& builder) {
                        builder->ty_bldr.set_base(specinfo.type, specinfo.is_const);
                        Type *finaltype = builder->ty_bldr.finalize();

                        if (builder->name) {
                            recordty->add_member(*builder->name, finaltype, decltr->loc);
                        } else {
                            recordty->add_member(finaltype, decltr->loc);
                        }
                    },
                    [&](std::monostate&) {
                        // no declarator, use the base type

                        Type *to_add = specinfo.type;

                        if (specinfo.is_const) {
                            to_add = types.get_const(to_add);
                        }
                        recordty->add_member(to_add, decltr->loc);
                    },
                    [&](auto&) {
                        ECC_UNREACHABLE(
                            "unexpected last_result when parsing ClassDeclaration");
                    }},
                last_result);

            last_result = std::monostate{};
        } catch (TypeSemError& e) {
            if (!e.has_loc()) {
                e.add_loc(decltr->loc);
            }
            add_typesem_error(e.clone());
            throw UnableToContinue();
        }
    }

    dv_return_void();
}

void MIRSynthesizer::do_visit(ClassDeclarator& node) {
    bsv_dbprint("visiting ClassDeclarator node: ", node.loc);
    if (node.declarator) {
        dv_call_noparam(node.declarator.value());
        auto ret = take_last_result<Box<DeclaratorBuilder>>();
        dv_return(ret);
    } else {
        dv_return_void();
    }

    // fixme: ignoring bit width for now, implement this when able
}

void MIRSynthesizer::do_visit(Initializer& node) { // NOLINT
    bsv_dbprint("visiting Initializer node: ", node.loc);

    Type *type   = take_dovisit_param<Type *>();
    Location loc = node.loc;

    std::visit(
        match{
            // Base case: single expression
            [&](Chunk<Expression>& expr) {
                bsv_dbprint("visiting single initializer");
                dv_call_noparam(expr);
                Chunk<ExprMIR> exprmir     = take_last_result<Chunk<ExprMIR>>();
                Chunk<InitializerMIR> init = make_chunk<InitializerMIR>(loc, std::move(exprmir));
                InitializerRet ret         = {{}, std::move(init)};
                dv_return(ret);
            },
            [&](Chunk<Initializer::Member>& mem) {
                Type *sub_type = type; // fallback: pass parent type (current behavior)

                if (type->is_class()) {
                    auto *cls    = type->as_class();
                    auto *member = cls->find(mem->member);
                    if (!member) {
                        add_error<EccSemError>(
                            std::format("no member \"{}\" in class", mem->member), loc);
                        throw UnableToContinue();
                    }
                    sub_type = member->ty;
                } else if (type->is_union()) {
                    auto *unn    = type->as_union();
                    auto *member = unn->find(mem->member);
                    if (!member) {
                        add_error<EccSemError>(
                            std::format("no member \"{}\" in union", mem->member), loc);
                        throw UnableToContinue();
                    }
                    sub_type = member->ty;
                }
                // If type is neither class nor union, pass it through; the validator
                // will catch the semantic error when it type-checks the initializer.

                dv_call(sub_type, mem->initializer);

                auto initmir = take_last_result<InitializerRet>();

                Chunk<InitializerMIR> init =
                    make_chunk<InitializerMIR>(loc, mem->member, std::move(initmir.init_mir));

                InitializerRet ret = {initmir.new_type, std::move(init)};
                dv_return(ret);
            },
            [&](Chunk<Initializer::Index>& idx) {
                bsv_dbprint("visiting index designated initializer");

                dv_call_noparam(idx->idx);

                Value new_idx = take_last_result<Value>();

                dv_call(type, idx->initializer);

                auto initmir = take_last_result<InitializerRet>();

                Chunk<InitializerMIR> init =
                    make_chunk<InitializerMIR>(loc, new_idx, std::move(initmir.init_mir));

                InitializerRet ret = {initmir.new_type, std::move(init)};
                dv_return(ret);
            },
            // Recursive case: sub-initializer
            [&](ArenaVec<Chunk<Initializer>>& inits) {
                bsv_dbprint("visiting compound initializer");

                ArenaVec<Chunk<InitializerMIR>> init_mirs{};
                InitializerRet ret{{}, nullptr};

                switch (type->kind) {
                case Type::Kind::ARRAY: {
                    bsv_dbprint("visiting arraytype compound initializer");
                    ArrayType *arrtype = type->as_array();
                    // if there are any subarrays, this is the one pointing to the largest one
                    ArrayType *max_subarray = nullptr;

                    for (auto& init : inits) {
                        // visit each initializer and take the return value
                        dv_call(arrtype->get_base(), init);
                        auto initmir = take_last_result<InitializerRet>();

                        /*
                        If there is a new array type reported by an initializer, we need to
                        propagate that up to our array type.

                        Since sub-array initializers can vary in size (unused spaces remain
                        deinitialized), we take the largest sub-initializer that we encounter.
                        */
                        if (initmir.new_type) {
                            // if we already have a max array set
                            if (max_subarray) {
                                // if the new array is larger than the current max size
                                if ((*initmir.new_type)->get_arr_size() >
                                    max_subarray->get_arr_size()) {
                                    max_subarray = *initmir.new_type;
                                }
                            } else {
                                // else, just set our array
                                max_subarray = *initmir.new_type;
                            }
                        }
                        init_mirs.push_back(std::move(initmir.init_mir));
                    }

                    if (!arrtype->get_arr_size()) {
                        // if no size
                        bsv_dbprint("array has no size, inferring from size of initializer");
                        if (max_subarray) {
                            // if max_subarray was set, use that as our base
                            arrtype = types.set_array_size(max_subarray, inits.size());
                        } else {
                            // otherwise, use our current base
                            arrtype = types.set_array_size(arrtype->get_base(), inits.size());
                        }
                        ret.new_type = arrtype;
                    }
                } break; // end case ARRAY

                case Type::Kind::CLASS: {
                    bsv_dbprint("visiting classtype compound initializer");
                    ClassType *clstype = type->as_class();

                    for (auto&& [idx, init] : std::views::enumerate(inits)) {
                        auto *mem = clstype->find(idx);
                        if (!mem)
                            continue;

                        dv_call(mem->ty, init);
                        auto initmir = take_last_result<InitializerRet>();
                        // ignore new array type here, since arrays in classes must have declared
                        // size
                        init_mirs.push_back(std::move(initmir.init_mir));
                    }
                } break; // end case CLASS

                default: {
                    add_error<InvalidInitializerError>(
                        // fixme: better error
                        "cannot initialize a variable that is not class or array with compound "
                        "initializer",
                        node.loc);
                    throw UnableToContinue();
                }
                }

                Chunk<InitializerMIR> fullinit =
                    make_chunk<InitializerMIR>(loc, std::move(init_mirs));
                ret.init_mir = std::move(fullinit);
                dv_return(ret);
            }},
        node.initializer);
}

void MIRSynthesizer::do_visit(TypeName& node) {
    // dovisit_param: monostate
    // last_result: Type *
    SpecifierInfo specinfo = parse_speclist(node.specifiers, syms.current);

    if (node.declarator) {
        dv_call_noparam(*node.declarator);
        auto builder = take_last_result<Box<DeclaratorBuilder>>();
        builder->ty_bldr.set_base(specinfo.type, specinfo.is_const);

        Type *finaltype = builder->ty_bldr.finalize();

        dv_return(finaltype);
    } else {

        Type *ret = specinfo.type;
        if (specinfo.is_const) {
            ret = types.get_const(ret);
        }
        dv_return(ret);
    }
}

void MIRSynthesizer::do_visit(CompoundStatement& node) {
    bsv_dbprint("visiting CompoundStatement node: ", node.loc);

    ArenaVec<Chunk<ProgItemMIR>> progitems{};
    for (auto& item : node.items) {
        dv_call_noparam(item);
        std::visit(
            match{
                [&](Chunk<DeclMIR>& decl) mutable { progitems.push_back(std::move(decl)); },
                [&](Chunk<StmtMIR>& stmt) mutable { progitems.push_back(std::move(stmt)); },
                [&](Chunk<FunctionMIR>& func) mutable { progitems.push_back(std::move(func)); },
                [](std::monostate&) {
                    // ignore and continue
                },
                [](auto&) {
                    ECC_UNREACHABLE("unexpected type while parsing program items");
                }},
            last_result);
        last_result = std::monostate{};
    }

    Chunk<StmtMIR> cmpdmir = 
        make_chunk<CompoundStmtMIR>(
            node.loc, std::move(progitems), syms.current->get_outer());

    dv_return(cmpdmir);
}

void MIRSynthesizer::do_visit(ExpressionStatement& node) {
    bsv_dbprint("visiting ExpressionStatement node: ", node.loc);
    using MNK = MIRNode::NodeKind;
    if (node.expression) {
        dv_call_noparam(*node.expression);
        auto expr = take_last_result<Chunk<ExprMIR>>();

        switch (expr->kind) {
        // If the internal expression is a string literal expression, emit a PrintStatement
        case MNK::LITEXPR_MIR: {
            auto *litexpr = dyncast<LiteralExprMIR>(expr.get());
            ECC_ASSERT(litexpr, "could not cast LITEXPR_MIR to LiteralExprMIR");

            if (auto *str = std::get_if<StringRef>(&litexpr->value)) {
                bsv_dbprint(
                    "found string literal inside ExpressionStatement, emitting PrintStmtMIR");
                Chunk<StmtMIR> stmt = make_chunk<PrintStmtMIR>(node.loc, *str, syms.current);
                dv_return(stmt);
            }
            break;
        }

        // if the internal expression is an identifier with type function, and function has no
        // params, emit a call to that function instead
        case MNK::IDENTEXPR_MIR: {
            auto *idexpr = dyncast<IdentExprMIR>(expr.get());
            ECC_ASSERT(idexpr, "could not cast IDENTEXPR_MIR to IdentExprMIR");
            if (idexpr->ident->get_type()->is_function()) {
                auto *idtype = idexpr->ident->get_type()->as_function();
                ECC_ASSERT(idtype, "type with kind TypeKind::Function is not FunctionType");
                if (idtype->no_params()) {
                    bsv_dbprint(
                        "found identexpr of type function with no params, emitting CallExprMIR");
                    ArenaVec<Chunk<ExprMIR>> empty_args{};
                    expr = make_chunk<CallExprMIR>(
                        node.loc, syms.current, std::move(expr), std::move(empty_args));
                }
            }
            break;
        }

        default:
            break;
        }
        Chunk<StmtMIR> stmt = make_chunk<ExprStmtMIR>(node.loc, std::move(expr));
        dv_return(stmt);

    } else {
        Chunk<StmtMIR> stmt = make_chunk<ExprStmtMIR>(node.loc, syms.current);
        dv_return(stmt);
    }
}

void MIRSynthesizer::do_visit(CaseStatement& node) {
    bsv_dbprint("visiting CaseStatement node: ", node.loc);
    dv_call_noparam(node.case_expr);
    Value case_val = take_last_result<Value>();

    dv_call_noparam(node.statement);
    Chunk<StmtMIR> stmt = take_last_result<Chunk<StmtMIR>>();

    Chunk<StmtMIR> casestmt = make_chunk<CaseStmtMIR>(node.loc, case_val, std::move(stmt));

    dv_return(casestmt);
}

void MIRSynthesizer::do_visit(CaseRangeStatement& node) {
    bsv_dbprint("visiting CaseRangeStatement node: ", node.loc);

    dv_call_noparam(node.range_start);
    Value case_start = take_last_result<Value>();

    dv_call_noparam(node.range_end);
    Value case_end = take_last_result<Value>();

    dv_call_noparam(node.statement);
    Chunk<StmtMIR> stmt = take_last_result<Chunk<StmtMIR>>();

    Chunk<StmtMIR> casestmt =
        make_chunk<CaseRangeStmtMIR>(node.loc, case_start, case_end, std::move(stmt));

    dv_return(casestmt);
}

void MIRSynthesizer::do_visit(DefaultStatement& node) {
    bsv_dbprint("visiting DefaultStatement node: ", node.loc);
    dv_call_noparam(node.statement);
    Chunk<StmtMIR> stmt = take_last_result<Chunk<StmtMIR>>();

    Chunk<StmtMIR> defstmt = make_chunk<DefaultStmtMIR>(node.loc, std::move(stmt));

    dv_return(defstmt);
}

void MIRSynthesizer::do_visit(LabeledStatement& node) {
    bsv_dbprint("visiting LabeledStatement node: ", node.loc);

    InsertLabelArgs args = {node.loc, node.label};
    LabelSymbol *labelptr; 
    try {
        labelptr = syms.insert_label(args);
    } catch (Symbol *existing) {
        add_error<LabelAlrDefinedError>(node.label, node.loc, existing->get_loc());
        throw UnableToContinue();
    }
    dv_call_noparam(node.statement);

    auto stmt = take_last_result<Chunk<StmtMIR>>();

    Chunk<StmtMIR> ret = make_chunk<LabeledStmtMIR>(node.loc, labelptr, std::move(stmt));
    dv_return(ret);
}

void MIRSynthesizer::do_visit(PrintStatement& node) {
    bsv_dbprint("visiting PrintStatement node: ", node.loc);

    ArenaVec<Chunk<ExprMIR>> exprs{};
    exprs.reserve(node.arguments.size());

    for (auto& arg : node.arguments) {
        dv_call_noparam(arg);
        Chunk<ExprMIR> argmir = take_last_result<Chunk<ExprMIR>>();
        exprs.push_back(std::move(argmir));
    }

    Chunk<StmtMIR> printstmt =
        make_chunk<PrintStmtMIR>(node.loc, node.format_string, syms.current, std::move(exprs));

    dv_return(printstmt);
}

void MIRSynthesizer::do_visit(IfStatement& node) {
    bsv_dbprint("visiting IfStatement node: ", node.loc);
    dv_call_noparam(node.condition);
    Chunk<ExprMIR> cond = take_last_result<Chunk<ExprMIR>>();

    dv_call_noparam(node.then_branch);
    Chunk<StmtMIR> then_br = take_last_result<Chunk<StmtMIR>>();

    Optional<Chunk<StmtMIR>> else_br;
    if (node.else_branch.has_value()) {
        dv_call_noparam(node.else_branch.value());
        else_br = take_last_result<Chunk<StmtMIR>>();
    }

    Chunk<StmtMIR> ifstmt =
        make_chunk<IfStmtMIR>(node.loc, std::move(cond), std::move(then_br), std::move(else_br));

    dv_return(ifstmt);
}

void MIRSynthesizer::do_visit(SwitchStatement& node) {
    bsv_dbprint("visiting SwitchStatement node: ", node.loc);
    dv_call_noparam(node.condition);
    Chunk<ExprMIR> cond = take_last_result<Chunk<ExprMIR>>();

    dv_call_noparam(node.body);
    Chunk<StmtMIR> stmt = take_last_result<Chunk<StmtMIR>>();

    Chunk<StmtMIR> switchst = make_chunk<SwitchStmtMIR>(node.loc, std::move(cond), std::move(stmt));

    dv_return(switchst);
}

void MIRSynthesizer::do_visit(WhileStatement& node) {
    bsv_dbprint("visiting WhileStatement node: ", node.loc);

    dv_call_noparam(node.condition);
    Chunk<ExprMIR> cond = take_last_result<Chunk<ExprMIR>>();
    dv_call_noparam(node.body);
    Chunk<StmtMIR> body = take_last_result<Chunk<StmtMIR>>();

    // Create the actual loop
    Chunk<StmtMIR> loop =
        make_chunk<LoopStmtMIR>(node.loc, std::move(cond), std::move(body), false);

    dv_return(loop);
}

void MIRSynthesizer::do_visit(DoWhileStatement& node) {
    bsv_dbprint("visiting DoWhileStatement node: ", node.loc);

    dv_call_noparam(node.condition);
    Chunk<ExprMIR> cond = take_last_result<Chunk<ExprMIR>>();
    dv_call_noparam(node.body);
    Chunk<StmtMIR> body = take_last_result<Chunk<StmtMIR>>();

    Chunk<StmtMIR> loop = make_chunk<LoopStmtMIR>(node.loc, std::move(cond), std::move(body), true);

    dv_return(loop);
}

void MIRSynthesizer::do_visit(ForStatement& node) {
    bsv_dbprint("visiting ForStatement node: ", node.loc);

    Chunk<LoopStmtMIR> loop = make_chunk<LoopStmtMIR>(node.loc, syms.current);

    syms.push_scope(); // introduce an implicit scope for the init variable

    if (node.init) {
        std::visit(
            match{
                [&](Chunk<Expression>& expr) {
                    dv_call_noparam(expr);
                    Chunk<ExprMIR> exprmir = take_last_result<Chunk<ExprMIR>>();
                    Chunk<ExprStmtMIR> exprstmt =
                        make_chunk<ExprStmtMIR>(expr->loc, std::move(exprmir));

                    loop->init = std::move(exprstmt);
                },
                [&](Chunk<VariableDeclaration>& decl) {
                    dv_call_noparam(decl);
                    Chunk<DeclMIR> declmir = take_last_result<Chunk<DeclMIR>>();
                    loop->init             = std::move(declmir);
                }},
            *node.init);
    }

    if (node.condition) {
        dv_call_noparam(node.condition.value());
        Chunk<ExprMIR> cond = take_last_result<Chunk<ExprMIR>>();
        loop->condition     = std::move(cond);
    }

    dv_call_noparam(node.body);
    Chunk<StmtMIR> body = take_last_result<Chunk<StmtMIR>>();

    loop->body = std::move(body);

    if (node.increment) {
        dv_call_noparam(node.increment.value());
        Chunk<ExprMIR> step_expr = take_last_result<Chunk<ExprMIR>>();
        Chunk<StmtMIR> step_stmt = make_chunk<ExprStmtMIR>(step_expr->loc, std::move(step_expr));

        loop->step = std::move(step_stmt);
    }

    syms.pop_scope();

    Chunk<StmtMIR> stmt = std::move(loop);
    dv_return(stmt);
}

void MIRSynthesizer::do_visit(GotoStatement& node) {
    bsv_dbprint("visiting GotoStatement node: ", node.loc);

    Chunk<StmtMIR> stmt = make_chunk<GotoStmtMIR>(node.loc, node.target_label, syms.current);
    dv_return(stmt);
}

void MIRSynthesizer::do_visit(BreakStatement& node) {
    bsv_dbprint("visiting BreakStatement node: ", node.loc);

    Chunk<StmtMIR> stmt = make_chunk<BreakStmtMIR>(node.loc, syms.current);
    dv_return(stmt);
}

void MIRSynthesizer::do_visit(ContinueStatement& node) {
    bsv_dbprint("visiting ContinueStatement node: ", node.loc);

    Chunk<StmtMIR> stmt = make_chunk<ContStmtMIR>(node.loc, syms.current);
    dv_return(stmt);
}

void MIRSynthesizer::do_visit(ReturnStatement& node) {
    bsv_dbprint("visiting ReturnStatement node: ", node.loc);

    Chunk<ReturnStmtMIR> retstmt = make_chunk<ReturnStmtMIR>(node.loc, syms.current);

    if (node.return_value) {
        dv_call_noparam(*node.return_value);
        Chunk<ExprMIR> return_value = take_last_result<Chunk<ExprMIR>>();
        retstmt->ret_expr           = std::move(return_value);
    }

    Chunk<StmtMIR> stmt = std::move(retstmt);
    dv_return(stmt);
}

void MIRSynthesizer::do_visit(BinaryExpression& node) {
    bsv_dbprint("visiting BinaryExpression node: ", node.loc);

    dv_call_noparam(node.left);
    Chunk<ExprMIR> left = take_last_result<Chunk<ExprMIR>>();
    dv_call_noparam(node.right);
    Chunk<ExprMIR> right = take_last_result<Chunk<ExprMIR>>();

    Chunk<ExprMIR> expr = make_chunk<BinaryExprMIR>(
        node.loc, syms.current, std::move(left), std::move(right), node.op);

    dv_return(expr);
}

void MIRSynthesizer::do_visit(UnaryExpression& node) {
    bsv_dbprint("visiting UnaryExpression node: ", node.loc);
    dv_call_noparam(node.operand);
    Chunk<ExprMIR> operand = take_last_result<Chunk<ExprMIR>>();

    Chunk<ExprMIR> expr =
        make_chunk<UnaryExprMIR>(node.loc, syms.current, std::move(operand), node.op);

    dv_return(expr);
}

void MIRSynthesizer::do_visit(CastExpression& node) {
    bsv_dbprint("visiting CastExpression node: ", node.loc);
    dv_call_noparam(node.inner);
    Chunk<ExprMIR> inner = take_last_result<Chunk<ExprMIR>>();
    dv_call_noparam(node.type_name);
    Type *target = take_last_result<Type *>();

    Chunk<ExprMIR> expr = make_chunk<CastExprMIR>(node.loc, syms.current, target, std::move(inner));

    dv_return(expr);
}

void MIRSynthesizer::do_visit(AssignmentExpression& node) {
    bsv_dbprint("visiting AssignmentExpression node: ", node.loc);
    dv_call_noparam(node.left);
    Chunk<ExprMIR> left = take_last_result<Chunk<ExprMIR>>();
    dv_call_noparam(node.right);
    Chunk<ExprMIR> right = take_last_result<Chunk<ExprMIR>>();

    Chunk<ExprMIR> expr = make_chunk<AssignExprMIR>(
        node.loc, syms.current, std::move(left), std::move(right), node.op);

    dv_return(expr);
}

void MIRSynthesizer::do_visit(ConditionalExpression& node) {
    bsv_dbprint("visiting ConditionalExpression node: ", node.loc);
    dv_call_noparam(node.condition);
    Chunk<ExprMIR> condition = take_last_result<Chunk<ExprMIR>>();
    dv_call_noparam(node.true_expr);
    Chunk<ExprMIR> true_expr = take_last_result<Chunk<ExprMIR>>();
    dv_call_noparam(node.false_expr);
    Chunk<ExprMIR> false_expr = take_last_result<Chunk<ExprMIR>>();

    Chunk<ExprMIR> expr = make_chunk<CondExprMIR>(
        node.loc, syms.current, std::move(condition), std::move(true_expr), std::move(false_expr));

    dv_return(expr);
}

void MIRSynthesizer::do_visit(IdentifierExpression& node) {
    bsv_dbprint("visiting IdentifierExpression node: ", node.loc);

    Symbol *sym = syms.lookup(node.name);
    if (!sym) {
        add_error<IdentNotDefinedError>(node.name, node.loc);
        throw UnableToContinue();
    }
    if (sym->is_abstract()) {
        add_error<InvalidIdentifierError>(node.name, node.loc);
        throw UnableToContinue();
    }

    PhysicalSymbol *physsym = sym->as_physical();
    ECC_ASSERT_N(physsym);

    Chunk<ExprMIR> expr = make_chunk<IdentExprMIR>(node.loc, syms.current, physsym);

    dv_return(expr);
}

void MIRSynthesizer::do_visit(ConstExpression& node) {
    bsv_dbprint("visiting ConstExpression node: ", node.loc);
    dv_call_noparam(node.inner);
    Chunk<ExprMIR> inner = take_last_result<Chunk<ExprMIR>>();

    ExprValidator exprv(types, syms);
    try {
        inner->accept(exprv);
    } catch (UnableToContinue& e) {
        drain(exprv);
        throw e;
    }

    if (exprv.has_diagnostics()) {
        bool has_errors = exprv.has_errors();
        drain(exprv);
        if (has_errors)
            throw UnableToContinue();
    }

    ConstEvaluator evalr(syms, types);
    Value res;
    try {
        res = inner->eval(evalr);
    } catch (InvalidCompileTimeEval& e) {
        e.add_loc(node.loc);
        add_error<InvalidCompileTimeEval>(e);
        throw UnableToContinue();
    }

    dv_return(res);
}

void MIRSynthesizer::do_visit(LiteralExpression& node) {
    bsv_dbprint("visiting LiteralExpression node: ", node.loc);

    Value val;
    switch (node.kind) {
    case LiteralExpression::INT:
        bsv_dbprint("found integer");
        val = Value::from_literal(std::get<uint64_t>(node.value));
        break;

    case LiteralExpression::FLOAT:
        bsv_dbprint("found float");
        val = Value::from_literal(std::get<double>(node.value));
        break;

    case LiteralExpression::CHAR:
        bsv_dbprint("found char");
        val = Value::from_literal(std::get<char>(node.value));
        break;

    case LiteralExpression::BOOL:
        bsv_dbprint("found bool");
        val = Value::from_literal(std::get<bool>(node.value));
        break;
    }

    Chunk<ExprMIR> expr = make_chunk<LiteralExprMIR>(node.loc, syms.current, val);

    dv_return(expr);
}

void MIRSynthesizer::do_visit(StringExpression& node) {
    bsv_dbprint("visiting StringExpression node: ", node.loc);

    Chunk<ExprMIR> expr = make_chunk<LiteralExprMIR>(node.loc, syms.current, node.value);

    dv_return(expr);
}

void MIRSynthesizer::do_visit(NullptrExpression& node) {
    bsv_dbprint("visiting NullptrExpression node: ", node.loc);

    Chunk<ExprMIR> expr = make_chunk<LiteralExprMIR>(node.loc, syms.current);

    dv_return(expr);
}

void MIRSynthesizer::do_visit(CallExpression& node) {
    bsv_dbprint("visiting CallExpression node: ", node.loc);
    dv_call_noparam(node.callee);
    Chunk<ExprMIR> callee = take_last_result<Chunk<ExprMIR>>();

    ArenaVec<Chunk<ExprMIR>> args;
    for (auto& arg : node.arguments) {
        dv_call_noparam(arg);
        Chunk<ExprMIR> argument = take_last_result<Chunk<ExprMIR>>();
        args.push_back(std::move(argument));
    }

    Chunk<ExprMIR> call =
        make_chunk<CallExprMIR>(node.loc, syms.current, std::move(callee), std::move(args));

    dv_return(call);
}

void MIRSynthesizer::do_visit(MemberAccessExpression& node) {
    bsv_dbprint("visiting MemberAccessExpression node: ", node.loc);

    dv_call_noparam(node.object);
    Chunk<ExprMIR> object = take_last_result<Chunk<ExprMIR>>();

    Chunk<ExprMIR> expr = make_chunk<MemberAccExprMIR>(
        node.loc, syms.current, std::move(object), node.member, node.is_arrow);

    dv_return(expr);
}

void MIRSynthesizer::do_visit(ReinterpretExpression& node) {
    bsv_dbprint("visiting ReinterpretExpression node: ", node.loc);

    dv_call_noparam(node.object);
    Chunk<ExprMIR> object = take_last_result<Chunk<ExprMIR>>();

    Chunk<ExprMIR> expr = make_chunk<ReintExprMIR>(
        node.loc, syms.current, std::move(object), node.target, node.is_arrow);

    dv_return(expr);
}

void MIRSynthesizer::do_visit(ArraySubscriptExpression& node) {
    bsv_dbprint("visiting ArraySubscriptExpression node: ", node.loc);

    dv_call_noparam(node.array);
    Chunk<ExprMIR> array = take_last_result<Chunk<ExprMIR>>();

    dv_call_noparam(node.index);
    Chunk<ExprMIR> index = take_last_result<Chunk<ExprMIR>>();

    Chunk<ExprMIR> expr =
        make_chunk<SubscrExprMIR>(node.loc, syms.current, std::move(array), std::move(index));

    dv_return(expr);
}

void MIRSynthesizer::do_visit(PostfixExpression& node) {
    bsv_dbprint("visiting PostfixExpression node: ", node.loc);

    dv_call_noparam(node.operand);
    Chunk<ExprMIR> operand = take_last_result<Chunk<ExprMIR>>();

    Chunk<ExprMIR> expr =
        make_chunk<PostfixExprMIR>(node.loc, syms.current, std::move(operand), node.op);

    dv_return(expr);
}

void MIRSynthesizer::do_visit(SizeofExpression& node) {
    bsv_dbprint("visiting SizeofExpression node: ", node.loc);

    Chunk<SizeofExprMIR> sizexpr = make_chunk<SizeofExprMIR>(node.loc, syms.current);
    std::visit(
        match{
            [&](Chunk<Expression>& expr) mutable {
                // this might be a literal expression, so we defer
                // resolution of the actual type to validation.
                dv_call_noparam(expr);
                Chunk<ExprMIR> target = take_last_result<Chunk<ExprMIR>>();
                sizexpr->operand      = std::move(target);
            },
            [&](Chunk<TypeName>& typen) mutable {
                dv_call_noparam(typen);
                Type *target     = take_last_result<Type *>();
                sizexpr->operand = target;
            }},
        node.operand);

    Chunk<ExprMIR> expr = std::move(sizexpr);
    dv_return(expr);
}