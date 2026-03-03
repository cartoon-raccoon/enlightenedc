#include <cassert>
#include <memory>
#include <stdexcept>

#include "compiler/semantics.hpp"
#include "compiler/elaborator.hpp"

using namespace ecc::compiler;

void SymbolTable::push_scope(Symbol *assoc) {
    /*
    Create a new scope, push it onto the current one, replace current scope with
    the new one
    */

    Box<Scope> newscope = std::make_unique<Scope>(assoc, current);

    Scope *new_current = newscope.get();

    current->inners.push_back(std::move(newscope));

    current = new_current;
}

void SymbolTable::pop_scope() {
    if (current != global.get()) {
        if (current->outer) {
            current = current->outer;
        } else {
            throw std::runtime_error("tried to exit global scope (this is a bug)");
        }
    }
}

Symbol *SymbolTable::lookup(std::string sym) {

    Scope *my_current = current;

    // look for symbol in current scope
    while (!(my_current->symbols.contains(sym))) {
        // if already global, return null
        if (my_current->outer == nullptr) {
            assert(my_current == global.get());
            return nullptr;
        }

        // move to outer
        my_current = my_current->outer;
    }

    return my_current->symbols.find(sym)->second.get();
}

void SymbolTable::tie_current_to(Symbol *sym, bool override) {
    if (current->assoc != nullptr) {
        if (override) {
            current->assoc = sym;
        }
    } else {
        current->assoc = sym;
    }
}

Symbol *SymbolTable::insert(std::string name, Box<Symbol> sym) {
    Symbol *ret = sym.get();
    current->symbols.insert({name, std::move(sym)});

    return ret;
}

BaseSemanticVisitor::ScopeGuard BaseSemanticVisitor::enter_scope(Symbol *assoc) {
    return ScopeGuard(syms, assoc);
}

/*
* VISIT METHODS
*/

void BaseSemanticVisitor::visit(Program& node) {
    for (auto& item : node.items) {
        item->accept(*this);
    }
}

void BaseSemanticVisitor::visit(Function& node) {
    for (auto& decl_spec : node.decl_spec_list) {
        decl_spec->accept(*this);
    }

    node.declarator->accept(*this);
    node.body->accept(*this);
}

void BaseSemanticVisitor::visit(TypeDeclaration& node) {
    // The last element of the specifiers should be the type specifier,
    // and there should only be ony type specifier.
    for (auto& specifier : node.specifiers) {
        specifier->accept(*this);
    }
}

void BaseSemanticVisitor::visit(VariableDeclaration& node) {
    for (auto& specifier : node.specifiers) {
        specifier->accept(*this);
    }

    for (auto& declarator : node.declarators) {
        declarator->accept(*this);
    }
}

void BaseSemanticVisitor::visit(ParameterDeclaration& node) {}

void BaseSemanticVisitor::visit(Declarator& node) {}

void BaseSemanticVisitor::visit(ParenDeclarator& node) {}

void BaseSemanticVisitor::visit(ArrayDeclarator& node) {}

void BaseSemanticVisitor::visit(FunctionDeclarator& node) {}

void BaseSemanticVisitor::visit(InitDeclarator& node) {}

void BaseSemanticVisitor::visit(Pointer& node) {}

void BaseSemanticVisitor::visit(ClassDeclarator& node) {}

void BaseSemanticVisitor::visit(ClassDeclaration& node) {}

void BaseSemanticVisitor::visit(Enumerator& node) {}

void BaseSemanticVisitor::visit(StorageClassSpecifier& node) {}

/*
Visit a type specifier, adding 
*/
void BaseSemanticVisitor::visit(TypeSpecifier& node) {
    // the new type gets added here.
}

void BaseSemanticVisitor::visit(TypeQualifier& node) {}

void BaseSemanticVisitor::visit(EnumSpecifier& node) {
    // no enter scope here, enumerators are scoped to the scope in which
    // their corresponding enum is declared.
}

void BaseSemanticVisitor::visit(ClassOrUnionSpecifier& node) {
    // any nested derived types have to be scoped within this specifier.
    auto guard = enter_scope();


}

void BaseSemanticVisitor::visit(Initializer& node) {}

void BaseSemanticVisitor::visit(TypeName& node) {}

void BaseSemanticVisitor::visit(IdentifierDeclarator& node) {}

void BaseSemanticVisitor::visit(CompoundStatement& node) {
    // compound statements should introduce a new scope.
    auto guard = enter_scope();

    for (auto& item : node.items) {
        item->accept(*this);
    }
}

void BaseSemanticVisitor::visit(ExpressionStatement& node) {}

void BaseSemanticVisitor::visit(CaseDefaultStatement& node) {}

void BaseSemanticVisitor::visit(LabeledStatement& node) {
    node.statement->accept(*this);
}

void BaseSemanticVisitor::visit(PrintStatement& node) {}

void BaseSemanticVisitor::visit(IfStatement& node) {
    node.condition->accept(*this);

    node.then_branch->accept(*this);
    if (node.else_branch.has_value()) {
        node.else_branch.value()->accept(*this);
    }
}

void BaseSemanticVisitor::visit(SwitchStatement& node) {
    node.condition->accept(*this);
    node.body->accept(*this);
}

void BaseSemanticVisitor::visit(WhileStatement& node) {}

void BaseSemanticVisitor::visit(DoWhileStatement& node) {}

void BaseSemanticVisitor::visit(ForStatement& node) {}

void BaseSemanticVisitor::visit(JumpStatement& node) {}

void BaseSemanticVisitor::visit(GotoStatement& node) {

}

void BaseSemanticVisitor::visit(BreakStatement& node) {

}

void BaseSemanticVisitor::visit(ReturnStatement& node) {
    
}

void BaseSemanticVisitor::visit(BinaryExpression& node) {}

void BaseSemanticVisitor::visit(UnaryExpression& node) {}

void BaseSemanticVisitor::visit(AssignmentExpression& node) {}

void BaseSemanticVisitor::visit(ConditionalExpression& node) {}

void BaseSemanticVisitor::visit(IdentifierExpression& node) {}

void BaseSemanticVisitor::visit(LiteralExpression& node) {}

void BaseSemanticVisitor::visit(StringExpression& node) {}

void BaseSemanticVisitor::visit(CallExpression& node) {}

void BaseSemanticVisitor::visit(MemberAccessExpression& node) {}

void BaseSemanticVisitor::visit(ArraySubscriptExpression& node) {}

void BaseSemanticVisitor::visit(PostfixExpression& node) {}

void BaseSemanticVisitor::visit(SizeofExpression& node) {}


void SemanticChecker::check_semantics(ASTNode& prog) {
    Elaborator elaborator(*symbols, *types);
    prog.accept(elaborator);

    Validator validator(*symbols, *types);
    prog.accept(validator);
}