#include <cassert>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "symbols.hpp"

using namespace ecc::sema::sym;
using namespace ecc::sema::types;

std::string VarSymbol::mangle() const {
    std::stringstream ss;
    ss << "var_" << name << "_" << "s" << scope->id;
    return ss.str();
}

std::string FuncSymbol::mangle() const {
    std::stringstream ss;
    ss << "func_" << name << "_" << "s" << scope->id;
    return ss.str();
}

std::string TypeSymbol::mangle() const {
    std::stringstream ss;
    ss << "type_" << name << "_" << "s" << scope->id;
    return ss.str();
}

std::string LabelSymbol::mangle() const {
    std::stringstream ss;
    ss << "label_" << name << "_" << "s" << scope->id;
    return ss.str();
}

Box<VarSymbol> FuncSymbol::as_funcptr(TypeContext& tctxt, bool is_const) {
    Type *ptrtype = tctxt.get_pointer(signature, is_const);

    return std::make_unique<VarSymbol>(loc, name, scope, ptrtype);
}

void SymbolTable::push_scope(Symbol *assoc) {
    /*
    Create a new scope, push it onto the current one, replace current scope with
    the new one
    */

    Box<Scope> newscope = std::make_unique<Scope>(assoc, current, next_id);
    next_id++;

    dbprint("SymbolTable: pushing scope ", newscope->id);
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
    dbprint("SymbolTable: entering scope ", new_current->id);

    current = new_current;
}

void SymbolTable::pop_scope() {
    if (current != global.get()) {
        if (current->outer) {
            dbprint("SymbolTable: exiting scope to ", current->outer->id);
            current = current->outer;
            current->nested_idx++;
        } else {
            throw std::runtime_error("tried to exit global scope");
        }
    }
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

Symbol *SymbolTable::lookup(std::string& sym, bool current_only) {
    VarSymbol *maybe_var = lookup_var(sym, current_only);
    if (maybe_var)
        return maybe_var;

    FuncSymbol *maybe_func = lookup_func(sym, current_only);
    if (maybe_func)
        return maybe_func;

    TypeSymbol *maybe_type = lookup_type(sym, current_only);
    if (maybe_type)
        return maybe_type;

    LabelSymbol *maybe_label = lookup_label(sym, current_only);
    if (maybe_label)
        return maybe_label;

    return nullptr;
}

VarSymbol *SymbolTable::lookup_var(std::string& sym, bool current_only) {
    
    if (current_only) {
        dbprint("SymbolTable: looking up varsymbol ", sym, " in current scope");
        if (this->current->var_symbols.contains(sym)) {
            return this->current->var_symbols.find(sym)->second.get();
        } else {
            return nullptr;
        }
    }
    dbprint("SymbolTable: looking up varsymbol ", sym);

    Scope *my_current = this->current;

    // look for symbol in current scope
    while (!(my_current->var_symbols.contains(sym))) {
        // if already global, return null
        if (my_current->outer == nullptr) {
            assert(my_current == global.get());
            dbprint("SymbolTable: symbol \'", sym, "\' not found");
            return nullptr;
        }

        // move to outer
        my_current = my_current->outer;
    }

    return my_current->var_symbols.find(sym)->second.get();
}

FuncSymbol *SymbolTable::lookup_func(std::string& sym, bool current_only) {
    if (current_only) {
        dbprint("SymbolTable: looking up funcsymbol ", sym, " in current scope");
        if (this->current->func_symbols.contains(sym)) {
            return this->current->func_symbols.find(sym)->second.get();
        } else {
            return nullptr;
        }
    }
    dbprint("SymbolTable: looking up funcsymbol ", sym);

    Scope *my_current = this->current;

    // look for symbol in current scope
    while (!(my_current->func_symbols.contains(sym))) {
        // if already global, return null
        if (my_current->outer == nullptr) {
            assert(my_current == global.get());
            dbprint("SymbolTable: symbol \'", sym, "\' not found");
            return nullptr;
        }

        // move to outer
        my_current = my_current->outer;
    }

    return my_current->func_symbols.find(sym)->second.get();
}

TypeSymbol *SymbolTable::lookup_type(std::string& sym, bool current_only) {
    if (current_only) {
        dbprint("SymbolTable: looking up typesymbol ", sym, " in current scope");
        if (this->current->type_symbols.contains(sym)) {
            return this->current->type_symbols.find(sym)->second.get();
        } else {
            return nullptr;
        }
    }
    dbprint("SymbolTable: looking up typesymbol ", sym);

    Scope *my_current = this->current;

    // look for symbol in current scope
    while (!(my_current->type_symbols.contains(sym))) {
        // if already global, return null
        if (my_current->outer == nullptr) {
            assert(my_current == global.get());
            dbprint("SymbolTable: symbol \'", sym, "\' not found");
            return nullptr;
        }

        // move to outer
        my_current = my_current->outer;
    }

    return my_current->type_symbols.find(sym)->second.get();
}

LabelSymbol *SymbolTable::lookup_label(std::string& sym, bool current_only) {
    if (current) {
        dbprint("SymbolTable: looking up labelsymbol ", sym, " in current scope");
        if (this->current->label_symbols.contains(sym)) {
            return this->current->label_symbols.find(sym)->second.get();
        } else {
            return nullptr;
        }
    }
    dbprint("SymbolTable: looking up labelsymbol ", sym);

    Scope *my_current = this->current;

    // look for symbol in current scope
    while (!(my_current->label_symbols.contains(sym))) {
        // if already global, return null
        if (my_current->outer == nullptr) {
            assert(my_current == global.get());
            dbprint("SymbolTable: symbol \'", sym, "\' not found");
            return nullptr;
        }

        // move to outer
        my_current = my_current->outer;
    }

    return my_current->label_symbols.find(sym)->second.get();
}

void SymbolTable::tie_current_to(Symbol *sym, bool override) {
    dbprint("SymbolTable: associating current scope ", current->id, " with symbol ", sym, " name \"", sym->name, "\"");
    if (current->assoc != nullptr) {
        if (override) {
            current->assoc = sym;
        }
    } else {
        current->assoc = sym;
    }
}

VarSymbol *SymbolTable::insert(std::string name, Box<VarSymbol> sym) {
    dbprint("SymbolTable: inserting varsymbol with name \"", name, "\"");
    if (current->var_symbols.contains(name)) {
        dbprint("SymbolTable: varsymbol with name ", name, " already exists");
        Symbol *existing = current->var_symbols.find(name)->second.get();
        throw existing;
    }
    VarSymbol *ret = sym.get();
    current->var_symbols.insert({name, std::move(sym)});

    return ret;
}

FuncSymbol *SymbolTable::insert(std::string name, Box<FuncSymbol> sym) {
    dbprint("SymbolTable: inserting funcsymbol with name \"", name, "\"");
    if (current->func_symbols.contains(name)) {
        dbprint("SymbolTable: funcsymbol with name ", name, " already exists");
        Symbol *existing = current->func_symbols.find(name)->second.get();
        throw existing;
    }
    FuncSymbol *ret = sym.get();
    current->func_symbols.insert({name, std::move(sym)});

    return ret;
}

TypeSymbol *SymbolTable::insert(std::string name, Box<TypeSymbol> sym) {
    dbprint("SymbolTable: inserting typesymbol with name \"", name, "\"");
    if (current->type_symbols.contains(name)) {
        dbprint("SymbolTable: typesymbol with name ", name, " already exists");
        Symbol *existing = current->type_symbols.find(name)->second.get();
        throw existing;
    }
    TypeSymbol *ret = sym.get();
    current->type_symbols.insert({name, std::move(sym)});

    return ret;
}

LabelSymbol *SymbolTable::insert(std::string name, Box<LabelSymbol> sym) {
    dbprint("SymbolTable: inserting labelsymbol with name \"", name, "\"");
    if (current->label_symbols.contains(name)) {
        dbprint("SymbolTable: labelsymbol with name ", name, " already exists");
        Symbol *existing = current->label_symbols.find(name)->second.get();
        throw existing;
    }
    LabelSymbol *ret = sym.get();
    current->label_symbols.insert({name, std::move(sym)});

    return ret;
}