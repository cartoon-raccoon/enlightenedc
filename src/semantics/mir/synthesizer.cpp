#include <memory>
#include <sstream>
#include <stdexcept>
#include <cassert>
#include <variant>

#include "semantics/mir/synthesizer.hpp"
#include "ast/ast.hpp"
#include "semantics/mir/mir.hpp"
#include "error.hpp"
#include "semantics/symbols.hpp"
#include "semantics/semerr.hpp"
#include "semantics/types.hpp"
#include "eval/value.hpp"
#include "eval/exec.hpp"
#include "util.hpp"

#define dv_return(val) do { last_result = std::move(val); return; } while (0)

#define dv_call(param, obj) do { dovisit_param = std::move(param); (obj)->accept(*this); } while (0)

using namespace ecc::ast;
using namespace ecc::sema;
using namespace ecc::sema::types;
using namespace ecc::sema::sym;
using namespace ecc::sema::mir;

Box<MIRSynthesizer::SpecifierInfo> MIRSynthesizer::parse_speclist(
    Vec<Box<ast::DeclarationSpecifier>>& speclist, Location loc
) {
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
                    specinfo->is_const = true;
                }
            }
            break;

            case NK::STORAGE_SPEC: {
                auto spectype = take_last_result<StorageClassSpecifier::SpecType>();
                switch (spectype) {
                    case StorageClassSpecifier::EXTERN:
                    specinfo->is_extern = true;
                    break;

                    case StorageClassSpecifier::PUBLIC:
                    specinfo->is_public = true;
                    break;

                    case StorageClassSpecifier::STATIC:
                    specinfo->is_static = true;
                    break;

                    case StorageClassSpecifier::EXTERNC:
                    specinfo->is_externc = true;
                    break;
                }
            }
            break;

            case NK::CLASS_SPEC: {
                auto typespecret = take_last_result<TypeSpecRet<ClassType>>();
                specinfo->type = typespecret.type;
                specinfo->symbol = std::move(typespecret.symbol);
                break;
            }

            case NK::UNION_SPEC: {
                auto typespecret = take_last_result<TypeSpecRet<UnionType>>();
                specinfo->type = typespecret.type;
                specinfo->symbol = std::move(typespecret.symbol);
                break;
            }

            case NK::ENUM_SPEC: {
                auto typespecret = take_last_result<TypeSpecRet<EnumType>>();
                specinfo->type = typespecret.type;
                specinfo->symbol = std::move(typespecret.symbol);
                break;
            }

            case NK::VOID_SPEC:
            specinfo->type = take_last_result<VoidType *>();
            break;

            case NK::PRIM_SPEC:
            specinfo->type = take_last_result<PrimitiveType *>();
            break;

            default:
            throw std::runtime_error("encountered a non-declaration specifier while parsing specifiers");
        }
    }
    assert(specinfo->type);
    bsv_dbprint("finished parsing specifiers for node ", loc);

    return std::move(specinfo);
}

void MIRSynthesizer::do_visit(Program& node) {
    bsv_dbprint("visiting Program node: ", node.loc);

    for (auto& item : node.items) {
        dv_call(std::monostate {}, item);
        std::visit(overloaded {
            [this] (Box<DeclMIR>& decl) mutable {
                prog_mir.add_item(std::move(decl));
            },
            [this] (Box<StmtMIR>& stmt) mutable {
                prog_mir.add_item(std::move(stmt));
            },
            [this] (Box<FunctionMIR>& func) mutable {
                prog_mir.add_item(std::move(func));
            },
            [] (auto& _) {
                // todo: throw exception
            }
        }, last_result);
    }

    // todo
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
    ElabVisitParam param = std::move(dovisit_param);
    Box<SpecifierInfo> specinfo = parse_speclist(node.decl_spec_list, node.loc);
    dovisit_param = std::move(param);

    BaseType *return_base = specinfo->type;

    if (!node.declarator->direct) {
        throw EccError("function declaration but missing direct declarator", node.declarator->loc);
    }
    if (node.declarator->direct.value()->kind != ASTNode::FUNC_DECLTR) {
        throw EccError("function declaration but declarator is not function", node.declarator->loc);
    }

    // Visit the Declarator to construct the type builder.
    dv_call(std::monostate {}, node.declarator);
    auto builder = take_last_result<Box<DeclaratorBuilder>>();
    builder->ty_bldr.set_base(return_base);;

    // The latest function parameters.
    Vec<FuncParam> last_func_params;

    // Extract the base type from our builder.
    Type *curr = builder->ty_bldr.base;

    while (!builder->ty_bldr.type_stack.empty()) {
        auto next_cstrctr = builder->ty_bldr.type_stack.top();
        std::visit(overloaded{
            [&builder, &curr] (TypeBuilder::Arr& a) mutable {
                // Wrap the base in an array.
                if (a.size) {
                    curr = builder->ty_bldr.ctxt.get_array(curr, *a.size);
                } else {
                    curr = builder->ty_bldr.ctxt.get_array(curr);
                }
            },
            [&builder, &curr] (TypeBuilder::Ptr& p) mutable {
                // Wrap the base in a pointer.
                curr = builder->ty_bldr.ctxt.get_pointer(curr, p.is_const);
            },
            [&builder, &curr, &last_func_params, this] (TypeBuilder::FnParams& fn) mutable {
                // map out the identifiers.
                Vec<Type *> params;
                params.reserve(fn.params.size());
                for (auto& param : fn.params) {
                    params.push_back(param.type);
                }

                last_func_params = std::move(fn.params);
                // Wrap the base as the return type in a function type.
                curr = builder->ty_bldr.ctxt.get_function(
                    curr, std::move(params), fn.variadic, syms.current);
            }
        }, next_cstrctr);

        // Pop the stack to the next constructor
        builder->ty_bldr.type_stack.pop();
    }

    FunctionType *functype = curr->as_function();
    if (!functype) {
        throw EccError("unable to resolve declarator to function declarator", node.loc);
    }
    if (!builder->name) {
        throw EccError("unable to parse name from declarator", node.declarator->loc);
    }

    Box<FuncSymbol> symbol = std::make_unique<FuncSymbol>(node.loc, *builder->name, functype);
    FuncSymbol *sym_ptr = symbol.get();
    try {
        syms.insert(*builder->name, std::move(symbol));
    } catch ( Location prev_def ) {
        throw SymbolAlrDecldError(
            std::format("function \"{}\" was previously declared", symbol->name), 
            symbol->loc, prev_def);
    }

    Vec<Box<VarSymbol>> params;
    for (FuncParam& param : last_func_params) {
        if (!param.name) {
            throw EccError("parameter in function declaration has no name", param.loc);
        }
        params.emplace_back(std::make_unique<VarSymbol>(param.loc, *param.name, param.type));
    }

    CmpdStmtDoVisitParam cmpdstmtp({sym_ptr, std::move(params)});

    // Then make call
    dv_call(cmpdstmtp, node.body);

    auto res = take_last_result<CmpdStmtFromFuncRes>();

    Box<FunctionMIR> func = std::make_unique<FunctionMIR>(node.loc, sym_ptr, res.second, std::move(res.first));
    dv_return(func);
}

void MIRSynthesizer::do_visit(TypeDeclaration& node) {
    bsv_dbprint("visiting TypeDeclaration node: ", node.loc);

    auto specinfo = parse_speclist(node.specifiers, node.loc);

    if (specinfo->symbol) {
        syms.insert(specinfo->symbol.value()->name, std::move(*specinfo->symbol));
    }
    dv_return(std::monostate {});

    Box<DeclMIR> decl = std::make_unique<TypeDeclMIR>(node.loc, specinfo->symbol.value().get());
    dv_return(decl);
}

void MIRSynthesizer::do_visit(VariableDeclaration& node) {
    bsv_dbprint("visiting VariableDeclaration node: ", node.loc);
    auto specinfo = parse_speclist(node.specifiers, node.loc);

    Box<VarDeclMIR> var_decl = std::make_unique<VarDeclMIR>(node.loc);

    for (auto& declarator : node.declarators) {
        // call accept on our declarator
        dv_call(std::monostate{}, declarator);

        // take the last result; should be InitDecltrRet
        auto ret = take_last_result<InitDecltrRet>();

        // extract the builder and finalize our type
        Box<DeclaratorBuilder> builder = std::move(ret.builder);
        builder->ty_bldr.set_base(specinfo->type);
        Type *complete = builder->ty_bldr.finalize();

        // declare symbol and corresponding pointer
        Box<VarSymbol> sym = nullptr;
        VarSymbol *symptr = nullptr;
        if (builder->name) {
            // initialize our symbol and its pointer
            sym = std::make_unique<VarSymbol>(declarator->loc, *builder->name, complete);
            symptr = sym.get();

            // populate other specifiers, and then insert into symbol table
            sym->is_public = specinfo->is_public;
            sym->is_static = specinfo->is_static;
            sym->is_const = specinfo->is_const;
            sym->is_extern = specinfo->is_extern;
            sym->is_externc = specinfo->is_externc;
            syms.insert(*builder->name, std::move(sym));
        } else {
            throw EccError("variable declaration with no name", declarator->loc);
        }

        // sym should be valid, since to get here builder's name had to exist
        if (!symptr) {
            throw EccError("unexpected null pointer when parsing variable declarator", declarator->loc);
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

    dv_call(std::monostate {}, node.declarator);
    // pull builder before we visit the initializer
    Box<DeclaratorBuilder> builder = take_last_result<Box<DeclaratorBuilder>>();

    if (node.initializer) {
        // call accept on our initializer
        dv_call(std::monostate {}, *node.initializer);
        auto init_mir = take_last_result<Box<InitializerMIR>>();
        InitDecltrRet ret = {std::move(builder), std::move(init_mir)};
        dv_return(ret);
    } else {
        InitDecltrRet ret = {std::move(builder), {}};
        dv_return(ret);
    }
}

void MIRSynthesizer::do_visit(Declarator& node) {
    Box<DeclaratorBuilder> builder;
    if (node.direct) {
        dv_call(std::monostate {}, node.direct.value());
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
    dv_call(std::monostate {}, node.inner);

    dv_return(take_last_result<Box<DeclaratorBuilder>>());
}

void MIRSynthesizer::do_visit(ArrayDeclarator& node) {
    bsv_dbprint("visiting ArrayDeclarator node: ", node.loc);
    dv_call(std::monostate {}, node.base);

    auto builder = take_last_result<Box<DeclaratorBuilder>>();
    
    std::optional<uint64_t> size {};
    if (node.size) {
        bsv_dbprint("array declarator has size, checking for compile time computability");
        // if we have a size for the array, evaluate it
        exec::Value val;
        exec::Evaluator evalr(syms, types);
        val = node.size.value().get()->accept(evalr);

        // if we were able to parse it as a u64, great
        if (auto s = val.value_as<long>()) {
            if (*s < 0) {
                throw EccError("array size cannot be a negative number");
            }
            size = *s;
        } else {
            // otherwise, the expression could not resolve, throw error
            // (do NOT coerce to unsized)
            throw EccError("could not evaluate size expression to U64", node.loc);
        }
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
    dv_call(std::monostate {}, node.base);
    auto builder = take_last_result<Box<DeclaratorBuilder>>();

    Vec<FuncParam> parameters;
    for (auto& param : node.parameters) {
        dv_call(std::monostate {}, param);
        FuncParam parsed = take_last_result<FuncParam>();
        parameters.push_back(parsed);
    }

    builder->ty_bldr.add_function(parameters, node.is_variadic, syms.current);

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
        dv_call(std::monostate {}, *node.declarator);
        auto builder = take_last_result<Box<DeclaratorBuilder>>();
        builder->ty_bldr.set_base(specinfo->type);

        Type *final_type = builder->ty_bldr.finalize();
        if (builder->name) {
            ret = {final_type, *builder->name, node.loc, specinfo->is_const};
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

    auto builder = take_dovisit_param<DeclaratorBuilder *>();

    bool is_const = false;
    for (auto& qual : node.qualifiers) {
        dv_call(std::monostate {}, qual);
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
        dv_return(std::monostate {});
    } else {
        builder->ty_bldr.add_pointer(is_const);
        dv_return(std::monostate {});
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

void MIRSynthesizer::do_visit(PrimitiveSpecifier& node) {
    bsv_dbprint("visiting PrimitiveSpecifier node: ", node.loc);
    /* terminal node */

    using PK = PrimitiveSpecifier::PrimKind;
    using PTK = PrimitiveType::Kind;

    switch (node.pkind) {
        case PK::U8:
        dv_return(types.get_primitive(PTK::U8));
        break;

        case PK::U16:
        dv_return(types.get_primitive(PTK::U16));
        break;

        case PK::U32:
        dv_return(types.get_primitive(PTK::U32));
        break;

        case PK::U64:
        dv_return(types.get_primitive(PTK::U64));
        break;

        case PK::I8:
        dv_return(types.get_primitive(PTK::I8));
        break;

        case PK::I16:
        dv_return(types.get_primitive(PTK::I16));
        break;

        case PK::I32:
        dv_return(types.get_primitive(PTK::I32));
        break;

        case PK::I64:
        dv_return(types.get_primitive(PTK::I64));
        break;

        case PK::F64:
        dv_return(types.get_primitive(PTK::F64));
        break;

        case PK::BOOL:
        dv_return(types.get_primitive(PTK::BOOL));
        break;
    }
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
            enm = types.get_enum(*(node.name), syms.current);
        } else {
            enm = types.get_enum(syms.current);
        }
    } catch ( UserType *prev_def ) {
        throw TypeDecldAsOtherError("enum already declared as another type", node.loc, prev_def->loc);
    }

    TypeSpecRet<EnumType> ret({}, enm);

    if (node.enumerators) {
        if (enm->complete) {
            throw TypeAlrDefinedError("enum was previously defined", node.loc, enm->loc);
        }

        for (auto& enumtr : *node.enumerators) {
            dv_call(enm, enumtr);
        }
        if (node.name) {
            Box<TypeSymbol> sym = std::make_unique<sym::TypeSymbol>(node.loc, *node.name, enm);
            syms.tie_current_to(sym.get());
            ret.symbol = std::move(sym);
        }
    }


    dv_return(ret);
}

void MIRSynthesizer::do_visit(Enumerator& node) {
    bsv_dbprint("visiting Enumerator node with name ", node.name);
    EnumType *enm = take_dovisit_param<EnumType *>();

    if (auto mem = enm->find(node.name)) {
        throw EnumeratorAlrDecldError(
            std::format("symbol \"{}\" previously declared", node.name), 
                        node.loc, mem->loc);
    }

    exec::Value value;
    if (node.value) {
        exec::Evaluator evalr(syms, types);
        value = node.value.value()->accept(evalr);
        std::optional<long> val = value.value_as<long>();
        if (val) {
            enm->add_enumerator(node.name, *val, node.loc);
        } else {
            throw EccError("invalid enumerator value", node.loc);
        }
    } else {
        value = enm->add_enumerator(node.name, node.loc);
    }

    syms.insert(node.name, std::make_unique<VarSymbol>(node.loc, node.name, enm, value));

    dv_return(std::monostate {});
}

void MIRSynthesizer::do_visit(ClassSpecifier& node) {
    bsv_dbprint("visiting ClassSpecifier node: ", node.loc);
    ClassType *cls = nullptr;
    try {
        if (node.name) {
            cls = types.get_class(*(node.name), syms.current);
        } else {
            cls = types.get_class(syms.current);
        }
    } catch (UserType *prev_def) {
        throw TypeDecldAsOtherError("class already declared as another type", node.loc, prev_def->loc);
    }

    TypeSpecRet<ClassType> ret({}, cls);

    if (node.declarations) {
        if (cls->complete) {
            // error: class was previously defined
            throw TypeAlrDefinedError("class was previously defined", node.loc, cls->loc);
        }

        // class is defined here, populate its members and mark it complete
        for (auto& decl : *node.declarations) {
            dv_call(cls, decl);
        }

        cls->complete = true;
        if (node.name) {
            Box<TypeSymbol> sym = std::make_unique<sym::TypeSymbol>(node.loc, *node.name, cls);
            syms.tie_current_to(sym.get());
            ret.symbol = std::move(sym);
        }
    }

    dv_return(ret);
}

void MIRSynthesizer::do_visit(UnionSpecifier& node) {
    bsv_dbprint("visiting UnionSpecifier node ", node.loc);
    UnionType *unn = nullptr;
    try {
        if (node.name) {
            unn = types.get_union(*(node.name), syms.current);
        } else {
            unn = types.get_union(syms.current);
        }
    } catch (UserType *prev_def) {
        throw TypeDecldAsOtherError("union already declared as another type", node.loc, prev_def->loc);
    }

    TypeSpecRet<UnionType> ret({}, unn);

    // declarations are present, start definition
    if (node.declarations) {
        if (unn->complete) {
            // error: union was previously defined
            throw TypeAlrDefinedError("union was previously defined", node.loc, unn->loc);
        }
        // union is defined here, populate its members and mark it complete
        for (auto& decl : *node.declarations) {
            dv_call(unn, decl);
        }

        unn->complete = true;
        // insert new symbol only at definition
        if (node.name) {
            Box<TypeSymbol> sym = std::make_unique<sym::TypeSymbol>(node.loc, *node.name, unn);
            syms.tie_current_to(sym.get());
            ret.symbol = std::move(sym);
        }
    }

    dv_return(ret);
}

void MIRSynthesizer::do_visit(ClassDeclaration& node) {
    bsv_dbprint("visiting ClassDeclaration node: ", node.loc);

    // save our current param, as it may get clobbered while parsing specifiers
    ElabVisitParam param = std::move(dovisit_param);
    Box<SpecifierInfo> specinfo = parse_speclist(node.specifiers, node.loc);

    // restore our current param
    dovisit_param = std::move(param);
    

    std::visit(overloaded {
        // ClassType case
        [&specinfo, &node, this] (ClassType *cls) {
            bsv_dbprint("parsing ClassDeclaration for ClassType ", cls);
            for (auto& decltr : node.declarators) {
                dv_call(std::monostate {}, decltr);
                std::visit(overloaded {
                    [&cls, &specinfo, &decltr] (Box<DeclaratorBuilder>& builder) {
                        builder->ty_bldr.set_base(specinfo->type);
                        Type *finaltype = builder->ty_bldr.finalize();
                        auto member = std::make_unique<ClassType::ClassTypeMember>(
                            ClassType::ClassTypeMember(builder->name, finaltype, decltr->loc));

                        cls->add_member(std::move(member));
                    },
                    [&cls, &specinfo, &decltr] (std::monostate& _) {
                        // no declarator, use the base type
                        auto member = std::make_unique<ClassType::ClassTypeMember>(
                            ClassType::ClassTypeMember(specinfo->type, decltr->loc));

                        cls->add_member(std::move(member));
                    },
                    [] (auto& _) {
                        throw std::runtime_error("unexpected last_result when parsing ClassDeclaration");
                    }
                }, last_result);

                last_result = std::monostate {};
            }

        },

        // UnionType case
        [&specinfo, &node, this] (UnionType *unn) {
            bsv_dbprint("parsing ClassDeclaration for UnionType ", unn);
            for (auto& decltr : node.declarators) {
                dv_call(std::monostate {}, decltr);
                std::visit(overloaded {
                    [&unn, &specinfo, &decltr] (Box<DeclaratorBuilder>& builder) {
                        builder->ty_bldr.set_base(specinfo->type);
                        Type *finaltype = builder->ty_bldr.finalize();

                        auto member = std::make_unique<UnionType::UnionTypeMember>(
                            UnionType::UnionTypeMember(builder->name, finaltype, decltr->loc));

                        unn->add_member(std::move(member));
                    },
                    [&unn, &specinfo, &decltr] (std::monostate& _) {
                        // no declarator, use the base type
                        auto member = std::make_unique<UnionType::UnionTypeMember>(
                            UnionType::UnionTypeMember(specinfo->type, decltr->loc));

                        unn->add_member(std::move(member));
                    },
                    [] (auto& _) {
                        throw std::runtime_error("unexpected last_result when parsing UnionDeclaration");
                    }
                }, last_result);

                last_result = std::monostate {};
            }
        },

        [&node, this] (auto& e) {
            bsv_dbprint(typeid(e).name(), " ", node.loc);
            throw std::runtime_error("unexpected dovisit param when parsing ClassDeclaration");
        }
    }, param);

    dv_return(std::monostate {});
}

void MIRSynthesizer::do_visit(ClassDeclarator& node) {
    bsv_dbprint("visiting ClassDeclarator node: ", node.loc);
    if (node.declarator) {
        dv_call(std::monostate {}, node.declarator.value());
        dv_return(take_last_result<Box<DeclaratorBuilder>>());
    } else {
        dv_return(std::monostate{});
    }

    // fixme: ignoring bit width for now, implement this when able
}

void MIRSynthesizer::do_visit(Initializer& node) {
    bsv_dbprint("visiting Initializer node: ", node.loc);

    Location loc = node.loc;

    return std::visit(overloaded {
        [this, loc] (Box<Expression>& expr) {
            bsv_dbprint("visiting single initializer");
            dv_call(std::monostate {}, expr);
            Box<ExprMIR> exprmir = take_last_result<Box<ExprMIR>>();
            Box<InitializerMIR> init = std::make_unique<InitializerMIR>(loc, std::move(exprmir));
            dv_return(init);
        },
        [this, loc] (Vec<Box<Initializer>>& inits) {
            bsv_dbprint("visiting compound initializer");
            Vec<Box<InitializerMIR>> init_mirs {};
            for (auto& init : inits) {
                dv_call(std::monostate {}, init);
                Box<InitializerMIR> initmir = take_last_result<Box<InitializerMIR>>();
                init_mirs.push_back(std::move(initmir));
            }
            Box<InitializerMIR> fullinit = std::make_unique<InitializerMIR>(loc, std::move(init_mirs));
            dv_return(fullinit);
        }
    }, node.initializer);
}

void MIRSynthesizer::do_visit(TypeName& node) {
    // dovisit_param: monostate
    // last_result: Type *
    Box<SpecifierInfo> specinfo = parse_speclist(node.specifiers, node.loc);

    if (node.declarator) {
        dv_call(std::monostate {}, *node.declarator);
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
    std::visit(overloaded {
        [&add_symbols] (CmpdStmtDoVisitParam& param) mutable {
            //dbprint("CmpdStmtDoVisitParams found");
            add_symbols = std::move(param);
        },
        [&add_symbols] (auto _) mutable {
            //dbprint("No params found");
            add_symbols = {};
        }
    }, dovisit_param);

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

    Vec<Box<ProgItemMIR>> progitems {};
    for (auto& item : node.items) {
        std::visit(overloaded {
            [&progitems] (Box<DeclMIR>& decl) mutable {
                progitems.push_back(std::move(decl));
            },
            [&progitems] (Box<StmtMIR>& stmt) mutable {
                progitems.push_back(std::move(stmt));
            },
            [&progitems] (Box<FunctionMIR>& func) mutable {
                progitems.push_back(std::move(func));
            },
            [] (auto& _) {
                // todo: throw exception
            }
        }, last_result);
    }

    // resolve our return value
    if (add_symbols) {
        // we had add_symbols, so we were called from function
        Box<CompoundStmtMIR> cmpdmir = std::make_unique<CompoundStmtMIR>(
            node.loc, std::move(progitems));
        ElabResult ret = std::pair(std::move(cmpdmir), syms.current);

        dv_return(ret);
    } else {

        Box<StmtMIR> cmpdmir = std::make_unique<CompoundStmtMIR>(
            node.loc, std::move(progitems));

        dv_return(cmpdmir);
    }
}

void MIRSynthesizer::do_visit(ExpressionStatement& node) {
    bsv_dbprint("visiting ExpressionStatement node: ", node.loc);
    if (node.expression) {
        dv_call(std::monostate {}, *node.expression);
        auto expr = take_last_result<Box<ExprMIR>>();

        Box<StmtMIR> stmt = std::make_unique<ExprStmtMIR>(node.loc, std::move(expr));
        dv_return(stmt);
    } else {
        Box<StmtMIR> stmt = std::make_unique<ExprStmtMIR>(node.loc);
        dv_return(stmt);
    }
}

void MIRSynthesizer::do_visit(CaseStatement& node) {
    bsv_dbprint("visiting CaseStatement node: ", node.loc);
    dv_call(std::monostate {}, node.case_expr);
    Box<ExprMIR> case_expr = take_last_result<Box<ExprMIR>>();

    dv_call(std::monostate {}, node.statement);
    Box<StmtMIR> stmt = take_last_result<Box<StmtMIR>>();

    Box<StmtMIR> casestmt = std::make_unique<CaseStmtMIR>(
        node.loc, std::move(case_expr), std::move(stmt));

    dv_return(casestmt);
}

void MIRSynthesizer::do_visit(CaseRangeStatement& node) {
    bsv_dbprint("visiting CaseRangeStatement node: ", node.loc);

    dv_call(std::monostate {}, node.range_start);
    Box<ExprMIR> case_start = take_last_result<Box<ExprMIR>>();
    
    dv_call(std::monostate {}, node.range_end);
    Box<ExprMIR> case_end = take_last_result<Box<ExprMIR>>();

    dv_call(std::monostate {}, node.statement);
    Box<StmtMIR> stmt = take_last_result<Box<StmtMIR>>();
    
    Box<StmtMIR> casestmt = std::make_unique<CaseRangeStmtMIR>(
        node.loc, std::move(case_start), std::move(case_end), std::move(stmt));

    dv_return(casestmt);
}

void MIRSynthesizer::do_visit(DefaultStatement& node) {
    bsv_dbprint("visiting DefaultStatement node: ", node.loc);
    dv_call(std::monostate {}, node.statement);
    Box<StmtMIR> stmt = take_last_result<Box<StmtMIR>>();
    
    Box<StmtMIR> defstmt = std::make_unique<DefaultStmtMIR>(
        node.loc, std::move(stmt));

    dv_return(defstmt);
}

void MIRSynthesizer::do_visit(LabeledStatement& node) {
    bsv_dbprint("visiting LabeledStatement node: ", node.loc);
    Box<LabelSymbol> label = std::make_unique<sym::LabelSymbol>(node.loc, node.label);
    LabelSymbol *labelptr = label.get();
    syms.insert(node.label, std::move(label));
    dv_call(std::monostate {}, node.statement);

    auto stmt = take_last_result<Box<StmtMIR>>();

    Box<StmtMIR> ret = std::make_unique<LabeledStmtMIR>(node.loc, labelptr, std::move(stmt));
    dv_return(ret);
}

void MIRSynthesizer::do_visit(PrintStatement& node) {
    bsv_dbprint("visiting PrintStatement node: ", node.loc);

    Vec<Box<ExprMIR>> exprs {};
    exprs.reserve(node.arguments.size());

    for (auto& arg : node.arguments) {
        dv_call(std::monostate {}, arg);
        Box<ExprMIR> argmir = take_last_result<Box<ExprMIR>>();
        exprs.push_back(std::move(argmir));
    }

    Box<StmtMIR> printstmt = std::make_unique<PrintStmtMIR>(
        node.loc, node.format_string, std::move(exprs));

    dv_return(printstmt);
}

void MIRSynthesizer::do_visit(IfStatement& node) {
    bsv_dbprint("visiting IfStatement node: ", node.loc);
    dv_call(std::monostate {}, node.condition);
    Box<ExprMIR> cond = take_last_result<Box<ExprMIR>>();

    dv_call(std::monostate {}, node.then_branch);
    Box<StmtMIR> then_br = take_last_result<Box<StmtMIR>>();

    std::optional<Box<StmtMIR>> else_br;
    if (node.else_branch.has_value()) {
        dv_call(std::monostate {}, node.else_branch.value());
        else_br = std::move(take_last_result<Box<StmtMIR>>());
    }

    Box<StmtMIR> ifstmt = std::make_unique<IfStmtMIR>(
        node.loc, std::move(cond), std::move(then_br), std::move(else_br));

    dv_return(ifstmt);
}

void MIRSynthesizer::do_visit(SwitchStatement& node) {
    bsv_dbprint("visiting SwitchStatement node: ", node.loc);
    dv_call(std::monostate {}, node.condition);
    Box<ExprMIR> cond = take_last_result<Box<ExprMIR>>();

    dv_call(std::monostate {}, node.body);
    Box<StmtMIR> stmt = take_last_result<Box<StmtMIR>>();

    Box<StmtMIR> switchst = std::make_unique<SwitchStmtMIR>(
        node.loc, std::move(cond), std::move(stmt));
    
    dv_return(switchst);
}

void MIRSynthesizer::do_visit(WhileStatement& node) {
    bsv_dbprint("visiting WhileStatement node: ", node.loc);

    dv_call(std::monostate {}, node.condition);
    Box<ExprMIR> cond = take_last_result<Box<ExprMIR>>();
    dv_call(std::monostate {}, node.body);
    Box<StmtMIR> body = take_last_result<Box<StmtMIR>>();

    // Create the name for the loop start label
    std::stringstream ss;
    ss << "while_start_" << &node;

    // Create the label symbol
    Box<LabelSymbol> loop_label = std::make_unique<LabelSymbol>(node.loc, ss.str());
    LabelSymbol *loop_lbl_ptr = syms.insert(ss.str(), std::move(loop_label))->as_labsym();
    assert(loop_lbl_ptr);

    // Create the LabeledStmtMIR node
    Box<LabeledStmtMIR> lab_body = std::make_unique<LabeledStmtMIR>(
        node.loc, loop_lbl_ptr, std::move(body));

    // Create the actual loop
    Box<StmtMIR> loop = std::make_unique<LoopStmtMIR>(
        node.loc, std::move(cond), std::move(lab_body), false);

    dv_return(loop);
}

void MIRSynthesizer::do_visit(DoWhileStatement& node) {
    bsv_dbprint("visiting DoWhileStatement node: ", node.loc);

    dv_call(std::monostate {}, node.condition);
    Box<ExprMIR> cond = take_last_result<Box<ExprMIR>>();
    dv_call(std::monostate {}, node.body);
    Box<StmtMIR> body = take_last_result<Box<StmtMIR>>();

    std::stringstream ss;
    ss << "dowhile_start_" << &node;

    Box<LabelSymbol> loop_label = std::make_unique<LabelSymbol>(node.loc, ss.str());
    LabelSymbol *loop_lbl_ptr = syms.insert(ss.str(), std::move(loop_label))->as_labsym();
    assert(loop_lbl_ptr);

    Box<LabeledStmtMIR> lab_body = std::make_unique<LabeledStmtMIR>(
        node.loc, loop_lbl_ptr, std::move(body));

    Box<StmtMIR> loop = std::make_unique<LoopStmtMIR>(
        node.loc, std::move(cond), std::move(lab_body), true);

    dv_return(loop);
}

void MIRSynthesizer::do_visit(ForStatement& node) {
    bsv_dbprint("visiting ForStatement node: ", node.loc);

    dv_call(std::monostate {}, node.body);
    Box<StmtMIR> body = take_last_result<Box<StmtMIR>>();

    std::stringstream ss;
    ss << "for_start_" << &node;

    Box<LabelSymbol> loop_label = std::make_unique<LabelSymbol>(node.loc, ss.str());
    LabelSymbol *loop_lbl_ptr = syms.insert(ss.str(), std::move(loop_label))->as_labsym();
    assert(loop_lbl_ptr);

    Box<LabeledStmtMIR> lab_body = std::make_unique<LabeledStmtMIR>(
        node.loc, loop_lbl_ptr, std::move(body));

    Box<LoopStmtMIR> loop = std::make_unique<LoopStmtMIR>(node.loc, std::move(lab_body));

    if (node.init.has_value()) {
        std::visit(overloaded {
            [this, &loop] (Box<Expression>& expr) {
                dv_call(std::monostate {}, expr);
                Box<ExprMIR> exprmir = take_last_result<Box<ExprMIR>>();
                Box<ExprStmtMIR> exprstmt = std::make_unique<ExprStmtMIR>(
                    expr->loc, std::move(exprmir));
                
                loop->init = std::move(exprstmt);
            },
            [this, &loop] (Box<VariableDeclaration>& decl) {
                dv_call(std::monostate {}, decl);
                Box<DeclMIR> declmir = take_last_result<Box<DeclMIR>>();
                loop->init = std::move(declmir);
            }
        }, *node.init);
    }

    if (node.condition.has_value()) {
        dv_call(std::monostate {}, node.condition.value());
        Box<ExprMIR> cond = take_last_result<Box<ExprMIR>>();
        loop->condition = std::move(cond);
    }

    if (node.increment.has_value()) {
        dv_call(std::monostate {}, node.increment.value());
        Box<ExprMIR> step_expr = take_last_result<Box<ExprMIR>>();
        Box<StmtMIR> step_stmt = std::make_unique<ExprStmtMIR>(
            step_expr->loc, std::move(step_expr));

        loop->step = std::move(step_stmt);
    }

    Box<StmtMIR> stmt = std::move(loop);
    dv_return(stmt);
}

void MIRSynthesizer::do_visit(GotoStatement& node) {
    bsv_dbprint("visiting GotoStatement node: ", node.loc);

    Symbol *target = syms.lookup(node.target_label);
    if (!target) {
        throw EccError("could not find specified target label", node.loc);
    }

    LabelSymbol *label = target->as_labsym();

    Box<StmtMIR> stmt = std::make_unique<GotoStmtMIR>(node.loc, label);
    dv_return(stmt);
}

void MIRSynthesizer::do_visit(BreakStatement& node) {
    bsv_dbprint("visiting BreakStatement node: ", node.loc);

    Box<StmtMIR> stmt = std::make_unique<BreakStmtMIR>(node.loc);
    dv_return(stmt);
}

void MIRSynthesizer::do_visit(ReturnStatement& node) {
    bsv_dbprint("visiting ReturnStatement node: ", node.loc);

    Box<ReturnStmtMIR> retstmt = std::make_unique<ReturnStmtMIR>(node.loc);

    if (node.return_value) {
        dv_call(std::monostate {}, *node.return_value);
        Box<ExprMIR> return_value = take_last_result<Box<ExprMIR>>();
        retstmt->ret_expr = std::move(return_value);
    }

    Box<StmtMIR> stmt = std::move(retstmt);
    dv_return(stmt);
}

void MIRSynthesizer::do_visit(BinaryExpression& node) {
    bsv_dbprint("visiting BinaryExpression node: ", node.loc);

    dv_call(std::monostate {}, node.left);
    Box<ExprMIR> left = take_last_result<Box<ExprMIR>>();
    dv_call(std::monostate {}, node.right);
    Box<ExprMIR> right = take_last_result<Box<ExprMIR>>();

    Box<ExprMIR> expr = std::make_unique<BinaryExprMIR>(
        node.loc, std::move(left), std::move(right), node.op);

    dv_return(expr);
}

void MIRSynthesizer::do_visit(UnaryExpression& node) {
    bsv_dbprint("visiting UnaryExpression node: ", node.loc);
    dv_call(std::monostate {}, node.operand);
    Box<ExprMIR> operand = take_last_result<Box<ExprMIR>>();

    Box<ExprMIR> expr = std::make_unique<UnaryExprMIR>(
        node.loc, std::move(operand), node.op);

    dv_return(expr);
}

void MIRSynthesizer::do_visit(CastExpression& node) {
    bsv_dbprint("visiting CastExpression node: ", node.loc);
    dv_call(std::monostate {}, node.inner);
    Box<ExprMIR> inner = take_last_result<Box<ExprMIR>>();
    dv_call(std::monostate {}, node.type_name);
    Type *target = take_last_result<Type *>();

    Box<ExprMIR> expr = std::make_unique<CastExprMIR>(node.loc, target, std::move(inner));

    dv_return(expr);
}

void MIRSynthesizer::do_visit(AssignmentExpression& node) {
    bsv_dbprint("visiting AssignmentExpression node: ", node.loc);
    dv_call(std::monostate {}, node.left);
    Box<ExprMIR> left = take_last_result<Box<ExprMIR>>();
    dv_call(std::monostate {}, node.right);
    Box<ExprMIR> right = take_last_result<Box<ExprMIR>>();

    Box<ExprMIR> expr = std::make_unique<AssignExprMIR>(
        node.loc, std::move(left), std::move(right), node.op);

    dv_return(expr);
}

void MIRSynthesizer::do_visit(ConditionalExpression& node) {
    bsv_dbprint("visiting ConditionalExpression node: ", node.loc);
    dv_call(std::monostate {}, node.condition);
    Box<ExprMIR> condition = take_last_result<Box<ExprMIR>>();
    dv_call(std::monostate {}, node.true_expr);
    Box<ExprMIR> true_expr = take_last_result<Box<ExprMIR>>();
    dv_call(std::monostate {}, node.false_expr);
    Box<ExprMIR> false_expr = take_last_result<Box<ExprMIR>>();

    Box<ExprMIR> expr = std::make_unique<CondExprMIR>(
        node.loc, std::move(condition), std::move(true_expr), std::move(false_expr));
    
    dv_return(expr);
}

void MIRSynthesizer::do_visit(IdentifierExpression& node) {
    bsv_dbprint("visiting IdentifierExpression node: ", node.loc);

    Symbol *sym = syms.lookup(node.name);
    if (!sym) {
        throw IdentNotDefinedError(node.name, node.loc);
    }
    if (sym->is_abstract()) {
        throw InvalidIdentifierError(node.name, node.loc);
    }

    PhysicalSymbol *physsym = sym->as_physical();
    assert(physsym);

    Box<ExprMIR> expr = std::make_unique<IdentExprMIR>(node.loc, physsym);

    dv_return(expr);
}

void MIRSynthesizer::do_visit(ConstExpression& node) {
    bsv_dbprint("visiting ConstExpression node: ", node.loc);
    dv_call(std::monostate {}, node.inner);
    Box<ExprMIR> inner = take_last_result<Box<ExprMIR>>();

    Box<ExprMIR> expr = std::make_unique<ConstExprMIR>(node.loc, std::move(inner));

    dv_return(expr);
}

void MIRSynthesizer::do_visit(LiteralExpression& node) {
    bsv_dbprint("visiting LiteralExpression node: ", node.loc);

    exec::Value val;
    switch (node.kind) {
        case LiteralExpression::INT:
        val = (long) node.value.i_val;
        break;

        case LiteralExpression::FLOAT:
        val = node.value.f_val;
        break;

        case LiteralExpression::CHAR:
        val = node.value.c_val;

        case LiteralExpression::BOOL:
        val = node.value.b_val;
    }

    Box<ExprMIR> expr = std::make_unique<LiteralExprMIR>(node.loc, val);

    dv_return(expr);
}

void MIRSynthesizer::do_visit(StringExpression& node) {
    bsv_dbprint("visiting StringExpression node: ", node.loc);

    Box<ExprMIR> expr = std::make_unique<LiteralExprMIR>(node.loc, node.value);

    dv_return(expr);
}

void MIRSynthesizer::do_visit(CallExpression& node) {
    bsv_dbprint("visiting CallExpression node: ", node.loc);
    dv_call(std::monostate {}, node.callee);
    Box<ExprMIR> callee = take_last_result<Box<ExprMIR>>();

    Vec<Box<ExprMIR>> args;
    for (auto& arg : node.arguments) {
        dv_call(std::monostate {}, arg);
        Box<ExprMIR> argument = take_last_result<Box<ExprMIR>>();
        args.push_back(std::move(argument));
    }

    Box<ExprMIR> call = std::make_unique<CallExprMIR>(
        node.loc, std::move(callee), std::move(args));

    dv_return(call);
}

void MIRSynthesizer::do_visit(MemberAccessExpression& node) {
    bsv_dbprint("visiting MemberAccessExpression node: ", node.loc);
    
    dv_call(std::monostate {}, node.object);
    Box<ExprMIR> object = take_last_result<Box<ExprMIR>>();

    Box<ExprMIR> expr = std::make_unique<MemberAccExprMIR>(
        node.loc, std::move(object), node.member, node.is_arrow);

    dv_return(expr);
}

void MIRSynthesizer::do_visit(ArraySubscriptExpression& node) {
    bsv_dbprint("visiting ArraySubscriptExpression node: ", node.loc);
    
    dv_call(std::monostate {}, node.array);
    Box<ExprMIR> array = take_last_result<Box<ExprMIR>>();

    dv_call(std::monostate {}, node.index);
    Box<ExprMIR> index = take_last_result<Box<ExprMIR>>();
    
    Box<ExprMIR> expr = std::make_unique<SubscrExprMIR>(
        node.loc, std::move(array), std::move(index));

    dv_return(expr);
}

void MIRSynthesizer::do_visit(PostfixExpression& node) {
    bsv_dbprint("visiting PostfixExpression node: ", node.loc);
    
    dv_call(std::monostate {}, node.operand);
    Box<ExprMIR> operand = take_last_result<Box<ExprMIR>>();

    Box<ExprMIR> expr = std::make_unique<PostfixExprMIR>(
        node.loc, std::move(operand), node.op);

    dv_return(expr);
}

void MIRSynthesizer::do_visit(SizeofExpression& node) {
    bsv_dbprint("visiting SizeofExpression node: ", node.loc);

    Box<SizeofExprMIR> sizexpr = std::make_unique<SizeofExprMIR>(node.loc);
    std::visit(overloaded {
        [this, &sizexpr] (Box<Expression>& expr) mutable {
            dv_call(std::monostate {}, expr);
            Box<ExprMIR> target = take_last_result<Box<ExprMIR>>();
            sizexpr->operand = std::move(target);
        },
        [this, &sizexpr] (Box<TypeName>& typen) mutable {
            dv_call(std::monostate {}, typen);
            Type *target = take_last_result<Type *>();
            sizexpr->operand = target;
        }
    }, node.operand);

    Box<ExprMIR> expr = std::move(sizexpr);
    dv_return(expr);
}