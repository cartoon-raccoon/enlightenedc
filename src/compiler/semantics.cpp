#include "compiler/semantics.hpp"
#include <memory>

using namespace ecc::compiler;

void SymbolTable::push_scope(Symbol *assoc) {
    /*
    Create a new scope, push it onto the current one, replace current scope with
    the new one
    */

    Box<Scope> newscope = std::make_unique<Scope>(assoc);

    Scope *new_current = newscope.get();

    current->inners.push_back(std::move(newscope));

    current = new_current;
}

void SymbolTable::pop_scope() {
    if (current != global.get()) {
        if (current->outer) {
            current = current->outer;
        } else {
            // there is no outer scope and we are not global,
            // throw exception

            // todo
        }
    }
}

Symbol *SymbolTable::lookup(std::string sym) {
    /*
    Look up symbol in current scope. If not there, check outer.
    Repeat until current scope is global. Return null if not found
    in global.
    */
    return nullptr; // todo
}

void SymbolTable::insert(std::string name, Box<Symbol> sym) {
    current->symbols.insert({name, std::move(sym)});
}

BaseSemanticVisitor::ScopeGuard BaseSemanticVisitor::enter_scope(Symbol *assoc) {
    return ScopeGuard(syms, assoc);
}