#include "elaborator.hpp"
#include "ast/ast.hpp"
#include "semantics/symbols.hpp"
#include "error.hpp"
#include "semantics/types.hpp"

#include <stdexcept>
#include <cassert>

#define dv_return(val) do { last_result = std::move(val); return; } while (0)

#define dv_call(param, obj) do { dovisit_param = std::move(param); (obj)->accept(*this); } while (0)

using namespace ecc::ast;
using namespace ecc::sema;
using namespace ecc::sema::types;
using namespace ecc::sema::sym;

Box<Elaborator::SpecifierInfo> Elaborator::parse_and_verify_speclist(Vec<Box<ast::DeclarationSpecifier>>& speclist) {
    return nullptr; // todo
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
    using NK = ASTNode::NodeKind;

    BaseType * return_base = nullptr;

    for (auto& decl_spec : node.decl_spec_list) {

        decl_spec->accept(*this);
        switch (node.kind) {
            case NK::TYPE_QUAL: // const

            break;

            case NK::STORAGE_SPEC: // 

            break;

            case NK::CLASS_SPEC:
            return_base = take_dovisit_param<ClassType *>();
            break;

            case NK::UNION_SPEC:
            return_base = take_dovisit_param<UnionType *>();
            break;

            case NK::ENUM_SPEC:
            return_base = take_dovisit_param<EnumType *>();
            break;

            case NK::PRIM_SPEC:
            return_base = take_dovisit_param<PrimitiveType *>();
            break;

            default:
            throw std::runtime_error("encountered a non-declaration specifier while visiting function node");
        }
    }

    assert(return_base);

    dv_call(std::monostate {}, node.declarator);
    auto builder = take_last_result<Box<DeclaratorBuilder>>();
    builder->ty_bldr.set_base(return_base);

    // finalize the function declarator to derive the final function type.
    FunctionType *complete_type = builder->ty_bldr.finalize()->as_function();
    if (!complete_type /* || complete_type->kind != Type::FUNCTION */) {
        // todo: throw error
    }

    // Then make call
    node.body->accept(*this);
}

void Elaborator::do_visit(TypeDeclaration& node) {
    // The last element of the specifiers should be the type specifier,
    // and there should only be ony type specifier.
    for (auto& specifier : node.specifiers) {
        specifier->accept(*this);
    }

    // todo: create symbol, associate current scope with symbol
}

void Elaborator::do_visit(VariableDeclaration& node) {
    for (auto& specifier : node.specifiers) {
        specifier->accept(*this);
    }

    for (auto& declarator : node.declarators) {
        declarator->accept(*this);
    }
}

void Elaborator::do_visit(InitDeclarator& node) {

}

void Elaborator::do_visit(ParameterDeclaration& node) {

}

void Elaborator::do_visit(Declarator& node) {
    Box<DeclaratorBuilder> builder;
    if (node.direct) {
        dv_call(std::monostate {}, node.direct.value());
        builder = take_last_result<Box<DeclaratorBuilder>>();
    } else {
        // no direct declarator, assume abstract
        builder = std::make_unique<DeclaratorBuilder>("", types.builder());
    }
    if (node.pointer.has_value()) {
        dv_call(builder.get(), node.pointer.value());
    }

    dv_return(builder);
}

void Elaborator::do_visit(ParenDeclarator& node) {
    dv_call(std::monostate {}, node.inner);

    dv_return(take_last_result<Box<DeclaratorBuilder>>());
}

void Elaborator::do_visit(ArrayDeclarator& node) {
    dv_call(std::monostate {}, node.base);

    auto builder = take_last_result<Box<DeclaratorBuilder>>();

    ConstExpression *size_expr;
    if (node.size) {
        size_expr = node.size.value().get();
    } else {
        size_expr = nullptr;
    }

    builder->ty_bldr.add_array(size_expr);
}

void Elaborator::do_visit(FunctionDeclarator& node) {
    dv_call(std::monostate {}, node.base);

    auto builder = take_last_result<Box<DeclaratorBuilder>>();

    // todo: construct parameters

    dv_return(builder);
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

    auto builder = take_dovisit_param<DeclaratorBuilder *>();

    // todo: account for type qualifiers
    if (node.nested) {
        dv_call(builder, node.nested.value()); 
        builder->ty_bldr.add_pointer(false /* fixme */);
        dv_return(std::monostate {});
    } else {
        builder->ty_bldr.add_pointer(false);
        dv_return(std::monostate {});
    }
}

void Elaborator::do_visit(StorageClassSpecifier& node) {
    /* terminal node */
    last_result = node.type;
}

void Elaborator::do_visit(PrimitiveSpecifier& node) {
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
    /* terminal node */
    last_result = node.qual;
}

void Elaborator::do_visit(EnumSpecifier& node) {
    
}

void Elaborator::do_visit(Enumerator& node) {

}

void Elaborator::do_visit(ClassSpecifier& node) {
    // any nested derived types have to be scoped within this specifier.
    auto guard = enter_scope();

    ClassType *cls = nullptr;
    if (node.name.has_value()) {
        cls = types.get_class(*(node.name), syms.current);
    } else {
        cls = types.get_class(syms.current);
    }

    if (!cls) {
        // error: type with name already exists but is not class
        throw ecc::EccError("class already declared as another type");
    }

    if (node.declarations.has_value()) {
        if (cls->complete) {
            // error: class was previously defined
            throw ecc::EccError("class was previously declared");
        }

        // class is defined here, populate its members and mark it complete
        // todo: populate class
        for (auto& decl : *node.declarations) {
            dovisit_param = cls;
            decl->accept(*this);
        }

        cls->complete = true;
    }

    last_result = cls;
    
    if (node.name.has_value()) {
        Symbol *sym = syms.insert(*node.name, std::make_unique<sym::TypeSymbol>(cls));
        syms.tie_current_to(sym);
    }
}

void Elaborator::do_visit(UnionSpecifier& node) {
    UnionType *unn = nullptr;
    if (node.name.has_value()) {
        unn = types.get_union(*(node.name), syms.current);
    } else {
        unn = types.get_union(syms.current);
    }

    if (!unn) {
        // error: type with name already exists but is not union
        throw ecc::EccError("union already declared as another type");
    }

    if (node.declarations.has_value()) {
        if (unn->complete) {
            // error: union was previously defined
            throw ecc::EccError("union was previously declared");
        }
        // class is defined here, populate its members and mark it complete
        // todo: populate union
        for (auto& decl : *node.declarations) {
            dovisit_param = unn;
            decl->accept(*this);
        }

        unn->complete = true;
    }

    last_result = unn;

    if (node.name.has_value()) {
        Symbol* sym = syms.insert(*node.name, std::make_unique<sym::TypeSymbol>(unn));
        syms.tie_current_to(sym);
    }
}

void Elaborator::do_visit(ClassDeclaration& node) {
    for (auto& spec : node.specifiers) {

    }

    for (auto& decltr : node.declarators) {

    }
}

void Elaborator::do_visit(ClassDeclarator& node) {

}

void Elaborator::do_visit(Initializer& node) {}

void Elaborator::do_visit(TypeName& node) {}

void Elaborator::do_visit(ExpressionStatement& node) {}

void Elaborator::do_visit(CompoundStatement& node) {
    CmpdStmtDoVisitParam add_symbols;

    // We can fail silently if the parameter isn't correct.
    try {
        add_symbols = take_dovisit_param<CmpdStmtDoVisitParam>();
    } catch (...) { // fixme: catch specific exception
        add_symbols = {};
    }

    if (add_symbols.has_value()) {
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
        item->accept(*this);
    }
}

void Elaborator::do_visit(LabeledStatement& node) {
    syms.insert(node.label, std::make_unique<sym::LabelSymbol>());
    node.statement->accept(*this);
}

void Elaborator::do_visit(WhileStatement& node) {}

void Elaborator::do_visit(DoWhileStatement& node) {}

void Elaborator::do_visit(ForStatement& node) {}

void Elaborator::do_visit(GotoStatement& node) {

}