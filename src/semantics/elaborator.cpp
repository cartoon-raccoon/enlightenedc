#include "elaborator.hpp"
#include "ast/ast.hpp"
#include "codegen/value.hpp"
#include "error.hpp"
#include "semantics/symbols.hpp"
#include "semerr.hpp"
#include "semantics/types.hpp"
#include "codegen/exec.hpp"
#include "util.hpp"

#include <stdexcept>
#include <cassert>
#include <unistd.h>
#include <variant>

#define dv_return(val) do { last_result = std::move(val); return; } while (0)

#define dv_call(param, obj) do { dovisit_param = std::move(param); (obj)->accept(*this); } while (0)

using namespace ecc::ast;
using namespace ecc::sema;
using namespace ecc::sema::types;
using namespace ecc::sema::sym;

Box<Elaborator::SpecifierInfo> Elaborator::parse_speclist(Vec<Box<ast::DeclarationSpecifier>>& speclist) {
    using NK = ASTNode::NodeKind;

    bsv_dbprint("parsing declaration specifier list");
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
            }

            case NK::ENUM_SPEC: {
                auto typespecret = take_last_result<TypeSpecRet<EnumType>>();
                specinfo->type = typespecret.type;
                specinfo->symbol = std::move(typespecret.symbol);
                break;
            }

            case NK::PRIM_SPEC:
            specinfo->type = take_last_result<PrimitiveType *>();
            break;

            default:
            throw std::runtime_error("encountered a non-declaration specifier while parsing specifiers");
        }
    }
    assert(specinfo->type);

    return std::move(specinfo);
}

void Elaborator::do_visit(Function& node) {
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
    Box<SpecifierInfo> specinfo = parse_speclist(node.decl_spec_list);
    BaseType *return_base = specinfo->type;

    bsv_dbprint("direct declarator kind ", node.declarator->direct.value()->kind);

    if (!node.declarator->direct) {
        throw EccError("function declaration but missing direct declarator", node.declarator->loc);
    }
    if (node.declarator->direct.value()->kind != ASTNode::FUNC_DECLTR) {
        throw EccError("function declaration but declarator is not function", node.declarator->loc);
    }

    // Visit the Declarator to construct the type builder.
    dv_call(std::monostate {}, node.declarator);
    auto builder = take_last_result<Box<DeclaratorBuilder>>();
    builder->ty_bldr.set_base(return_base);

    // Extract the TypeBuiilder
    TypeBuilder& ty_bldr = builder->ty_bldr;

    // The latest function parameters.
    Vec<FuncParam> last_func_params;

    // Extract the base type from our builder.
    Type *curr = ty_bldr.base;

    while (!ty_bldr.type_stack.empty()) {
        auto next_cstrctr = ty_bldr.type_stack.top();
        std::visit(overloaded{
            [ty_bldr, curr] (TypeBuilder::Arr& a) mutable {
                // Wrap the base in an array.
                if (a.size) {
                    curr = ty_bldr.ctxt.get_array(curr, *a.size);
                } else {
                    curr = ty_bldr.ctxt.get_array(curr);
                }
            },
            [ty_bldr, curr] (TypeBuilder::Ptr& p) mutable {
                // Wrap the base in a pointer.
                curr = ty_bldr.ctxt.get_pointer(curr, p.is_const);
            },
            [ty_bldr, curr, &last_func_params] (TypeBuilder::FnParams& fn) mutable {
                // map out the identifiers.
                Vec<Type *> params;
                params.reserve(fn.params.size());
                for (auto& param : fn.params) {
                    params.push_back(param.type);
                }

                last_func_params = std::move(fn.params);
                // Wrap the base as the return type in a function type.
                curr = ty_bldr.ctxt.get_function(curr, std::move(params), fn.variadic);
            }
        }, next_cstrctr);

        // Pop the stack to the next constructor
        ty_bldr.type_stack.pop();
    }

    // todo: construct params for compound statement dv call

    // Then make call
    dv_call(CmpdStmtDoVisitParam(), node.body);
}

void Elaborator::do_visit(TypeDeclaration& node) {
    bsv_dbprint("visiting TypeDeclaration node: ", node.loc);

    auto specinfo = parse_speclist(node.specifiers);

    if (specinfo->symbol) {
        syms.insert(specinfo->symbol.value()->name, std::move(*specinfo->symbol));
    }
    dv_return(std::monostate {});
}

void Elaborator::do_visit(VariableDeclaration& node) {
    bsv_dbprint("visiting VariableDeclaration node: ", node.loc);
    auto specinfo = parse_speclist(node.specifiers);

    for (auto& declarator : node.declarators) {
        dv_call(std::monostate{}, declarator);
        auto builder = take_last_result<Box<DeclaratorBuilder>>();
        builder->ty_bldr.set_base(specinfo->type);
        Type *complete = builder->ty_bldr.finalize();
        if (builder->name) {
            Box<VarSymbol> sym = std::make_unique<VarSymbol>(declarator->loc, *builder->name, complete);
            sym->is_public = specinfo->is_public;
            sym->is_static = specinfo->is_static;
            sym->is_const = specinfo->is_const;
            sym->is_extern = specinfo->is_extern;
            syms.insert(*builder->name, std::move(sym));
        } else {
            throw EccError("variable declaration with no name", declarator->loc);
        }
    }
}

void Elaborator::do_visit(InitDeclarator& node) {
    bsv_dbprint("visiting InitDeclarator node: ", node.loc);

    dv_call(std::monostate {}, node.declarator);
    dv_return(take_last_result<Box<DeclaratorBuilder>>());
}

void Elaborator::do_visit(Declarator& node) {
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

void Elaborator::do_visit(ParenDeclarator& node) {
    bsv_dbprint("visiting ParenDeclarator node: ", node.loc);
    dv_call(std::monostate {}, node.inner);

    dv_return(take_last_result<Box<DeclaratorBuilder>>());
}

void Elaborator::do_visit(ArrayDeclarator& node) {
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

void Elaborator::do_visit(FunctionDeclarator& node) {
    bsv_dbprint("visiting FunctionDeclarator node: ", node.loc);
    dv_call(std::monostate {}, node.base);
    auto builder = take_last_result<Box<DeclaratorBuilder>>();

    Vec<FuncParam> parameters;
    for (auto& param : node.parameters) {
        dv_call(std::monostate {}, param);
        FuncParam parsed = take_last_result<FuncParam>();
        parameters.push_back(parsed);
    }

    builder->ty_bldr.add_function(parameters, node.is_variadic);

    dv_return(builder);
}

void Elaborator::do_visit(ParameterDeclaration& node) {
    /*
    dovisit_param: monostate
    last_result: FuncParam
    */
    bsv_dbprint("visiting ParameterDeclarator node: ", node.loc);
    Box<SpecifierInfo> specinfo = parse_speclist(node.specifiers);

    FuncParam ret;
    if (node.declarator) {
        dv_call(std::monostate {}, *node.declarator);
        auto builder = take_last_result<Box<DeclaratorBuilder>>();
        builder->ty_bldr.set_base(specinfo->type);

        Type *final_type = builder->ty_bldr.finalize();
        if (builder->name) {
            ret = {final_type, *builder->name};
        } else {
            ret = {final_type, {}};
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

void Elaborator::do_visit(IdentifierDeclarator& node) {
    /*
    Our base case for declarator type building.

    Return a newly constructed declarator builder.
    */
    bsv_dbprint("visiting IdentifierDeclarator node: ", node.loc);
    dv_return(std::make_unique<DeclaratorBuilder>(node.name, types.builder()));
}

void Elaborator::do_visit(Pointer& node) {
    // dovisit_param: DeclaratorBuilder *
    // last_result: monostate

    bsv_dbprint("visiting Pointer node");

    auto builder = take_dovisit_param<DeclaratorBuilder *>();

    bool is_const; // todo: populate with node qualifiers

    if (node.nested) {
        dv_call(builder, node.nested.value()); 
        builder->ty_bldr.add_pointer(is_const);
        dv_return(std::monostate {});
    } else {
        builder->ty_bldr.add_pointer(is_const);
        dv_return(std::monostate {});
    }
}

void Elaborator::do_visit(StorageClassSpecifier& node) {
    bsv_dbprint("visiting StorageClassSpecifier node: ", node.loc);
    /* terminal node */
    last_result = node.type;
}

void Elaborator::do_visit(PrimitiveSpecifier& node) {
    bsv_dbprint("visiting PrimitiveSpecifier node: ", node.loc);
    /* terminal node */

    using PK = PrimitiveSpecifier::PrimKind;
    using PTK = PrimitiveType::Kind;

    switch (node.pkind) {
        case PK::VOID:
        last_result = types.get_primitive(PTK::U0);
        break;

        case PK::U0:
        last_result = types.get_primitive(PTK::U0);
        break;

        case PK::U8:
        last_result = types.get_primitive(PTK::U8);
        break;

        case PK::U16:
        last_result = types.get_primitive(PTK::U16);
        break;

        case PK::U32:
        last_result = types.get_primitive(PTK::U32);
        break;

        case PK::U64:
        last_result = types.get_primitive(PTK::U64);
        break;

        case PK::I0:
        last_result = types.get_primitive(PTK::I0);
        break;

        case PK::I8:
        last_result = types.get_primitive(PTK::I8);
        break;

        case PK::I16:
        last_result = types.get_primitive(PTK::I16);
        break;

        case PK::I32:
        last_result = types.get_primitive(PTK::I32);
        break;

        case PK::I64:
        last_result = types.get_primitive(PTK::I64);
        break;

        case PK::F64:
        last_result = types.get_primitive(PTK::F64);
        break;

        case PK::BOOL:
        last_result = types.get_primitive(PTK::BOOL);
        break;
    }
}

void Elaborator::do_visit(TypeQualifier& node) {
    bsv_dbprint("visiting TypeQualifier node: ", node.loc);
    /* terminal node */
    last_result = node.qual;
}

void Elaborator::do_visit(EnumSpecifier& node) {
    bsv_dbprint("visiting EnumSpecifier node: ", node.loc);
    EnumType *enm = nullptr;
    if (node.name) {
        enm = types.get_enum(*(node.name), syms.current);
    } else {
        enm = types.get_enum(syms.current);
    }

    if (!enm) {
        // error: type with name already exists but is not class
        throw ecc::EccError("enum already declared as another type", node.loc);
    }

    TypeSpecRet<EnumType> ret({}, enm);

    if (node.enumerators) {
        if (enm->complete) {
            throw EccTypeAlreadyDefinedError("enum was previously defined", node.loc, enm->loc);
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

void Elaborator::do_visit(Enumerator& node) {
    bsv_dbprint("visiting Enumerator node with name ", node.name);
    EnumType *enm = take_dovisit_param<EnumType *>();

    if (auto mem = enm->contains(node.name)) {
        // todo: throw error: name already taken
    }

    if (node.value) {
        exec::Evaluator evalr(syms, types);
        exec::Value value = node.value.value()->accept(evalr);
        std::optional<long> val = value.value_as<long>();
        if (val) {
            enm->add_enumerator(node.name, *val, node.loc);
        } else {
            throw EccError("invalid enumerator value", node.loc);
        }
    } else {
        enm->add_enumerator(node.name, node.loc);
    }

    syms.insert(node.name, std::make_unique<VarSymbol>(node.loc, node.name, enm));

    dv_return(std::monostate {});
}

void Elaborator::do_visit(ClassSpecifier& node) {
    bsv_dbprint("visiting ClassSpecifier node: ", node.loc);
    ClassType *cls = nullptr;
    if (node.name) {
        cls = types.get_class(*(node.name), syms.current);
    } else {
        cls = types.get_class(syms.current);
    }

    if (!cls) {
        // error: type with name already exists but is not class
        throw ecc::EccError("class already declared as another type", node.loc);
    }

    TypeSpecRet<ClassType> ret({}, cls);

    if (node.declarations) {
        if (cls->complete) {
            // error: class was previously defined
            throw EccTypeAlreadyDefinedError("class was previously defined", node.loc, cls->loc);
        }

        // class is defined here, populate its members and mark it complete
        // todo: populate class
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

void Elaborator::do_visit(UnionSpecifier& node) {
    bsv_dbprint("visiting UnionSpecifier node ", node.loc);
    UnionType *unn = nullptr;
    if (node.name) {
        unn = types.get_union(*(node.name), syms.current);
    } else {
        unn = types.get_union(syms.current);
    }

    if (!unn) { // todo: make this throw an error instead of just returning nullptr
        // error: type with name already exists but is not union
        throw ecc::EccError("union already declared as another type", node.loc);
    }

    TypeSpecRet<UnionType> ret({}, unn);

    // declarations are present, start definition
    if (node.declarations) {
        if (unn->complete) {
            // error: union was previously defined
            throw EccTypeAlreadyDefinedError("union was previously defined", node.loc, unn->loc);
        }
        // class is defined here, populate its members and mark it complete
        // todo: populate union
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

void Elaborator::do_visit(ClassDeclaration& node) {
    bsv_dbprint("visiting ClassDeclaration node: ", node.loc);
    Box<SpecifierInfo> specinfo = parse_speclist(node.specifiers);

    for (auto& decltr : node.declarators) {
        dv_call(std::monostate {}, decltr);

        // todo
    }
}

void Elaborator::do_visit(ClassDeclarator& node) {
    bsv_dbprint("visiting ClassDeclarator node: ", node.loc);
    if (node.declarator) {
        dv_call(std::monostate {}, node.declarator.value());
        dv_return(take_last_result<Box<DeclaratorBuilder>>());
    } else {
        dv_return(std::monostate{});
    }

    // fixme: ignoring bit width for now, implement this when able
}

void Elaborator::do_visit(Initializer& node) {
    bsv_dbprint("visiting Initializer node: ", node.loc);
}

void Elaborator::do_visit(TypeName& node) {}

void Elaborator::do_visit(ExpressionStatement& node) {}

void Elaborator::do_visit(CompoundStatement& node) {
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

    if (add_symbols) {
        bsv_dbprint("CmpdStmtDoVisitParams found to have value, checking for function");
        // check the immediate outer node is a function
        if (in_node(ASTNode::NodeKind::FUNC) != 1) {
            throw std::runtime_error("received symbols to add when not in function");
        }

        // Tie the function symbol to our current scope
        syms.tie_current_to(add_symbols->first);

        // Add function argument symbols to our current scope
        for (auto& sym : add_symbols->second) {
            syms.insert(sym.first, std::move(sym.second));
        }
    }

    for (auto& item : node.items) {
        dv_call(std::monostate {}, item);
    }
}

void Elaborator::do_visit(LabeledStatement& node) {
    bsv_dbprint("visiting LabeledStatement node: ", node.loc);
    syms.insert(node.label, std::make_unique<sym::LabelSymbol>(node.loc, node.label));
    dv_call(std::monostate {}, node.statement);
}

void Elaborator::do_visit(ast::CaseStatement& node) {}

void Elaborator::do_visit(ast::CaseRangeStatement& node) {}

void Elaborator::do_visit(ast::DefaultStatement& node) {}

void Elaborator::do_visit(ast::PrintStatement& node) {}

void Elaborator::do_visit(ast::IfStatement& node) {}

void Elaborator::do_visit(ast::SwitchStatement& node) {}

void Elaborator::do_visit(ast::WhileStatement& node) {}

void Elaborator::do_visit(ast::DoWhileStatement& node) {}

void Elaborator::do_visit(ast::ForStatement& node) {}

void Elaborator::do_visit(ast::GotoStatement& node) {}

void Elaborator::do_visit(ast::BreakStatement& node) {}

void Elaborator::do_visit(ast::ReturnStatement& node) {}

void Elaborator::do_visit(ast::BinaryExpression& node) {}

void Elaborator::do_visit(ast::CastExpression& node) {}

void Elaborator::do_visit(ast::UnaryExpression& node) {}

void Elaborator::do_visit(ast::AssignmentExpression& node) {}

void Elaborator::do_visit(ast::ConditionalExpression& node) {}

void Elaborator::do_visit(ast::IdentifierExpression& node) {}

void Elaborator::do_visit(ast::ConstExpression& node) {}

void Elaborator::do_visit(ast::LiteralExpression& node) {}

void Elaborator::do_visit(ast::StringExpression& node) {}

void Elaborator::do_visit(ast::CallExpression& node) {}

void Elaborator::do_visit(ast::MemberAccessExpression& node) {}

void Elaborator::do_visit(ast::ArraySubscriptExpression& node) {}

void Elaborator::do_visit(ast::PostfixExpression& node) {}

void Elaborator::do_visit(ast::SizeofExpression& node) {}