#include "semantics/symbols.hpp"

#include <cassert>
#include <sstream>
#include <stdexcept>
#include <utility>

using namespace ecc::sema::sym;
using namespace ecc::sema::types;

std::string VarSymbol::mangle() const {
    std::stringstream ss;
    if (is_global) {
        ss << name;
    } else {
        ss << name << "_" << "s" << scope->id;
    }
    return ss.str();
}

std::string FuncSymbol::mangle() const {
    std::stringstream ss;
    if (is_global) {
        ss << name;
    } else {
        ss << name << "_" << "s" << scope->id;
    }
    return ss.str();
}

std::string TypeSymbol::mangle() const {
    std::stringstream ss;
    if (is_global) {
        ss << name;
    } else {
        ss << name << "_" << "s" << scope->id;
    }
    return ss.str();
}

std::string LabelSymbol::mangle() const {
    std::stringstream ss;
    if (is_global) {
        ss << "global_" << name;
    } else {
        if (scope->assoc) {
            ss << scope->assoc->mangle() << "_" << name;
        } else {
            ss << name << "_" << scope->id;
        }
    }
    return ss.str();
}

Box<VarSymbol> FuncSymbol::as_funcptr(TypeContext& tctxt, bool is_const) {
    Type *ptrtype = tctxt.get_pointer(signature, is_const);

    return std::make_unique<VarSymbol>(loc, name, scope, ptrtype);
}

void SymbolTable::clear() {
    // todo
}

void SymbolTableWalker::push_scope(FuncSymbol *assoc) {
    /*
    Create a new scope, push it onto the current one, replace current scope with
    the new one
    */

    Box<Scope> newscope = std::make_unique<Scope>(assoc, current, *next_id, next_scope_idx);
    next_id++;

    dbprint("SymbolTable: pushing scope ", newscope->id);
    current->nested.push_back(std::move(newscope));

    enter_scope();
}

void SymbolTableWalker::enter_scope() {
    // If there are no scopes left to enter
    if (current->nested.empty()) {
        throw std::runtime_error("tried to enter nonexistent nested scope");
    }

    if (next_scope_idx >= current->nested.size()) {
        throw std::runtime_error("no more nested scopes left to enter in current scope");
    }

    Scope *new_current = current->nested[next_scope_idx].get();
    next_scope_idx++;
    dbprint("SymbolTable: entering scope ", new_current->id);

    prev_scope_idxs.push(next_scope_idx);
    next_scope_idx = 0;

    current = new_current;
}

void SymbolTableWalker::pop_scope() {
    if (current != st.get().global.get()) {
        if (current->outer) {
            dbprint("SymbolTable: exiting scope to ", current->outer->id);
            current = current->outer;

            next_scope_idx = prev_scope_idxs.top();
            prev_scope_idxs.pop();
        } else {
            throw std::runtime_error("tried to exit global scope");
        }
    }
}

void SymbolTableWalker::reset() {
    current = global();
}

Symbol *SymbolTableWalker::lookup(std::string& sym, bool current_only) const {
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

VarSymbol *SymbolTableWalker::lookup_var(std::string& sym, bool current_only) const {
    Scope *my_current = current;
    if (current_only) {
        dbprint("SymbolTable: looking up varsymbol ", sym, " in current scope");
        if (my_current->phys_symbols.contains(sym)) {
            // this returns null if we pull a funcsymbol
            return my_current->phys_symbols.find(sym)->second->as_varsym();
        } else {
            return nullptr;
        }
    }
    dbprint("SymbolTable: looking up varsymbol ", sym);

    // look for symbol in current scope
    while (!(my_current->phys_symbols.contains(sym))) {
        // if already global, return null
        if (my_current->outer == nullptr) {
            assert(my_current == global());
            dbprint("SymbolTable: symbol \'", sym, "\' not found");
            return nullptr;
        }

        // move to outer
        my_current = my_current->outer;
    }

    return my_current->phys_symbols.find(sym)->second->as_varsym();
}

FuncSymbol *SymbolTableWalker::lookup_func(std::string& sym, bool current_only) const {
    Scope *my_current = current;
    if (current_only) {
        dbprint("SymbolTable: looking up funcsymbol ", sym, " in current scope");
        if (my_current->phys_symbols.contains(sym)) {
            return my_current->phys_symbols.find(sym)->second->as_funcsym();
        } else {
            return nullptr;
        }
    }
    dbprint("SymbolTable: looking up funcsymbol ", sym);

    // look for symbol in current scope
    while (!(my_current->phys_symbols.contains(sym))) {
        // if already global, return null
        if (my_current->outer == nullptr) {
            assert(my_current == global());
            dbprint("SymbolTable: symbol \'", sym, "\' not found");
            return nullptr;
        }

        // move to outer
        my_current = my_current->outer;
    }

    return my_current->phys_symbols.find(sym)->second->as_funcsym();
}

TypeSymbol *SymbolTableWalker::lookup_type(std::string& sym, bool current_only) const {
    Scope *my_current = current;
    if (current_only) {
        dbprint("SymbolTable: looking up typesymbol ", sym, " in current scope");
        if (my_current->type_symbols.contains(sym)) {
            return my_current->type_symbols.find(sym)->second.get();
        } else {
            return nullptr;
        }
    }
    dbprint("SymbolTable: looking up typesymbol ", sym);

    // look for symbol in current scope
    while (!(my_current->type_symbols.contains(sym))) {
        // if already global, return null
        if (my_current->outer == nullptr) {
            assert(my_current == global());
            dbprint("SymbolTable: symbol \'", sym, "\' not found");
            return nullptr;
        }

        // move to outer
        my_current = my_current->outer;
    }

    return my_current->type_symbols.find(sym)->second.get();
}

LabelSymbol *SymbolTableWalker::lookup_label(std::string& sym, bool current_only) const {
    Scope *my_current = current;

    if (current == global()) {
        current_only = true;
    }

    if (current_only) {
        dbprint("SymbolTable: looking up labelsymbol ", sym, " in current scope");
        if (my_current->label_symbols.contains(sym)) {
            return my_current->label_symbols.find(sym)->second.get();
        } else {
            return nullptr;
        }
    }
    dbprint("SymbolTable: looking up labelsymbol in function scope ", sym);

    // look for symbol up to function scope
    while (!(my_current->label_symbols.contains(sym))) {
        // if already at function scope, return null
        if (my_current->assoc) {
            dbprint("SymbolTable: symbol \'", sym, "\' not found in function scope");
            return nullptr;
        }

        // move to outer
        my_current = my_current->outer;
    }

    return my_current->label_symbols.find(sym)->second.get();
}

void SymbolTableWalker::tie_current_to(FuncSymbol *sym, bool override) const {
    dbprint(
        "SymbolTable: associating current scope ", current->id, " with symbol ", sym, " name \"",
        sym->name, "\"");
    if (current->assoc != nullptr) {
        if (override) {
            current->assoc = sym;
        }
    } else {
        current->assoc = sym;
    }
}

VarSymbol *SymbolTableWalker::insert(std::string& name, Box<VarSymbol> sym) const {
    dbprint("SymbolTable: inserting varsymbol with name \"", name, "\"");
    if (current->phys_symbols.contains(name)) {
        dbprint("SymbolTable: varsymbol with name ", name, " already exists");
        Symbol *existing = current->phys_symbols.find(name)->second.get();
        throw existing;
    }
    if (current == global()) {
        sym->is_global = true;
    }
    VarSymbol *ret = sym.get();
    current->phys_symbols.insert_or_assign(name, std::move(sym));

    return ret;
}

FuncSymbol *SymbolTableWalker::insert(std::string& name, Box<FuncSymbol> sym) const {
    dbprint("SymbolTable: inserting funcsymbol with name \"", name, "\"");
    if (current->phys_symbols.contains(name)) {
        dbprint("SymbolTable: symbol with name ", name, " already exists");
        // if the same symbol exists, is a function, has the same signature, and is not extern
        // replace the existing symbol with the new one
        PhysicalSymbol *existing = current->phys_symbols.find(name)->second.get();
        if (existing->get_type()->is_function()) {
            dbprint("SymbolTable: existing symbol has function type, checking for replaceability");
            FunctionType *othertype = existing->get_type()->as_function();
            FunctionType *mytype    = sym->get_type()->as_function();
            if (!othertype || !mytype) {
                dbprint("SymbolTable: could not cast othertype or mytype to FunctionType");
                goto exists;
            }
            if (othertype == mytype && !existing->is_external()) {
                dbprint("SymbolTable: existing symbol matches function signature, replacing");
                goto insert;
            } else {
                goto exists;
            }
        }
    exists:
        throw (Symbol *)existing;
    }
insert:
    if (current == global()) {
        sym->is_global = true;
    }
    FuncSymbol *ret = sym.get();
    current->phys_symbols.insert_or_assign(name, std::move(sym));

    return ret;
}

TypeSymbol *SymbolTableWalker::insert(std::string& name, Box<TypeSymbol> sym) const {
    dbprint("SymbolTable: inserting typesymbol with name \"", name, "\"");
    if (current->type_symbols.contains(name)) {
        dbprint("SymbolTable: typesymbol with name ", name, " already exists");
        Symbol *existing = current->type_symbols.find(name)->second.get();
        throw existing;
    }

    if (current == global()) {
        sym->is_global = true;
    }
    TypeSymbol *ret = sym.get();
    current->type_symbols.insert_or_assign(name, std::move(sym));

    return ret;
}

LabelSymbol *SymbolTableWalker::insert(std::string& name, Box<LabelSymbol> sym) const {
    dbprint("SymbolTable: inserting labelsymbol with name \"", name, "\"");
    if (current->label_symbols.contains(name)) {
        dbprint("SymbolTable: labelsymbol with name ", name, " already exists");
        Symbol *existing = current->label_symbols.find(name)->second.get();
        throw existing;
    }

    if (current == global()) {
        sym->is_global = true;
    }
    LabelSymbol *ret = sym.get();
    current->label_symbols.insert_or_assign(name, std::move(sym));

    return ret;
}