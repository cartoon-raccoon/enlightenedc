#include "elaborator.hpp"
#include "ast/ast.hpp"
#include "semantics/symbols.hpp"
#include "error.hpp"
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

Box<Elaborator::SpecifierInfo> Elaborator::parse_and_verify_speclist(Vec<Box<ast::DeclarationSpecifier>>& speclist) {
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

            case NK::CLASS_SPEC:
            specinfo->type = take_last_result<ClassType *>();
            break;

            case NK::UNION_SPEC:
            specinfo->type = take_last_result<UnionType *>();
            break;

            case NK::ENUM_SPEC:
            specinfo->type = take_last_result<EnumType *>();
            break;

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

    bsv_dbprint("visiting Function node");

    // Parse and construct specifier info
    Box<SpecifierInfo> specinfo = parse_and_verify_speclist(node.decl_spec_list);
    BaseType *return_base = specinfo->type;

    // Visit the Declarator to construct the type builder.
    dv_call(std::monostate {}, node.declarator);
    auto builder = take_last_result<Box<DeclaratorBuilder>>();
    builder->ty_bldr.set_base(return_base);

    // Extract the TypeBuiilder
    TypeBuilder& ty_bldr = builder->ty_bldr;

    // The latest function parameters.
    Vec<TypeBuilder::TypedIdent> last_func_params;

    // Extract the base type from our builder.
    Type *curr = ty_bldr.base;

    while (!ty_bldr.type_stack.empty()) {
        auto next_cstrctr = ty_bldr.type_stack.top();
        std::visit(overloaded{
            [ty_bldr, curr] (TypeBuilder::Arr& a) mutable {
                // Wrap the base in an array.
                curr = ty_bldr.ctxt.get_array(curr, a.size);
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
                    params.push_back(param.first);
                }

                last_func_params = std::move(fn.params);
                // Wrap the base as the return type in a function type.
                curr = ty_bldr.ctxt.get_function(curr, std::move(params), fn.variadic);
            }
        }, next_cstrctr);

        // Pop the stack to the next constructor
        ty_bldr.type_stack.pop();
    }

    // Then make call
    dv_call(CmpdStmtDoVisitParam(), node.body);
}

void Elaborator::do_visit(TypeDeclaration& node) {
    bsv_dbprint("visiting TypeDeclaration node");
    // The last element of the specifiers should be the type specifier,
    // and there should only be ony type specifier.
    auto specinfo = parse_and_verify_speclist(node.specifiers);

    // todo: create symbol, associate current scope with symbol
}

void Elaborator::do_visit(VariableDeclaration& node) {
    bsv_dbprint("visiting VariableDeclaration node");
    auto specinfo = parse_and_verify_speclist(node.specifiers);

    for (auto& declarator : node.declarators) {
        dv_call(std::monostate{}, declarator);
        auto builder = take_last_result<Box<DeclaratorBuilder>>();
        builder->ty_bldr.set_base(specinfo->type);
        Type *complete = builder->ty_bldr.finalize();
        // todo: account for storage class specs

    }
}

void Elaborator::do_visit(InitDeclarator& node) {
    bsv_dbprint("visiting InitDeclarator node");

    
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
    bsv_dbprint("visiting ParenDeclarator node");
    dv_call(std::monostate {}, node.inner);

    dv_return(take_last_result<Box<DeclaratorBuilder>>());
}

void Elaborator::do_visit(ArrayDeclarator& node) {
    bsv_dbprint("visiting ArrayDeclarator node");
    dv_call(std::monostate {}, node.base);

    auto builder = take_last_result<Box<DeclaratorBuilder>>();

    exec::Value size;
    exec::Evaluator evalr(syms, types);
    if (node.size) {
        size = node.size.value().get()->accept(evalr);
    } else {
        size = std::monostate();
    }

    builder->ty_bldr.add_array(size);

    dv_return(builder);
}

void Elaborator::do_visit(FunctionDeclarator& node) {
    bsv_dbprint("visiting FunctionDeclarator node");
    dv_call(std::monostate {}, node.base);
    auto builder = take_last_result<Box<DeclaratorBuilder>>();

    // todo: construct parameters
    for (auto& param : node.parameters) {
        dv_call(std::monostate {}, param);


    }

    dv_return(builder);
}

void Elaborator::do_visit(ParameterDeclaration& node) {
    bsv_dbprint("visiting ParameterDeclarator node");
    Box<SpecifierInfo> specinfo = parse_and_verify_speclist(node.specifiers);
}

void Elaborator::do_visit(IdentifierDeclarator& node) {
    /*
    Our base case for declarator type building.

    Return a newly constructed declarator builder.
    */

    dv_return(std::make_unique<DeclaratorBuilder>(node.name, types.builder()));
}

void Elaborator::do_visit(Pointer& node) {
    // dovisit_param: DeclaratorBuilder *
    // last_result: monostate

    bsv_dbprint("visiting Pointer node");

    auto builder = take_dovisit_param<DeclaratorBuilder *>();

    bool is_const; // todo: populate with node qualifiers

    // todo: account for type qualifiers
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
    bsv_dbprint("visiting StorageClassSpecifier node");
    /* terminal node */
    last_result = node.type;
}

void Elaborator::do_visit(PrimitiveSpecifier& node) {
    bsv_dbprint("visiting PrimitiveSpecifier node");
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
    bsv_dbprint("visiting TypeQualifier node");
    /* terminal node */
    last_result = node.qual;
}

void Elaborator::do_visit(EnumSpecifier& node) {
    bsv_dbprint("visiting EnumSpecifier node");
}

void Elaborator::do_visit(Enumerator& node) {
    bsv_dbprint("visiting Enumerator node");
}

void Elaborator::do_visit(ClassSpecifier& node) {
    ClassType *cls = nullptr;
    if (node.name) {
        cls = types.get_class(*(node.name), syms.current);
    } else {
        cls = types.get_class(syms.current);
    }

    if (!cls) {
        // error: type with name already exists but is not class
        throw ecc::EccError("class already declared as another type");
    }

    if (node.declarations) {
        if (cls->complete) {
            // error: class was previously defined
            throw ecc::EccError("class was previously declared");
        }

        // class is defined here, populate its members and mark it complete
        // todo: populate class
        for (auto& decl : *node.declarations) {
            dv_call(cls, decl);
        }

        cls->complete = true;
    }
    
    if (node.name) {
        Symbol *sym = syms.insert(*node.name, std::make_unique<sym::TypeSymbol>(cls));
        syms.tie_current_to(sym);
    }

    dv_return(cls);
}

void Elaborator::do_visit(UnionSpecifier& node) {
    UnionType *unn = nullptr;
    if (node.name) {
        unn = types.get_union(*(node.name), syms.current);
    } else {
        unn = types.get_union(syms.current);
    }

    if (!unn) { // todo: make this throw an error instead of just returning nullptr
        // error: type with name already exists but is not union
        throw ecc::EccError("union already declared as another type");
    }

    if (node.declarations) {
        if (unn->complete) {
            // error: union was previously defined
            throw ecc::EccError("union was previously declared");
        }
        // class is defined here, populate its members and mark it complete
        // todo: populate union
        for (auto& decl : *node.declarations) {
            dv_call(unn, decl);
        }

        unn->complete = true;
    }

    if (node.name) {
        Symbol* sym = syms.insert(*node.name, std::make_unique<sym::TypeSymbol>(unn));
        syms.tie_current_to(sym);
    }

    dv_return(unn);
}

void Elaborator::do_visit(ClassDeclaration& node) {
    Box<SpecifierInfo> specinfo = parse_and_verify_speclist(node.specifiers);

    for (auto& decltr : node.declarators) {
        dv_call(std::monostate {}, decltr);

         // todo
    }
}

void Elaborator::do_visit(ClassDeclarator& node) {
    if (node.declarator) {
        dv_call(std::monostate {}, node.declarator.value());
        dv_return(take_last_result<Box<DeclaratorBuilder>>());
    } else {
        dv_return(std::monostate{});
    }

    // ignore bit width for now
}

void Elaborator::do_visit(Initializer& node) {}

void Elaborator::do_visit(TypeName& node) {}

void Elaborator::do_visit(ExpressionStatement& node) {}

void Elaborator::do_visit(CompoundStatement& node) {
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
            add_symbols = std::move(param);
        },
        [&add_symbols] (auto _) mutable {
            add_symbols = {};
        }
    }, dovisit_param);

    if (add_symbols) {
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
    syms.insert(node.label, std::make_unique<sym::LabelSymbol>());
    dv_call(std::monostate {}, node.statement);
}