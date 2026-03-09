#include "elaborator.hpp"
#include "ast/ast.hpp"
#include "semantics/symbols.hpp"
#include "error.hpp"

using namespace ecc::ast;
using namespace ecc::sema;
using namespace ecc::sema::types;

ElabResult Elaborator::take_last_result() {
    ElabResult ret = std::move(last_result);

    last_result = std::monostate {};

    return ret;
}

void Elaborator::visit(Function& node) {
    for (auto& decl_spec : node.decl_spec_list) {
        decl_spec->accept(*this);
    }

    node.declarator->accept(*this);
    node.body->accept(*this);
}

void Elaborator::visit(TypeDeclaration& node) {
    // The last element of the specifiers should be the type specifier,
    // and there should only be ony type specifier.
    for (auto& specifier : node.specifiers) {
        specifier->accept(*this);
    }
}

void Elaborator::visit(VariableDeclaration& node) {
    for (auto& specifier : node.specifiers) {
        specifier->accept(*this);
    }

    for (auto& declarator : node.declarators) {
        declarator->accept(*this);
    }
}

void Elaborator::visit(ParameterDeclaration& node) {}

void Elaborator::visit(Declarator& node) {}

void Elaborator::visit(ParenDeclarator& node) {}

void Elaborator::visit(ArrayDeclarator& node) {}

void Elaborator::visit(FunctionDeclarator& node) {}

void Elaborator::visit(InitDeclarator& node) {}

void Elaborator::visit(Pointer& node) {
    
}

void Elaborator::visit(ClassDeclarator& node) {}

void Elaborator::visit(ClassDeclaration& node) {}

void Elaborator::visit(Enumerator& node) {}

void Elaborator::visit(StorageClassSpecifier& node) {}

/*
Visit a type specifier, adding 
*/
void Elaborator::visit(PrimitiveSpecifier& node) {
    // the new type gets added here.
}

void Elaborator::visit(TypeQualifier& node) {}

void Elaborator::visit(EnumSpecifier& node) {
    // no enter scope here, enumerators are scoped to the scope in which
    // their corresponding enum is declared.
}

void Elaborator::visit(ClassSpecifier& node) {
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

        }

        cls->complete = true;
    }

    last_result = cls;
    

    // todo: create symbol and associate current scope with it
    if (node.name.has_value()) {
        syms.insert(*node.name, std::make_unique<sym::TypeSymbol>(cls));
    }

}

void Elaborator::visit(UnionSpecifier& node) {
    UnionType *unn = nullptr;
    if (node.name.has_value()) {
        unn = types.get_union(*(node.name), syms.current);
    } else {
        unn = types.get_union(syms.current);
    }

    if (!unn) {
        // error: type with name already exists but is not union
        // todo: throw error
    }

    if (node.declarations.has_value()) {
        if (unn->complete) {
            // error: union was previously defined
            // todo: throw error
        }
        // class is defined here, populate its members and mark it complete
        // todo: populate union
        for (auto& decl : *node.declarations) {

        }

        unn->complete = true;
    }

    last_result = unn;

    // todo: create symbol and associate current scope with it
    if (node.name.has_value()) {
        syms.insert(*node.name, std::make_unique<sym::TypeSymbol>(unn));
    }
}

void Elaborator::visit(Initializer& node) {}

void Elaborator::visit(TypeName& node) {}

void Elaborator::visit(IdentifierDeclarator& node) {}

void Elaborator::visit(ExpressionStatement& node) {}

void Elaborator::visit(CaseDefaultStatement& node) {}

void Elaborator::visit(LabeledStatement& node) {
    syms.insert(node.label, std::make_unique<sym::LabelSymbol>());
    node.statement->accept(*this);
}

void Elaborator::visit(WhileStatement& node) {}

void Elaborator::visit(DoWhileStatement& node) {}

void Elaborator::visit(ForStatement& node) {}

void Elaborator::visit(GotoStatement& node) {

}

void Elaborator::visit(BreakStatement& node) {

}

void Elaborator::visit(ReturnStatement& node) {
    
}

void Elaborator::do_visit(ClassSpecifier& node) {

}

void Elaborator::do_visit(UnionSpecifier& node) {

}