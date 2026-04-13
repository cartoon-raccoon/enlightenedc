#include "semantics/mir/synthesizer.hpp"

#include <cassert>
#include <memory>
#include <stdexcept>
#include <variant>

#include "ast/ast.hpp"
#include "error.hpp"
#include "eval/consteval.hpp"
#include "eval/value.hpp"
#include "semantics/mir/mir.hpp"
#include "semantics/semerr.hpp"
#include "semantics/symbols.hpp"
#include "semantics/typeerr.hpp"
#include "semantics/types.hpp"
#include "util.hpp"

#define dv_return(val)                \
    do {                              \
        last_result = std::move(val); \
        return;                       \
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

Box<MIRSynthesizer::SpecifierInfo>
MIRSynthesizer::parse_speclist(Vec<Box<ast::DeclarationSpecifier>>& speclist, Location loc) {
    using NK = ASTNode::NodeKind;

    bsv_dbprint("parsing declaration specifier list for node at ", loc);
    Box<SpecifierInfo> specinfo = std::make_unique<SpecifierInfo>();

    for (auto& decl_spec : speclist) {
        decl_spec->accept(*this);
        switch (decl_spec->kind) {
        case NK::TYPE_QUAL: {
            auto qualtype = take_last_result<TypeQualifier::QualType>();
            switch (qualtype) {
            case TypeQualifier::QualType::CONST:
                if (specinfo->is_const) {
                    add_error<EccSemError>("duplicate const qualifier", decl_spec->loc);
                } else {
                    specinfo->is_const = true;
                }
            }
        } break;

        case NK::STORAGE_SPEC: {
            auto spectype = take_last_result<StorageClassSpecifier::SpecType>();
            switch (spectype) {
            case StorageClassSpecifier::PUBLIC:
                if (specinfo->is_public) {
                    add_error<EccSemError>("duplicate public specifier", decl_spec->loc);
                } else {
                    specinfo->is_public = true;
                }
                break;

            case StorageClassSpecifier::STATIC:
                if (specinfo->is_static) {
                    add_error<EccSemError>("duplicate static specifier", decl_spec->loc);
                } else {
                    specinfo->is_static = true;
                }
                break;

            case StorageClassSpecifier::EXTERN:
                if (specinfo->linkage != PhysicalSymbol::Linkage::INTERNAL) {
                    add_error<EccSemError>("multiple storage class specifiers", decl_spec->loc);
                } else {
                    specinfo->linkage = PhysicalSymbol::Linkage::EXTERNAL;
                }
                break;

            case StorageClassSpecifier::EXTERNC:
                if (specinfo->linkage != PhysicalSymbol::Linkage::INTERNAL) {
                    add_error<EccSemError>("multiple storage class specifiers", decl_spec->loc);
                } else {
                    specinfo->linkage = PhysicalSymbol::Linkage::EXTERNC;
                }
                break;
            }
        } break;

        case NK::TYPE_IDENT: {
            specinfo->symbol = take_last_result<TypeSymbol *>();
            // No need to check for nullptr here because it is guaranteed,
            // if the ident didn't exist it would have thrown
            specinfo->type = (*specinfo->symbol)->type;
            break;
        }

        case NK::CLASS_SPEC: {
            auto typespecret = take_last_result<TypeSpecRet<ClassType>>();
            specinfo->type   = typespecret.type;
            specinfo->symbol = typespecret.symbol;
            break;
        }

        case NK::UNION_SPEC: {
            auto typespecret = take_last_result<TypeSpecRet<UnionType>>();
            specinfo->type   = typespecret.type;
            specinfo->symbol = typespecret.symbol;
            break;
        }

        case NK::ENUM_SPEC: {
            auto typespecret = take_last_result<TypeSpecRet<EnumType>>();
            specinfo->type   = typespecret.type;
            specinfo->symbol = typespecret.symbol;
            break;
        }

        case NK::VOID_SPEC:
            specinfo->type = take_last_result<VoidType *>();
            break;

        case NK::PRIM_SPEC:
            specinfo->type = take_last_result<PrimitiveType *>();
            break;

        default:
            throw std::runtime_error(
                "encountered a non-declaration specifier while parsing specifiers");
        }
    }
    assert(specinfo->type);
    bsv_dbprint("finished parsing specifiers for node ", loc);

    return std::move(specinfo);
}

void MIRSynthesizer::do_visit(Program& node) {
    bsv_dbprint("visiting Program node: ", node.loc);

    for (auto& item : node.items) {
        dv_call(std::monostate{}, item);
        std::visit(
            match{[&](Box<DeclMIR>& decl) mutable { prog_mir.add_item(std::move(decl)); },
                  [&](Box<StmtMIR>& stmt) mutable { prog_mir.add_item(std::move(stmt)); },
                  [&](Box<FunctionMIR>& func) mutable { prog_mir.add_item(std::move(func)); },
                  [&](std::monostate& mono) {
                      // ignore and continue
                  },
                  [&](auto& err) {
                      throw std::runtime_error("unexpected item while parsing programitems");
                  }},
            last_result);
        last_result = std::monostate{};
    }

    dv_return(std::monostate{});
}

void MIRSynthesizer::do_visit(Function& node) {
    /*
    1. Create function signature:
    - Return type from the decl spec list
    - Construct pointer from return type if declarator has pointer
    - Parameters from declarator (check type of declarator)

    2. Create function type with TypeContext

    3. Create function symbol with the signature

    4. Construct parameter for call to function body (CompoundStatement)
    5. Call accept on node.body

    */

    bsv_dbprint("visiting Function node: ", node.loc);

    // Parse and construct specifier info
    ElabVisitParam param        = std::move(dovisit_param);
    Box<SpecifierInfo> specinfo = parse_speclist(node.decl_spec_list, node.loc);
    dovisit_param               = std::move(param);

    if (specinfo->linkage != PhysicalSymbol::Linkage::INTERNAL) {
        add_error<EccSemError>("externally linked functions cannot have a body", node.loc);
        throw UnableToContinue();
    }

    BaseType *return_base = specinfo->type;

    if (!node.declarator->direct) {
        add_error<EccSemError>("function declaration but missing direct declarator",
                               node.declarator->loc);
        throw UnableToContinue();
    }
    if (node.declarator->direct.value()->kind != ASTNode::FUNC_DECLTR) {
        add_error<EccSemError>("function declaration but declarator is not function",
                               node.declarator->loc);
        throw UnableToContinue();
    }

    // Visit the Declarator to construct the type builder.
    dv_call(std::monostate{}, node.declarator);
    auto builder = take_last_result<Box<DeclaratorBuilder>>();
    builder->ty_bldr.set_base(return_base);

    // The latest function parameters.
    Vec<FuncParam> last_func_params;

    // Extract the base type from our builder.
    Type *curr = builder->ty_bldr.base;

    while (!builder->ty_bldr.type_stack.empty()) {
        auto next_cstrctr = builder->ty_bldr.type_stack.top();
        std::visit(match{[&](TypeBuilder::Arr& arr) mutable {
                             // Wrap the base in an array.
                             if (arr.size) {
                                 curr = builder->ty_bldr.ctxt().get_array(curr, *arr.size);
                             } else {
                                 curr = builder->ty_bldr.ctxt().get_array(curr);
                             }
                         },
                         [&](TypeBuilder::Ptr& ptr) mutable {
                             // Wrap the base in a pointer.
                             curr = builder->ty_bldr.ctxt().get_pointer(curr, ptr.is_const);
                         },
                         [&](TypeBuilder::FnParams& fn) mutable {
                             // map out the identifiers.
                             Vec<Type *> params;
                             params.reserve(fn.params.size());
                             for (auto& param : fn.params) {
                                 params.push_back(param.type);
                             }

                             last_func_params = std::move(fn.params);
                             // Wrap the base as the return type in a function type.
                             curr = builder->ty_bldr.ctxt().get_function(
                                 fn.loc, curr, std::move(params), fn.variadic);
                         }},
                   next_cstrctr);

        // Pop the stack to the next constructor
        builder->ty_bldr.type_stack.pop();
    }

    FunctionType *functype = curr->as_function();
    if (!functype) {
        add_error<EccSemError>("unable to resolve declarator to function declarator", node.loc);
        throw UnableToContinue();
    }
    if (!builder->name) {
        add_error<EccSemError>("unable to parse name from declarator", node.declarator->loc);
        throw UnableToContinue();
    }

    dbprint("parsing params");

    Vec<Box<VarSymbol>> params;
    for (FuncParam& param : last_func_params) {
        if (!param.name) {
            add_error<EccSemError>("parameter in function declaration has no name", param.loc);
        }
        Box<VarSymbol> paramsym =
            std::make_unique<VarSymbol>(param.loc, *param.name, syms.current, param.type);
        paramsym->is_funcparam = true;
        params.push_back(std::move(paramsym));
    }

    Vec<VarSymbol *> paramsym_ptrs{};
    for (Box<VarSymbol>& sym : params) {
        paramsym_ptrs.push_back(sym.get());
    }

    Box<FuncSymbol> symbol = std::make_unique<FuncSymbol>(node.loc, *builder->name, syms.current,
                                                          functype, std::move(paramsym_ptrs));
    FuncSymbol *sym_ptr    = symbol.get();
    try {
        sym_ptr = syms.insert(*builder->name, std::move(symbol));
    } catch (Symbol *previous) {
        add_error<SymbolAlrDecldError>(
            std::format("function \"{}\" was previously declared", sym_ptr->name), sym_ptr->loc,
            previous->loc);
        throw UnableToContinue();
    }

    CmpdStmtDoVisitParam cmpdstmtp({sym_ptr, std::move(params)});

    // Then make call
    dv_call(cmpdstmtp, node.body);

    auto res = take_last_result<CmpdStmtFromFuncRes>();

    Box<FunctionMIR> func =
        std::make_unique<FunctionMIR>(node.loc, sym_ptr, res.second, std::move(res.first));
    dv_return(func);
}

void MIRSynthesizer::do_visit(TypeDeclaration& node) {
    bsv_dbprint("visiting TypeDeclaration node: ", node.loc);

    auto specinfo = parse_speclist(node.specifiers, node.loc);

    if (specinfo->symbol) {
        TypeSymbol *symptr = (*specinfo->symbol);
        Box<DeclMIR> decl  = std::make_unique<TypeDeclMIR>(node.loc, symptr);
        dv_return(decl);
    } else {
        dv_return(std::monostate{});
    }
}

void MIRSynthesizer::do_visit(VariableDeclaration& node) {
    bsv_dbprint("visiting VariableDeclaration node: ", node.loc);

    auto specinfo = parse_speclist(node.specifiers, node.loc);

    Box<VarDeclMIR> var_decl = std::make_unique<VarDeclMIR>(node.loc);

    for (auto& declarator : node.declarators) {
        // call accept on our declarator
        dv_call(specinfo->type, declarator);

        // take the last result; should be InitDecltrRet
        auto ret = take_last_result<InitDecltrRet>();

        // declare symbol and corresponding pointer
        Box<VarSymbol> sym = nullptr;
        VarSymbol *symptr  = nullptr;
        if (ret.name) {
            // initialize our symbol and its pointer
            sym = std::make_unique<VarSymbol>(declarator->loc, *ret.name, syms.current, ret.type);
            symptr = sym.get();

            // populate other specifiers, and then insert into symbol table
            sym->is_public = specinfo->is_public;
            sym->is_static = specinfo->is_static;
            sym->is_const  = specinfo->is_const;
            sym->linkage   = specinfo->linkage;

            if (sym->is_external() && syms.current != syms.global()) {
                add_error<EccSemError>("extern variable declaration must be at global scope",
                                       declarator->loc);
                throw UnableToContinue();
            }

            syms.insert(*ret.name, std::move(sym));
        } else {
            add_error<EccSemError>("variable declaration with no name", declarator->loc);
            throw UnableToContinue();
        }

        // sym should be valid, since to get here builder's name had to exist
        if (!symptr) {
            add_error<EccSemError>("unexpected null pointer when parsing variable declarator",
                                   declarator->loc);
            throw UnableToContinue();
        }

        // extract the initializer mir
        if (ret.init_mir) {
            Box<InitializerMIR> init_mir = std::move(*ret.init_mir);
            var_decl->add_decl(symptr, std::move(init_mir));
        } else {
            var_decl->add_decl(symptr);
        }
    }

    Box<DeclMIR> decl = std::move(var_decl);

    dv_return(decl);
}

void MIRSynthesizer::do_visit(InitDeclarator& node) {
    bsv_dbprint("visiting InitDeclarator node: ", node.loc);

    BaseType *base = take_dovisit_param<BaseType *>();

    dv_call(std::monostate{}, node.declarator);
    // pull builder before we visit the initializer
    Box<DeclaratorBuilder> builder = take_last_result<Box<DeclaratorBuilder>>();

    builder->ty_bldr.set_base(base);
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
        InitDecltrRet ret = {std::move(builder->name), ret_type, std::move(init_ret.init_mir)};
        dv_return(ret);
    } else {
        InitDecltrRet ret = {std::move(builder->name), complete, {}};
        dv_return(ret);
    }
}

void MIRSynthesizer::do_visit(Declarator& node) {
    Box<DeclaratorBuilder> builder;
    if (node.direct) {
        dv_call(std::monostate{}, node.direct.value());
        builder = take_last_result<Box<DeclaratorBuilder>>();
    } else {
        // no direct declarator, assume abstract
        builder = std::make_unique<DeclaratorBuilder>(std::nullopt, types.builder());
    }
    if (node.pointer.has_value()) {
        dv_call(builder.get(), node.pointer.value());
    }

    dv_return(builder);
}

void MIRSynthesizer::do_visit(ParenDeclarator& node) {
    bsv_dbprint("visiting ParenDeclarator node: ", node.loc);
    dv_call(std::monostate{}, node.inner);

    dv_return(take_last_result<Box<DeclaratorBuilder>>());
}

void MIRSynthesizer::do_visit(ArrayDeclarator& node) {
    bsv_dbprint("visiting ArrayDeclarator node: ", node.loc);
    dv_call(std::monostate{}, node.base);

    auto builder = take_last_result<Box<DeclaratorBuilder>>();

    Optional<uint64_t> size{};
    if (node.size) {
        bsv_dbprint("array declarator has size, checking for compile time computability");
        dv_call(std::monostate{}, *node.size);
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
    dv_call(std::monostate{}, node.base);
    auto builder = take_last_result<Box<DeclaratorBuilder>>();

    Vec<FuncParam> parameters;
    for (auto& param : node.parameters) {
        dv_call(std::monostate{}, param);
        FuncParam parsed = take_last_result<FuncParam>();
        parameters.push_back(parsed);
    }

    builder->ty_bldr.add_function(node.base->loc, parameters, node.is_variadic);

    dv_return(builder);
}

void MIRSynthesizer::do_visit(ParameterDeclaration& node) {
    /*
    dovisit_param: monostate
    last_result: FuncParam
    */
    bsv_dbprint("visiting ParameterDeclarator node: ", node.loc);
    Box<SpecifierInfo> specinfo = parse_speclist(node.specifiers, node.loc);

    FuncParam ret;
    if (node.declarator) {
        dv_call(std::monostate{}, *node.declarator);
        auto builder = take_last_result<Box<DeclaratorBuilder>>();
        builder->ty_bldr.set_base(specinfo->type);

        Type *final_type = builder->ty_bldr.finalize();
        if (builder->name) {
            ret = {final_type, builder->name, node.loc, specinfo->is_const};
        } else {
            ret = {final_type, {}, node.loc, specinfo->is_const};
        }
    } else {
        ret = {specinfo->type, {}};
    }

    if (node.default_value) {
        // default value should only be present when parsing a declarator in a Function node
        // fixme: ignoring default values for now, implement this
    }

    dv_return(ret);
}

void MIRSynthesizer::do_visit(IdentifierDeclarator& node) {
    /*
    Our base case for declarator type building.

    Return a newly constructed declarator builder.
    */
    bsv_dbprint("visiting IdentifierDeclarator node: ", node.loc);
    dv_return(std::make_unique<DeclaratorBuilder>(node.name, types.builder()));
}

void MIRSynthesizer::do_visit(Pointer& node) {
    // dovisit_param: DeclaratorBuilder *
    // last_result: monostate

    bsv_dbprint("visiting Pointer node: ", node.loc);

    auto *builder = take_dovisit_param<DeclaratorBuilder *>();

    bool is_const = false;
    for (auto& qual : node.qualifiers) {
        dv_call(std::monostate{}, qual);
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
        dv_return(std::monostate{});
    } else {
        builder->ty_bldr.add_pointer(is_const);
        dv_return(std::monostate{});
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
        add_error<TypeDecldAsOtherError>("enum already declared as another type", node.loc,
                                         prev_def->decl_loc);
        throw UnableToContinue();
    }

    Optional<TypeSymbol *> retsym = {};
    // If class has name, compute symbol to add
    if (node.name) {
        bsv_dbprint("enum has name, inserting typesymbol if needed");
        TypeSymbol *enmsym = syms.lookup_type(*node.name, true);
        if (!enmsym) {
            Box<TypeSymbol> sym =
                std::make_unique<sym::TypeSymbol>(node.loc, *node.name, syms.current, enm);
            retsym = sym.get();
            syms.insert(*node.name, std::move(sym));
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
            enm->underlying = underlying;
        }

        for (auto& enumtr : *node.enumerators) {
            dv_call(enm, enumtr);
        }

        enm->finish(node.loc);
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
        dv_call(std::monostate{}, *node.value);
        value = take_last_result<Value>();

        int64_t val = value.cast<int64_t>();

        enm->add_enumerator(node.name, val, node.loc);
    } else {
        value = enm->add_enumerator(node.name, node.loc);
    }

    syms.insert(node.name,
                std::make_unique<VarSymbol>(node.loc, node.name, syms.current, enm, value));

    dv_return(std::monostate{});
}

void MIRSynthesizer::do_visit(ClassSpecifier& node) {
    bsv_dbprint("visiting ClassSpecifier node: ", node.loc);
    ClassType *cls = nullptr;
    try {
        if (node.name) {
            cls = types.get_class(node.loc, *(node.name), syms.current);
        } else {
            cls = types.get_class(node.loc, syms.current);
        }
    } catch (UserType *prev_def) {
        add_error<TypeDecldAsOtherError>("class already declared as another type", node.loc,
                                         prev_def->decl_loc);
        throw UnableToContinue();
    }

    Optional<TypeSymbol *> retsym = {};
    // If class has name, compute symbol to add
    if (node.name) {
        bsv_dbprint("class has name, inserting typesymbol if needed");
        TypeSymbol *clssym = syms.lookup_type(*node.name, true);
        if (!clssym) {
            Box<TypeSymbol> sym =
                std::make_unique<sym::TypeSymbol>(node.loc, *node.name, syms.current, cls);
            retsym = sym.get();
            syms.insert(*node.name, std::move(sym));
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

        // class is defined here, populate its members and mark it complete
        for (auto& decl : *node.declarations) {
            dv_call(cls, decl);
        }

        cls->finish(node.loc);
    }

    dv_return(ret);
}

void MIRSynthesizer::do_visit(UnionSpecifier& node) {
    bsv_dbprint("visiting UnionSpecifier node ", node.loc);
    UnionType *unn = nullptr;
    try {
        if (node.name) {
            unn = types.get_union(node.loc, *(node.name), syms.current);
        } else {
            unn = types.get_union(node.loc, syms.current);
        }
    } catch (UserType *prev_def) {
        add_error<TypeDecldAsOtherError>("union already declared as another type", node.loc,
                                         prev_def->decl_loc);
        throw UnableToContinue();
    }

    Optional<TypeSymbol *> retsym = {};
    // If class has name, compute symbol to add
    if (node.name) {
        bsv_dbprint("union has name, inserting typesymbol if needed");
        TypeSymbol *unnsym = syms.lookup_type(*node.name, true);
        if (!unnsym) {
            Box<TypeSymbol> sym =
                std::make_unique<sym::TypeSymbol>(node.loc, *node.name, syms.current, unn);
            retsym = sym.get();
            syms.insert(*node.name, std::move(sym));
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
            unn->type_rep = typerep;
        }
        // union is defined here, populate its members and mark it complete
        for (auto& decl : *node.declarations) {
            dv_call(unn, decl);
        }

        unn->finish(node.loc);
    }

    dv_return(ret);
}

void MIRSynthesizer::do_visit(ClassDeclaration& node) {
    bsv_dbprint("visiting ClassDeclaration node: ", node.loc);

    // save our current param, as it may get clobbered while parsing specifiers
    ElabVisitParam param        = std::move(dovisit_param);
    Box<SpecifierInfo> specinfo = parse_speclist(node.specifiers, node.loc);

    // restore our current param
    dovisit_param = std::move(param);

    std::visit(
        match{// ClassType case
              [&](ClassType *cls) {
                  bsv_dbprint("parsing ClassDeclaration for ClassType ", cls);
                  for (auto& decltr : node.declarators) {
                      dv_call(std::monostate{}, decltr);
                      try {
                          std::visit(
                              match{[&](Box<DeclaratorBuilder>& builder) {
                                        builder->ty_bldr.set_base(specinfo->type);
                                        Type *finaltype = builder->ty_bldr.finalize();

                                        if (builder->name) {
                                            cls->add_member(*builder->name, finaltype, decltr->loc);
                                        } else {
                                            cls->add_member(finaltype, decltr->loc);
                                        }
                                    },
                                    [&](std::monostate& mono) {
                                        // no declarator, use the base type
                                        cls->add_member(specinfo->type, decltr->loc);
                                    },
                                    [&](auto& err) {
                                        throw std::runtime_error(
                                            "unexpected last_result when parsing ClassDeclaration");
                                    }},
                              last_result);

                          last_result = std::monostate{};
                      } catch (TypeSemError& e) {
                          add_error<TypeSemError>(e);
                          throw UnableToContinue();
                      }
                  }
              },

              // UnionType case
              [&specinfo, &node, this](UnionType *unn) {
                  bsv_dbprint("parsing ClassDeclaration for UnionType ", unn);
                  for (auto& decltr : node.declarators) {
                      dv_call(std::monostate{}, decltr);
                      try {
                          std::visit(
                              match{[&](Box<DeclaratorBuilder>& builder) {
                                        builder->ty_bldr.set_base(specinfo->type);
                                        Type *finaltype = builder->ty_bldr.finalize();

                                        if (builder->name) {
                                            unn->add_member(*builder->name, finaltype, decltr->loc);
                                        } else {
                                            unn->add_member(finaltype, decltr->loc);
                                        }
                                    },
                                    [&](std::monostate& mono) {
                                        // no declarator, use the base type
                                        unn->add_member(specinfo->type, decltr->loc);
                                    },
                                    [&](auto& err) {
                                        throw std::runtime_error(
                                            "unexpected last_result when parsing UnionDeclaration");
                                    }},
                              last_result);

                          last_result = std::monostate{};
                      } catch (TypeSemError& e) {
                          add_error<TypeSemError>(e);
                          throw UnableToContinue();
                      }
                  }
              },

              [&node, this](auto& err) {
                  bsv_dbprint(typeid(err).name(), " ", node.loc);
                  throw std::runtime_error(
                      "unexpected dovisit param when parsing ClassDeclaration");
              }},
        param);

    dv_return(std::monostate{});
}

void MIRSynthesizer::do_visit(ClassDeclarator& node) {
    bsv_dbprint("visiting ClassDeclarator node: ", node.loc);
    if (node.declarator) {
        dv_call(std::monostate{}, node.declarator.value());
        dv_return(take_last_result<Box<DeclaratorBuilder>>());
    } else {
        dv_return(std::monostate{});
    }

    // fixme: ignoring bit width for now, implement this when able
}

void MIRSynthesizer::do_visit(Initializer& node) { // NOLINT
    bsv_dbprint("visiting Initializer node: ", node.loc);

    Type *type   = take_dovisit_param<Type *>();
    Location loc = node.loc;

    std::visit(
        match{// Base case: single expression
              [&](Box<Expression>& expr) {
                  bsv_dbprint("visiting single initializer");
                  dv_call(std::monostate{}, expr);
                  Box<ExprMIR> exprmir = take_last_result<Box<ExprMIR>>();
                  Box<InitializerMIR> init =
                      std::make_unique<InitializerMIR>(loc, std::move(exprmir));
                  InitializerRet ret = {{}, std::move(init)};
                  dv_return(ret);
              },
              // Recursive case: sub-initializer
              [&](Vec<Box<Initializer>>& inits) {
                  bsv_dbprint("visiting compound initializer");

                  Vec<Box<InitializerMIR>> init_mirs{};
                  InitializerRet ret{{}, nullptr};

                  switch (type->kind) {
                  case Type::Kind::ARRAY: {
                      bsv_dbprint("visiting arraytype compound initializer");
                      ArrayType *arrtype = type->as_array();
                      // if there are any subarrays, this is the one pointing to the largest one
                      ArrayType *max_subarray = nullptr;

                      for (auto& init : inits) {
                          // visit each initializer and take the return value
                          dv_call(arrtype->base, init);
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
                                  if ((*initmir.new_type)->arr_size > max_subarray->arr_size) {
                                      max_subarray = *initmir.new_type;
                                  }
                              } else {
                                  // else, just set our array
                                  max_subarray = *initmir.new_type;
                              }
                          }
                          init_mirs.push_back(std::move(initmir.init_mir));
                      }

                      if (!arrtype->arr_size) {
                          // if no size
                          bsv_dbprint("array has no size, inferring from size of initializer");
                          if (max_subarray) {
                              // if max_subarray was set, use that as our base
                              arrtype = types.set_array_size(max_subarray, inits.size());
                          } else {
                              // otherwise, use our current base
                              arrtype = types.set_array_size(arrtype->base, inits.size());
                          }
                          ret.new_type = arrtype;
                      }
                  } break; // end case ARRAY

                  case Type::Kind::CLASS: {
                      bsv_dbprint("visiting classtype compound initializer");
                      ClassType *clstype = type->as_class();

                      for (int i = 0; i < inits.size(); i++) {
                          auto& init                      = inits[i];
                          ClassType::ClassTypeMember *mem = clstype->index(i);
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

                  Box<InitializerMIR> fullinit =
                      std::make_unique<InitializerMIR>(loc, std::move(init_mirs));
                  ret.init_mir = std::move(fullinit);
                  dv_return(ret);
              }},
        node.initializer);
}

void MIRSynthesizer::do_visit(TypeName& node) {
    // dovisit_param: monostate
    // last_result: Type *
    Box<SpecifierInfo> specinfo = parse_speclist(node.specifiers, node.loc);

    if (node.declarator) {
        dv_call(std::monostate{}, *node.declarator);
        auto builder = take_last_result<Box<DeclaratorBuilder>>();
        builder->ty_bldr.set_base(specinfo->type);

        Type *finaltype = builder->ty_bldr.finalize();

        dv_return(finaltype);
    } else {
        dv_return(specinfo->type);
    }
}

void MIRSynthesizer::do_visit(CompoundStatement& node) {
    bsv_dbprint("visiting CompoundStatement node: ", node.loc);
    CmpdStmtDoVisitParam add_symbols;

    // There are three possible states for the params for CmpdStmt:
    // the param is CmpdStmtDoVisitParam and has a value
    // the param is CmpdStmtDoVisitParam and has no value
    // the param is some other type
    // Since do_visit on this node can be called outside of functions,
    // having the param not be of the expected type is valid,
    // we just don't use it.
    std::visit(match{[&](CmpdStmtDoVisitParam& param) mutable {
                         // dbprint("CmpdStmtDoVisitParams found");
                         add_symbols = std::move(param);
                     },
                     [&](auto& nothing) mutable {
                         // dbprint("No params found");
                         add_symbols = {};
                     }},
               dovisit_param);

    // Reset dovisit_param to monostate
    dovisit_param = std::monostate{};

    if (add_symbols) {
        bsv_dbprint("CmpdStmtDoVisitParams found to have value, checking for function");
        // check the immediate outer node is a function
        if (in_node(ASTNode::NodeKind::FUNC) != 1) {
            throw std::runtime_error("received symbols to add when not in function");
        }

        // Tie the function symbol to our current scope
        syms.tie_current_to(add_symbols.value().first);

        // Add function argument symbols to our current scope
        for (auto& sym : add_symbols.value().second) {
            syms.insert(sym->name, std::move(sym));
        }
    }

    Vec<Box<ProgItemMIR>> progitems{};
    for (auto& item : node.items) {
        dv_call(std::monostate{}, item);
        std::visit(
            match{[&](Box<DeclMIR>& decl) mutable { progitems.push_back(std::move(decl)); },
                  [&](Box<StmtMIR>& stmt) mutable { progitems.push_back(std::move(stmt)); },
                  [&](Box<FunctionMIR>& func) mutable { progitems.push_back(std::move(func)); },
                  [](std::monostate& mono) {
                      // ignore and continue
                  },
                  [](auto& err) {
                      throw std::runtime_error("unexpected type while parsing program items");
                  }},
            last_result);
        last_result = std::monostate{};
    }

    // resolve our return value
    if (add_symbols) {
        // we had add_symbols, so we were called from function
        Box<CompoundStmtMIR> cmpdmir =
            std::make_unique<CompoundStmtMIR>(node.loc, std::move(progitems));
        ElabResult ret = std::pair(std::move(cmpdmir), syms.current);

        dv_return(ret);
    } else {

        Box<StmtMIR> cmpdmir = std::make_unique<CompoundStmtMIR>(node.loc, std::move(progitems));

        dv_return(cmpdmir);
    }
}

void MIRSynthesizer::do_visit(ExpressionStatement& node) {
    bsv_dbprint("visiting ExpressionStatement node: ", node.loc);
    using MNK = MIRNode::NodeKind;
    if (node.expression) {
        dv_call(std::monostate{}, *node.expression);
        auto expr = take_last_result<Box<ExprMIR>>();

        switch (expr->kind) {
        // If the internal expression is a string literal expression, emit a PrintStatement
        case MNK::LITEXPR_MIR: {
            auto *litexpr = dynamic_cast<LiteralExprMIR *>(expr.get());
            if (!litexpr) {
                throw std::runtime_error("could not cast LITEXPR_MIR to LiteralExprMIR");
            }
            if (auto *str = std::get_if<std::string>(&litexpr->value)) {
                bsv_dbprint(
                    "found string literal inside ExpressionStatement, emitting PrintStmtMIR");
                std::string format_string = std::move(*str);
                Box<StmtMIR> stmt =
                    std::make_unique<PrintStmtMIR>(node.loc, std::move(format_string));
                dv_return(stmt);
            }
            break;
        }

        // if the internal expression is an identifier with type function, and function has no
        // params, emit a call to that function instead
        case MNK::IDENTEXPR_MIR: {
            auto *idexpr = dynamic_cast<IdentExprMIR *>(expr.get());
            if (!idexpr) {
                throw std::runtime_error("could not cast IDENTEXPR_MIR to IdentExprMIR");
            }
            if (idexpr->ident->get_type()->is_function()) {
                auto *idtype = idexpr->ident->get_type()->as_function();
                if (!idtype) {
                    throw std::runtime_error(
                        "type with kind TypeKind::Function is not FunctionType");
                }
                if (idtype->no_params()) {
                    bsv_dbprint(
                        "found identexpr of type function with no params, emitting CallExprMIR");
                    Vec<Box<ExprMIR>> empty_args{};
                    expr = std::make_unique<CallExprMIR>(node.loc, syms.current, std::move(expr),
                                                         std::move(empty_args));
                }
            }
            break;
        }

        default:
            break;
        }
        Box<StmtMIR> stmt = std::make_unique<ExprStmtMIR>(node.loc, std::move(expr));
        dv_return(stmt);

    } else {
        Box<StmtMIR> stmt = std::make_unique<ExprStmtMIR>(node.loc);
        dv_return(stmt);
    }
}

void MIRSynthesizer::do_visit(CaseStatement& node) {
    bsv_dbprint("visiting CaseStatement node: ", node.loc);
    dv_call(std::monostate{}, node.case_expr);
    Value case_val = take_last_result<Value>();

    dv_call(std::monostate{}, node.statement);
    Box<StmtMIR> stmt = take_last_result<Box<StmtMIR>>();

    Box<StmtMIR> casestmt = std::make_unique<CaseStmtMIR>(node.loc, case_val, std::move(stmt));

    dv_return(casestmt);
}

void MIRSynthesizer::do_visit(CaseRangeStatement& node) {
    bsv_dbprint("visiting CaseRangeStatement node: ", node.loc);

    dv_call(std::monostate{}, node.range_start);
    Value case_start = take_last_result<Value>();

    dv_call(std::monostate{}, node.range_end);
    Value case_end = take_last_result<Value>();

    dv_call(std::monostate{}, node.statement);
    Box<StmtMIR> stmt = take_last_result<Box<StmtMIR>>();

    Box<StmtMIR> casestmt =
        std::make_unique<CaseRangeStmtMIR>(node.loc, case_start, case_end, std::move(stmt));

    dv_return(casestmt);
}

void MIRSynthesizer::do_visit(DefaultStatement& node) {
    bsv_dbprint("visiting DefaultStatement node: ", node.loc);
    dv_call(std::monostate{}, node.statement);
    Box<StmtMIR> stmt = take_last_result<Box<StmtMIR>>();

    Box<StmtMIR> defstmt = std::make_unique<DefaultStmtMIR>(node.loc, std::move(stmt));

    dv_return(defstmt);
}

void MIRSynthesizer::do_visit(LabeledStatement& node) {
    bsv_dbprint("visiting LabeledStatement node: ", node.loc);
    Box<LabelSymbol> label = std::make_unique<sym::LabelSymbol>(node.loc, node.label, syms.current);
    LabelSymbol *labelptr  = label.get();
    syms.insert(node.label, std::move(label));
    dv_call(std::monostate{}, node.statement);

    auto stmt = take_last_result<Box<StmtMIR>>();

    Box<StmtMIR> ret = std::make_unique<LabeledStmtMIR>(node.loc, labelptr, std::move(stmt));
    dv_return(ret);
}

void MIRSynthesizer::do_visit(PrintStatement& node) {
    bsv_dbprint("visiting PrintStatement node: ", node.loc);

    Vec<Box<ExprMIR>> exprs{};
    exprs.reserve(node.arguments.size());

    for (auto& arg : node.arguments) {
        dv_call(std::monostate{}, arg);
        Box<ExprMIR> argmir = take_last_result<Box<ExprMIR>>();
        exprs.push_back(std::move(argmir));
    }

    Box<StmtMIR> printstmt =
        std::make_unique<PrintStmtMIR>(node.loc, node.format_string, std::move(exprs));

    dv_return(printstmt);
}

void MIRSynthesizer::do_visit(IfStatement& node) {
    bsv_dbprint("visiting IfStatement node: ", node.loc);
    dv_call(std::monostate{}, node.condition);
    Box<ExprMIR> cond = take_last_result<Box<ExprMIR>>();

    dv_call(std::monostate{}, node.then_branch);
    Box<StmtMIR> then_br = take_last_result<Box<StmtMIR>>();

    Optional<Box<StmtMIR>> else_br;
    if (node.else_branch.has_value()) {
        dv_call(std::monostate{}, node.else_branch.value());
        else_br = std::move(take_last_result<Box<StmtMIR>>());
    }

    Box<StmtMIR> ifstmt = std::make_unique<IfStmtMIR>(node.loc, std::move(cond), std::move(then_br),
                                                      std::move(else_br));

    dv_return(ifstmt);
}

void MIRSynthesizer::do_visit(SwitchStatement& node) {
    bsv_dbprint("visiting SwitchStatement node: ", node.loc);
    dv_call(std::monostate{}, node.condition);
    Box<ExprMIR> cond = take_last_result<Box<ExprMIR>>();

    dv_call(std::monostate{}, node.body);
    Box<StmtMIR> stmt = take_last_result<Box<StmtMIR>>();

    Box<StmtMIR> switchst =
        std::make_unique<SwitchStmtMIR>(node.loc, std::move(cond), std::move(stmt));

    dv_return(switchst);
}

void MIRSynthesizer::do_visit(WhileStatement& node) {
    bsv_dbprint("visiting WhileStatement node: ", node.loc);

    dv_call(std::monostate{}, node.condition);
    Box<ExprMIR> cond = take_last_result<Box<ExprMIR>>();
    dv_call(std::monostate{}, node.body);
    Box<StmtMIR> body = take_last_result<Box<StmtMIR>>();

    // Create the actual loop
    Box<StmtMIR> loop =
        std::make_unique<LoopStmtMIR>(node.loc, std::move(cond), std::move(body), false);

    dv_return(loop);
}

void MIRSynthesizer::do_visit(DoWhileStatement& node) {
    bsv_dbprint("visiting DoWhileStatement node: ", node.loc);

    dv_call(std::monostate{}, node.condition);
    Box<ExprMIR> cond = take_last_result<Box<ExprMIR>>();
    dv_call(std::monostate{}, node.body);
    Box<StmtMIR> body = take_last_result<Box<StmtMIR>>();

    Box<StmtMIR> loop =
        std::make_unique<LoopStmtMIR>(node.loc, std::move(cond), std::move(body), true);

    dv_return(loop);
}

void MIRSynthesizer::do_visit(ForStatement& node) {
    bsv_dbprint("visiting ForStatement node: ", node.loc);

    dv_call(std::monostate{}, node.body);
    Box<StmtMIR> body = take_last_result<Box<StmtMIR>>();

    Box<LoopStmtMIR> loop = std::make_unique<LoopStmtMIR>(node.loc, std::move(body));

    if (node.init.has_value()) {
        std::visit(match{[&](Box<Expression>& expr) {
                             dv_call(std::monostate{}, expr);
                             Box<ExprMIR> exprmir = take_last_result<Box<ExprMIR>>();
                             Box<ExprStmtMIR> exprstmt =
                                 std::make_unique<ExprStmtMIR>(expr->loc, std::move(exprmir));

                             loop->init = std::move(exprstmt);
                         },
                         [&](Box<VariableDeclaration>& decl) {
                             dv_call(std::monostate{}, decl);
                             Box<DeclMIR> declmir = take_last_result<Box<DeclMIR>>();
                             loop->init           = std::move(declmir);
                         }},
                   *node.init);
    }

    if (node.condition.has_value()) {
        dv_call(std::monostate{}, node.condition.value());
        Box<ExprMIR> cond = take_last_result<Box<ExprMIR>>();
        loop->condition   = std::move(cond);
    }

    if (node.increment.has_value()) {
        dv_call(std::monostate{}, node.increment.value());
        Box<ExprMIR> step_expr = take_last_result<Box<ExprMIR>>();
        Box<StmtMIR> step_stmt =
            std::make_unique<ExprStmtMIR>(step_expr->loc, std::move(step_expr));

        loop->step = std::move(step_stmt);
    }

    Box<StmtMIR> stmt = std::move(loop);
    dv_return(stmt);
}

void MIRSynthesizer::do_visit(GotoStatement& node) {
    bsv_dbprint("visiting GotoStatement node: ", node.loc);

    Box<StmtMIR> stmt = std::make_unique<GotoStmtMIR>(node.loc, node.target_label);
    dv_return(stmt);
}

void MIRSynthesizer::do_visit(BreakStatement& node) {
    bsv_dbprint("visiting BreakStatement node: ", node.loc);

    Box<StmtMIR> stmt = std::make_unique<BreakStmtMIR>(node.loc);
    dv_return(stmt);
}

void MIRSynthesizer::do_visit(ContinueStatement& node) {
    bsv_dbprint("visiting ContinueStatement node: ", node.loc);

    Box<StmtMIR> stmt = std::make_unique<ContStmtMIR>(node.loc);
    dv_return(stmt);
}

void MIRSynthesizer::do_visit(ReturnStatement& node) {
    bsv_dbprint("visiting ReturnStatement node: ", node.loc);

    Box<ReturnStmtMIR> retstmt = std::make_unique<ReturnStmtMIR>(node.loc);

    if (node.return_value) {
        dv_call(std::monostate{}, *node.return_value);
        Box<ExprMIR> return_value = take_last_result<Box<ExprMIR>>();
        retstmt->ret_expr         = std::move(return_value);
    }

    Box<StmtMIR> stmt = std::move(retstmt);
    dv_return(stmt);
}

void MIRSynthesizer::do_visit(BinaryExpression& node) {
    bsv_dbprint("visiting BinaryExpression node: ", node.loc);

    dv_call(std::monostate{}, node.left);
    Box<ExprMIR> left = take_last_result<Box<ExprMIR>>();
    dv_call(std::monostate{}, node.right);
    Box<ExprMIR> right = take_last_result<Box<ExprMIR>>();

    Box<ExprMIR> expr = std::make_unique<BinaryExprMIR>(node.loc, syms.current, std::move(left),
                                                        std::move(right), node.op);

    dv_return(expr);
}

void MIRSynthesizer::do_visit(UnaryExpression& node) {
    bsv_dbprint("visiting UnaryExpression node: ", node.loc);
    dv_call(std::monostate{}, node.operand);
    Box<ExprMIR> operand = take_last_result<Box<ExprMIR>>();

    Box<ExprMIR> expr =
        std::make_unique<UnaryExprMIR>(node.loc, syms.current, std::move(operand), node.op);

    dv_return(expr);
}

void MIRSynthesizer::do_visit(CastExpression& node) {
    bsv_dbprint("visiting CastExpression node: ", node.loc);
    dv_call(std::monostate{}, node.inner);
    Box<ExprMIR> inner = take_last_result<Box<ExprMIR>>();
    dv_call(std::monostate{}, node.type_name);
    Type *target = take_last_result<Type *>();

    Box<ExprMIR> expr =
        std::make_unique<CastExprMIR>(node.loc, syms.current, target, std::move(inner));

    dv_return(expr);
}

void MIRSynthesizer::do_visit(AssignmentExpression& node) {
    bsv_dbprint("visiting AssignmentExpression node: ", node.loc);
    dv_call(std::monostate{}, node.left);
    Box<ExprMIR> left = take_last_result<Box<ExprMIR>>();
    dv_call(std::monostate{}, node.right);
    Box<ExprMIR> right = take_last_result<Box<ExprMIR>>();

    Box<ExprMIR> expr = std::make_unique<AssignExprMIR>(node.loc, syms.current, std::move(left),
                                                        std::move(right), node.op);

    dv_return(expr);
}

void MIRSynthesizer::do_visit(ConditionalExpression& node) {
    bsv_dbprint("visiting ConditionalExpression node: ", node.loc);
    dv_call(std::monostate{}, node.condition);
    Box<ExprMIR> condition = take_last_result<Box<ExprMIR>>();
    dv_call(std::monostate{}, node.true_expr);
    Box<ExprMIR> true_expr = take_last_result<Box<ExprMIR>>();
    dv_call(std::monostate{}, node.false_expr);
    Box<ExprMIR> false_expr = take_last_result<Box<ExprMIR>>();

    Box<ExprMIR> expr = std::make_unique<CondExprMIR>(node.loc, syms.current, std::move(condition),
                                                      std::move(true_expr), std::move(false_expr));

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
    assert(physsym);

    Box<ExprMIR> expr = std::make_unique<IdentExprMIR>(node.loc, syms.current, physsym);

    dv_return(expr);
}

void MIRSynthesizer::do_visit(ConstExpression& node) {
    bsv_dbprint("visiting ConstExpression node: ", node.loc);
    dv_call(std::monostate{}, node.inner);
    Box<ExprMIR> inner = take_last_result<Box<ExprMIR>>();

    ConstEvaluator evalr(syms, types);
    Value res;
    try {
        res = inner->eval(evalr);
    } catch (InvalidCompileTimeEval& e) {
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
        val = node.value.i_val;
        break;

    case LiteralExpression::FLOAT:
        val = node.value.f_val;
        break;

    case LiteralExpression::CHAR:
        val = node.value.c_val;
        break;

    case LiteralExpression::BOOL:
        val = node.value.b_val;
        break;
    }

    Box<ExprMIR> expr = std::make_unique<LiteralExprMIR>(node.loc, syms.current, val);

    dv_return(expr);
}

void MIRSynthesizer::do_visit(StringExpression& node) {
    bsv_dbprint("visiting StringExpression node: ", node.loc);

    Box<ExprMIR> expr = std::make_unique<LiteralExprMIR>(node.loc, syms.current, node.value);

    dv_return(expr);
}

void MIRSynthesizer::do_visit(CallExpression& node) {
    bsv_dbprint("visiting CallExpression node: ", node.loc);
    dv_call(std::monostate{}, node.callee);
    Box<ExprMIR> callee = take_last_result<Box<ExprMIR>>();

    Vec<Box<ExprMIR>> args;
    for (auto& arg : node.arguments) {
        dv_call(std::monostate{}, arg);
        Box<ExprMIR> argument = take_last_result<Box<ExprMIR>>();
        args.push_back(std::move(argument));
    }

    Box<ExprMIR> call =
        std::make_unique<CallExprMIR>(node.loc, syms.current, std::move(callee), std::move(args));

    dv_return(call);
}

void MIRSynthesizer::do_visit(MemberAccessExpression& node) {
    bsv_dbprint("visiting MemberAccessExpression node: ", node.loc);

    dv_call(std::monostate{}, node.object);
    Box<ExprMIR> object = take_last_result<Box<ExprMIR>>();

    Box<ExprMIR> expr = std::make_unique<MemberAccExprMIR>(
        node.loc, syms.current, std::move(object), node.member, node.is_arrow);

    dv_return(expr);
}

void MIRSynthesizer::do_visit(ArraySubscriptExpression& node) {
    bsv_dbprint("visiting ArraySubscriptExpression node: ", node.loc);

    dv_call(std::monostate{}, node.array);
    Box<ExprMIR> array = take_last_result<Box<ExprMIR>>();

    dv_call(std::monostate{}, node.index);
    Box<ExprMIR> index = take_last_result<Box<ExprMIR>>();

    Box<ExprMIR> expr =
        std::make_unique<SubscrExprMIR>(node.loc, syms.current, std::move(array), std::move(index));

    dv_return(expr);
}

void MIRSynthesizer::do_visit(PostfixExpression& node) {
    bsv_dbprint("visiting PostfixExpression node: ", node.loc);

    dv_call(std::monostate{}, node.operand);
    Box<ExprMIR> operand = take_last_result<Box<ExprMIR>>();

    Box<ExprMIR> expr =
        std::make_unique<PostfixExprMIR>(node.loc, syms.current, std::move(operand), node.op);

    dv_return(expr);
}

void MIRSynthesizer::do_visit(SizeofExpression& node) {
    bsv_dbprint("visiting SizeofExpression node: ", node.loc);

    Box<SizeofExprMIR> sizexpr = std::make_unique<SizeofExprMIR>(node.loc, syms.current);
    std::visit(match{[&](Box<Expression>& expr) mutable {
                         // this might be a literal expression, so we defer
                         // resolution of the actual type to validation.
                         dv_call(std::monostate{}, expr);
                         Box<ExprMIR> target = take_last_result<Box<ExprMIR>>();
                         sizexpr->operand    = std::move(target);
                     },
                     [&](Box<TypeName>& typen) mutable {
                         dv_call(std::monostate{}, typen);
                         Type *target     = take_last_result<Type *>();
                         sizexpr->operand = target;
                     }},
               node.operand);

    Box<ExprMIR> expr = std::move(sizexpr);
    dv_return(expr);
}