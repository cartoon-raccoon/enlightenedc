#include "semantics/symbols.hpp"

#include <algorithm>
#include <sstream>
#include <utility>
#include "builtins.hpp"
#include "semantics/symdata.hpp"
#include "semantics/types.hpp"

using namespace ecc::sema::sym;
using namespace ecc::sema::types;

Symbol::Symbol(Kind kind, StringRef name, Scope *scope)
    : kind(kind), name(name), scope(scope), global(scope->is_global()) {}

Symbol::Symbol(Kind kind, Location loc, StringRef name, Scope *scope)
    : kind(kind), name(name), loc(loc), scope(scope), global(scope->is_global()) {}

std::string VarSymbol::mangle() const {
    std::stringstream ss;
    if (global) {
        ss << name;
    } else if (is_implicit() && name == EC_IMPLICIT_ARGC) {
        ss << ECC_MANGLED_ARGC;
    } else if (is_implicit() && name == EC_IMPLICIT_ARGV) {
        ss << ECC_MANGLED_ARGV;
    } else {
        ss << name << "_" << "s" << scope->get_id();
    }
    return ss.str();
}

std::string FuncSymbol::mangle() const {
    std::stringstream ss;
    if (global) {
        ss << name;
    } else {
        ss << name << "_" << "s" << scope->get_id();
    }
    return ss.str();
}

std::string TypeSymbol::mangle() const {
    std::stringstream ss;
    if (global) {
        ss << name;
    } else {
        ss << name << "_" << "s" << scope->get_id();
    }
    return ss.str();
}

std::string LabelSymbol::mangle() const {
    std::stringstream ss;
    if (global) {
        ss << "global_" << name;
    } else {
        if (scope->has_assoc() && scope->is_func_assocd()) {
            ss << scope->get_func_assoc()->mangle() << "_" << name;
        } else {
            ss << name << "_" << scope->get_id();
        }
    }
    return ss.str();
}

void FuncSymbol::add_parameter(VarSymbol *param) {
    ECC_ASSERT(parameters.empty() || !parameters.back()->has_value(),
        "attempted to insert non-default parameter after defaults");

    parameters.push_back(param);
}

void FuncSymbol::add_default_param(VarSymbol *param, const eval::Value& val) {
    if (param->has_value()) {
        ECC_ASSERT(param->get_value() == val, "passed value does not match param's value");

        parameters.push_back(param);
    } else {
        param->set_value(val);
        parameters.push_back(param);
    }
}

Box<VarSymbol> FuncSymbol::as_funcptr(TypeContext& tctxt, bool is_const) {
    Type *ptrtype = tctxt.get_pointer(get_signature());

    if (is_const) {
        ptrtype = tctxt.get_const(ptrtype);
    }

    return std::make_unique<VarSymbol>(loc, name, scope, ptrtype);
}

size_t FuncSymbol::num_default_params() const {
    return std::count_if(
        parameters.begin(), parameters.end(), [](VarSymbol *sym) { return sym->has_value(); });
}

FuncSymParams FuncSymbol::params() {
#ifndef NDEBUG
    ECC_ASSERT(params_well_ordered(), "parameters in FuncSymbol not partitioned correctly");
#endif
    return FuncSymParams(parameters.data(), parameters.data() + parameters.size());
}

FuncSymParams FuncSymbol::params() const {
#ifndef NDEBUG
    ECC_ASSERT(params_well_ordered(), "parameters in FuncSymbol not partitioned correctly");
#endif
    return FuncSymParams(parameters.data(), parameters.data() + parameters.size());
}

FuncSymParams FuncSymbol::default_params() {
#ifndef NDEBUG
    ECC_ASSERT(params_well_ordered(), "parameters in FuncSymbol not partitioned correctly");
#endif
    return FuncSymParams(
        parameters.data() + num_non_default_params(), parameters.data() + parameters.size());
}

FuncSymParams FuncSymbol::default_params_after(size_t idx) {
#ifndef NDEBUG
    ECC_ASSERT(params_well_ordered(), "parameters in FuncSymbol not partitioned correctly");
#endif
    if (idx < num_non_default_params()) {
        return default_params();
    }

    return FuncSymParams(parameters.data() + idx, parameters.data() + parameters.size());
}

bool FuncSymbol::params_well_ordered() const {
    return std::ranges::is_partitioned(
        parameters, [](const VarSymbol *p) { return !p->has_value(); });
}

void Scope::set_assoc(FuncSymbol *sym, bool override) {
    dbprint("Scope: ", id, " associating with symbol name \"", sym->get_name(), "\"");
    if (assoc) {
        if (override) {
            assoc = sym;
        }
    } else {
        assoc = sym;
    }
}

void Scope::set_assoc(types::RecordType *type, bool override) {
    dbprint("Scope: ", id, " associating with type \"", type->formal(), "\"");
    if (assoc) {
        if (override) {
            assoc = type;
        }
    } else {
        assoc = type;
    }
}

FuncSymbol *Scope::get_func_assoc() const {
    if (assoc && (*assoc).is_func_assocd()) {
        return (*assoc).as_func_assoc();
    } else {
        return nullptr;
    }
}

RecordType *Scope::get_type_assoc() const {
    if (assoc && (*assoc).is_type_assocd()) {
        return (*assoc).as_type_assoc();
    } else {
        return nullptr;
    }
}

bool Scope::locally_contains(StringRef sym) const {
    return phys_symbols.contains(sym) || implicits.contains(sym);
}

PhysicalSymbol *Scope::get(StringRef sym) const {
    // get from explicit physical symbols first
    if (phys_symbols.contains(sym)) {
        return phys_symbols.find(sym)->second.get();
    }

    // failing which, get the implicit one
    if (implicits.contains(sym)) {
        return implicits.find(sym)->second.get();
    }

    return nullptr;
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
    ECC_ASSERT(!current->nested.empty(), "tried to enter nonexistent nested scope");
    ECC_ASSERT(next_scope_idx < current->nested.size(),
        "no more nested scopes left to enter in current scope");

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
            ECC_UNREACHABLE("tried to exit global scope");
        }
    }
}

void SymbolTableWalker::reset() {
    current = global();
}

Symbol *SymbolTableWalker::lookup(StringRef sym, bool current_only) const {
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

VarSymbol *SymbolTableWalker::lookup_var(StringRef sym, bool current_only) const {
    Scope *my_current = current;
    if (current_only) {
        dbprint("SymbolTable: looking up varsymbol ", sym, " in current scope");
        if (my_current->locally_contains(sym)) {
            // this returns null if we pull a funcsymbol
            return my_current->get(sym)->as_varsym();
        } else {
            return nullptr;
        }
    }
    dbprint("SymbolTable: looking up varsymbol ", sym);

    // look for symbol in current scope
    while (!(my_current->locally_contains(sym))) {
        // if already global, return null
        if (my_current->outer == nullptr) {
            ECC_ASSERT_N(my_current == global());
            dbprint("SymbolTable: symbol \'", sym, "\' not found");
            return nullptr;
        }

        // move to outer
        my_current = my_current->outer;
    }

    return my_current->get(sym)->as_varsym();
}

VarSymbol *SymbolTableWalker::lookup_var_from(Scope *from, StringRef sym, bool current_only) const {
    Scope *saved = current;
    current = from;

    VarSymbol *ret = lookup_var(sym, current_only);

    current = saved;
    return ret;
}

FuncSymbol *SymbolTableWalker::lookup_func(StringRef sym, bool current_only) const {
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
            ECC_ASSERT_N(my_current == global());
            dbprint("SymbolTable: symbol \'", sym, "\' not found");
            return nullptr;
        }

        // move to outer
        my_current = my_current->outer;
    }

    return my_current->phys_symbols.find(sym)->second->as_funcsym();
}

FuncSymbol *SymbolTableWalker::lookup_func_from(Scope *from, StringRef sym, bool current_only) const {
    Scope *saved = current;
    current = from;

    FuncSymbol *ret = lookup_func(sym, current_only);

    current = saved;
    return ret;
}

TypeSymbol *SymbolTableWalker::lookup_type(StringRef sym, bool current_only) const {
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
            ECC_ASSERT_N(my_current == global());
            dbprint("SymbolTable: symbol \'", sym, "\' not found");
            return nullptr;
        }

        // move to outer
        my_current = my_current->outer;
    }

    return my_current->type_symbols.find(sym)->second.get();
}

TypeSymbol *SymbolTableWalker::lookup_type_from(Scope *from, StringRef sym, bool current_only) const {
    Scope *saved = current;
    current = from;

    TypeSymbol *ret = lookup_type(sym, current_only);

    current = saved;
    return ret;
}

LabelSymbol *SymbolTableWalker::lookup_label(StringRef sym, bool current_only) const {
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

        // if already at global scope (outer is null), label does not exist
        if (my_current->outer == nullptr) {
            ECC_ASSERT_N(my_current == global());
            dbprint("SymbolTable: label \'", sym, "\' not found");
            return nullptr;
        }

        // move to outer
        my_current = my_current->outer;
    }

    return my_current->label_symbols.find(sym)->second.get();
}

LabelSymbol *SymbolTableWalker::lookup_label_from(Scope *from, StringRef sym, bool current_only) const {
    Scope *saved = current;
    current = from;

    LabelSymbol *ret = lookup_label(sym, current_only);

    current = saved;
    return ret;
}

void SymbolTableWalker::tie_current_to(FuncSymbol *sym, bool override) const {
    current->set_assoc(sym, override);
}

void SymbolTableWalker::tie_current_to(RecordType *type, bool override) const {
    current->set_assoc(type, override);
}

VarSymbol *SymbolTableWalker::insert_var(InsertVarArgs args) const {
    dbprint("SymbolTable: inserting varsymbol with name \"", args.name, "\"");
    if (current->phys_symbols.contains(args.name)) {
        dbprint("SymbolTable: varsymbol with name ", args.name, " already exists");
        Symbol *existing = current->phys_symbols.find(args.name)->second.get();
        throw existing;
    }
    // override
    auto sym = make_box<VarSymbol>(args.loc, args.name, current, args.type);
    if (args.val) {
        sym->set_value(*args.val);
    }
    sym->get_symdata()->set_linkage(args.linkage);
    sym->get_symdata()->set_duration(args.duration);
    VarSymbol *ret = sym.get();
    current->phys_symbols.insert_or_assign(args.name.str(), std::move(sym));

    return ret;
}

VarSymbol *SymbolTableWalker::insert_var_at(Scope *at, InsertVarArgs args) const {
    Scope *saved = current;
    current = at;
    VarSymbol *ret;
    try {
        ret = insert_var(std::move(args));
    } catch (Symbol *existing) {
        current = saved;
        throw existing;
    }

    current = saved;
    return ret;
}

VarSymbol *SymbolTableWalker::insert_implicit(InsertVarArgs args) const {
    dbprint("SymbolTable: inserting implicit varsymbol with name \"", args.name, "\"");
    if (current->locally_contains(args.name)) {
        Symbol *existing = current->get(args.name);
        throw existing;
    }

    ECC_ASSERT(current != global(), "tried to insert implicit at global scope");

    auto sym = make_box<VarSymbol>(args.loc, args.name, current, args.type);
    if (args.val) {
        sym->set_value(*args.val);
    }

    // ignore any non-default linkage or duration
    sym->get_symdata()->set_linkage(Linkage::NONE);
    sym->get_symdata()->set_duration(StorageDuration::AUTO);
    sym->implicit = true;
    VarSymbol *ret = sym.get();
    current->implicits.insert_or_assign(args.name.str(), std::move(sym));

    return ret;
}

AliasSymbol *SymbolTableWalker::insert_alias(InsertAliasArgs args) const {
    dbprint("SymbolTable: inserting alias for \"", args.aliasee->get_name(), "\"");

    auto sym = make_box<AliasSymbol>(args.loc, args.aliasee, current);

    AliasSymbol *ret = sym.get();

    if (current->phys_symbols.contains(args.aliasee->get_name())) {
        // if the current physical symbols table contains the name, extract it
        // and banish it to the shadow zone
        Box<PhysicalSymbol> existing
            = std::move(current->phys_symbols.find(args.aliasee->get_name())->second);

        current->phys_symbols.erase(args.aliasee->get_name());

        current->shadowed.insert(std::move(existing));
    }

    current->phys_symbols.insert_or_assign(args.aliasee->get_name().str(), std::move(sym));

    return ret;
}

FuncSymbol *SymbolTableWalker::insert_func(InsertFuncArgs args) const {
    dbprint("SymbolTable: inserting funcsymbol with name \"", args.name, "\"");
    if (current->phys_symbols.contains(args.name)) {
        dbprint("SymbolTable: symbol with name ", args.name, " already exists");

        PhysicalSymbol *existing = current->phys_symbols.find(args.name)->second.get();

        // If the existing symbol is a function, attempt decl-def reconciliation
        if (existing->is_func()) {
            dbprint("SymbolTable: existing symbol has function type, checking for replaceability");
            FunctionType *othertype = existing->as_funcsym()->get_signature();
            FunctionType *mytype    = args.signature;
            FuncSymData *existdata = existing->as_funcsym()->get_symdata();
            if (!othertype || !mytype) {
                dbprint("SymbolTable: could not cast othertype or mytype to FunctionType");
                goto exists;
            }
            if (othertype == mytype) {
                dbprint(
                    "SymbolTable: existing symbol matches function signature, evaluating "
                    "reconciliation");
                FuncSymbol *existfunc = existing->as_funcsym();
                ECC_ASSERT_N(existfunc);
                if (!existfunc->has_body() && args.has_body) {
                    // existing is decl, new sym is def, check linkage and language linkage
                    if (!existdata->compatible_from(args.signature, args.linkage, args.langlink)) {
                        goto exists;
                    }

                    existfunc->set_body();
                    existfunc->parameters = std::move(args.parameters);
                    for (auto *param : existfunc->parameters) {
                        param->set_funcparam(true);
                    }

                    return existfunc;
                } else if (existfunc->has_body() && args.has_body) {
                    // existing is def, new sym is def

                    goto exists;
                } else {
                    // existing is decl or def, new sym is decl, check compatibility
                    if (!existdata->compatible_from(args.signature, args.linkage, args.langlink)) {
                        goto exists;
                    }

                    return existfunc;
                }
            } else {
                goto exists;
            }
        }

    exists:
        throw (Symbol *)existing;
    }
    auto sym = make_box<FuncSymbol>(args.loc, args.name, current, args.signature, std::move(args.parameters));
    sym->has_body_ = args.has_body;
    sym->get_symdata()->set_linkage(args.linkage);
    sym->get_symdata()->set_lang_linkage(args.langlink);
    FuncSymbol *ret = sym.get();
    current->phys_symbols.insert_or_assign(args.name.str(), std::move(sym));

    return ret;
}

FuncSymbol *SymbolTableWalker::insert_func_at(Scope *at, InsertFuncArgs args) const {
    Scope *saved = current;
    current = at;
    FuncSymbol *ret;
    try {
        ret = insert_func(std::move(args));
    } catch (Symbol *existing) {
        current = saved;
        throw existing;
    }

    current = saved;
    return ret;
}

TypeSymbol *SymbolTableWalker::insert_type(InsertTypeArgs args) const {
    dbprint("SymbolTable: inserting typesymbol with name \"", args.name, "\"");
    if (current->type_symbols.contains(args.name)) {
        dbprint("SymbolTable: typesymbol with name ", args.name, " already exists");
        Symbol *existing = current->type_symbols.find(args.name)->second.get();
        throw existing;
    }

    auto sym = make_box<TypeSymbol>(args.loc, args.name, current, args.type);
    TypeSymbol *ret = sym.get();
    current->type_symbols.insert_or_assign(args.name.str(), std::move(sym));

    return ret;
}

TypeSymbol *SymbolTableWalker::insert_type_at(Scope *at, InsertTypeArgs args) const {
    Scope *saved = current;
    current = at;
    TypeSymbol *ret;
    try {
        ret = insert_type(args);
    } catch (Symbol *existing) {
        current = saved;
        throw existing;
    }

    current = saved;
    return ret;
}

LabelSymbol *SymbolTableWalker::insert_label(InsertLabelArgs args) const {
    dbprint("SymbolTable: inserting labelsymbol with name \"", args.name, "\"");
    if (current->label_symbols.contains(args.name)) {
        dbprint("SymbolTable: labelsymbol with name ", args.name, " already exists");
        Symbol *existing = current->label_symbols.find(args.name)->second.get();
        throw existing;
    }
    auto sym = make_box<LabelSymbol>(args.loc, args.name, current);
    LabelSymbol *ret = sym.get();
    current->label_symbols.insert_or_assign(args.name.str(), std::move(sym));

    return ret;
}

LabelSymbol *SymbolTableWalker::insert_label_at(Scope *at, InsertLabelArgs args) const {
    Scope *saved = current;
    current = at;
    LabelSymbol *ret;
    try {
        ret = insert_label(args);
    } catch (Symbol *existing) {
        current = saved;
        throw existing;
    }

    current = saved;
    return ret;
}