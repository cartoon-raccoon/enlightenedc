#include <cassert>
#include <stdexcept>

#include "symbols.hpp"

using namespace ecc::sema::sym;
using namespace ecc::sema::types;

Box<VarSymbol> FuncSymbol::as_funcptr(TypeContext& tctxt, bool is_const) {
    Type *ptrtype = tctxt.get_pointer(signature, is_const);

    return std::make_unique<VarSymbol>(loc, name, ptrtype);
}

void SymbolTable::push_scope(Symbol *assoc) {
    /*
    Create a new scope, push it onto the current one, replace current scope with
    the new one
    */

    Box<Scope> newscope = std::make_unique<Scope>(assoc, current);
    dbprint("SymbolTable: pushing scope ", newscope.get());
    current->nested.push_back(std::move(newscope));

    enter_scope();
}

void SymbolTable::enter_scope() {
    // If there are no scopes left to enter
    if (current->nested.empty()) {
        throw std::runtime_error("tried to enter nonexistent nested scope");
    }

    if (current->nested_idx >= current->nested.size()) {
        throw std::runtime_error("no more nested scopes left to enter in current scope");
    }

    Scope *new_current = current->nested[current->nested_idx].get();
    dbprint("SymbolTable: entering scope ", new_current);

    current = new_current;

    dbprint("SymbolTable: current scope ", current);
}

void SymbolTable::pop_scope() {
    if (current != global.get()) {
        if (current->outer) {
            dbprint("SymbolTable: exiting scope to ", current->outer);
            current = current->outer;
            current->nested_idx++;
        } else {
            throw std::runtime_error("tried to exit global scope");
        }
    }

    dbprint("SymbolTable: current scope ", current);
}

void SymbolTable::reset() {
    reset_from(global.get());

    current = global.get();
}

void SymbolTable::reset_from(Scope *scope) {
    scope->nested_idx = 0;
    for (auto& scope : scope->nested) {
        reset_from(scope.get());
    }
}

void SymbolTable::reset_current() {
    current->nested_idx = 0;
}


void SymbolTable::clear() {
    // todo
}


Symbol *SymbolTable::lookup(std::string sym) {
    dbprint("SymbolTable: looking up symbol ", sym);

    Scope *my_current = this->current;

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
    dbprint("SymbolTable: associating current scope ", current, " with symbol ", sym, " name \"", sym->name, "\"");
    if (current->assoc != nullptr) {
        if (override) {
            current->assoc = sym;
        }
    } else {
        current->assoc = sym;
    }
}

Symbol *SymbolTable::insert(std::string name, Box<Symbol> sym) {
    dbprint("SymbolTable: inserting symbol with name \"", name, "\"");
    Symbol *ret = sym.get();
    current->symbols.insert({name, std::move(sym)});

    return ret;
}