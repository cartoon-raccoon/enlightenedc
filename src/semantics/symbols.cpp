#include <cassert>

#include "symbols.hpp"

using namespace ecc::sema::sym;

void SymbolTable::push_scope(Symbol *assoc) {
    /*
    Create a new scope, push it onto the current one, replace current scope with
    the new one
    */

    Box<Scope> newscope = std::make_unique<Scope>(assoc, current);

    Scope *new_current = newscope.get();

    current->nested.push_back(std::move(newscope));

    current = new_current;
}

void SymbolTable::enter_scope() {
    /*
    Enter the currently indexed scope in current scope.

    Throw error if no scopes exist to enter.
    */
    // todo
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