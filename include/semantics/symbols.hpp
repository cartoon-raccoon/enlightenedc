#pragma once

#ifndef ECC_SYMBOLS_H
#define ECC_SYMBOLS_H

#include <cstdint>
#include <memory>
#include <stack>

#include "eval/value.hpp"
#include "location.hpp"
#include "semantics/symdata.hpp"
#include "semantics/types.hpp"
#include "ds/stringmap.hpp"
#include "prelude.hpp"

namespace ecc::sema::sym {
/*
Symbols and Symbol Table of Ecc.

A symbol represents something that was declared within an EnlightenedC program.
This can be a type, a variable, a function, etc.

Symbols are stored within a SymbolTable. This object maps symbol names to their
corresponding Symbol objects, within Scope objects.
*/

using namespace ecc;
using namespace util;
using namespace location;

class PhysicalSymbol;
class AbstractSymbol;
class VarSymbol;
class FuncSymbol;
class AliasSymbol;
class TypeSymbol;
class LabelSymbol;
class Scope;
class SymbolTableWalker;

/**
The abstract symbol class.
*/
class Symbol : public NoCopy {
public:
    // The kind of symbol.
    enum class Kind : uint8_t {
        VAR,   // This symbol references a variable.
        FUNC,  // This symbol references a function definition.
        ALIAS, // This symbol is an alias to a physical symbol.
        TYPE,  // This symbol references a declared type.
        LABEL, // This symbol references a label.
    };

    Symbol(Kind kind, StringRef name, Scope *scope);

    Symbol(Kind kind, Location loc, StringRef name, Scope *scope);

    virtual ~Symbol() = default;

    const Kind kind;

    StringRef get_name() { return name; }

    Location get_loc() { return loc; }

    Scope *get_scope() { return scope; }

    void set_scope(Scope *sc) { scope = sc; }

    bool is_global() const { return global; }

    virtual bool is_physical() { return false; };
    virtual bool is_abstract() { return false; };

    bool is_var() const { return kind == Kind::VAR; }
    bool is_func() const { return kind == Kind::FUNC; }
    bool is_alias() const { return kind == Kind::ALIAS; }
    bool is_type() const { return kind == Kind::TYPE; }
    bool is_label() const { return kind == Kind::LABEL; }

    virtual std::string to_string() const = 0;

    virtual std::string mangle() const = 0;

    virtual PhysicalSymbol *as_physical() { return nullptr; }
    virtual AbstractSymbol *as_abstract() { return nullptr; }

    virtual VarSymbol *as_varsym() { return nullptr; }
    virtual FuncSymbol *as_funcsym() { return nullptr; }
    virtual AliasSymbol *as_alias() { return nullptr; }
    virtual TypeSymbol *as_typesym() { return nullptr; }
    virtual LabelSymbol *as_labsym() { return nullptr; }
protected:

    std::string name;

    // The location of the symbol.
    Location loc;

    Scope *scope;

        /// If the symbol is global.
    bool global = false;
};

/**
A symbol that names a runtime entity, usually a variable or a function.

In most cases, a PhysicalSymbol gets translated to a real address. However,
the one exception is constexpr symbols, which fold to literals at the LIR level,
and thus never get storage or linkage.
*/
class PhysicalSymbol : public Symbol {
protected:
    Rc<SymData> symdata = nullptr;

    bool implicit = false;

public:
    friend class SymbolTableWalker;

    PhysicalSymbol(Kind kind, StringRef name, Scope *scope) : Symbol(kind, name, scope) {}

    PhysicalSymbol(Kind kind, Location loc, StringRef name, Scope *scope)
        : Symbol(kind, loc, name, scope) {}

    bool is_external() const { 
        return symdata->get_linkage() != Linkage::INTERNAL && symdata->get_linkage() != Linkage::NONE;
    }

    SymData *get_symdata() { return symdata.get(); }

    /**
    The shared owner of this symbol's data. Pass this (not get_symdata()) when
    another object needs to co-own the data, so it shares this control block
    rather than creating a second one over the same object.
    */
    const Rc<SymData>& get_symdata_rc() const { return symdata; }

    Linkage get_linkage() const { return symdata->get_linkage(); }

    bool is_implicit() const { return implicit; }

    virtual types::Type *get_type() const = 0;

    bool is_physical() override { return true; }

    PhysicalSymbol *as_physical() override { return this; }

    static bool classof(const Symbol *sym) {
        switch (sym->kind) {
        case Kind::VAR:
        case Kind::FUNC:
            return true;
        default:
            return false;
        }
    }
};

// A symbol that is abstract, and exists only for the purposes of the compiler.
// (e.g. a label or type declaration).
class AbstractSymbol : public Symbol {
public:
    friend class SymbolTableWalker;

    AbstractSymbol(Kind kind, StringRef name, Scope *scope) : Symbol(kind, name, scope) {}

    AbstractSymbol(Kind kind, Location loc, StringRef name, Scope *scope)
        : Symbol(kind, loc, name, scope) {}

    AbstractSymbol *as_abstract() override { return this; }

    bool is_abstract() override { return true; }

    static bool classof(const Symbol *sym) {
        switch (sym->kind) {
        case Kind::TYPE:
        case Kind::LABEL:
            return true;
        default:
            return false;
        }
    }
};

/**
A symbol representing a variable declaration.
*/
class VarSymbol : public PhysicalSymbol {
public:
    VarSymbol(Location loc, StringRef name, Scope *scope, types::Type *type)
        : PhysicalSymbol(Symbol::Kind::VAR, loc, name, scope) {

        symdata = make_rc<VarSymData>(name, type);
    }

    VarSymbol(
        Location loc, StringRef name, Scope *scope, types::Type *type, eval::Value value)
        : PhysicalSymbol(Symbol::Kind::VAR, loc, name, scope), value(value) {

        symdata = make_rc<VarSymData>(name, type);
    }

    std::string to_string() const override;

    std::string mangle() const override;

    VarSymData *get_symdata() const { return dyncast<VarSymData>(symdata.get()); }

    /**
    Set the value of the VarSymbol.
    */
    void set_value(const eval::Value& val) { value = val; }

    /**
    Check if the VarSymbol has a value.
    */
    bool has_value() const { return value.has_value(); }

    bool is_const_foldable() const { return value.has_value() && !funcparam; }

    void set_funcparam(bool funcparam) { this->funcparam = funcparam; }

    bool is_funcparam() const { return funcparam; }

    Optional<eval::Value> get_value() const { return value; }

    VarSymbol *as_varsym() override { return this; }

    types::Type *get_type() const override { return get_symdata()->get_type(); }

    void set_type(types::Type *type) const { get_symdata()->set_type(type); }

    static bool classof(const Symbol *sym) { return sym->kind == Kind::VAR; }

private:
    // The value of the Symbol, if defined.
    Optional<eval::Value> value;

    bool funcparam = false;
};

/**
An iterator over the arguments to a FuncSymbol.
*/
class FuncSymParams {
    VarSymbol *const *first;
    VarSymbol *const *last;

    friend class FuncSymbol;
    FuncSymParams(VarSymbol *const *first, VarSymbol *const *last)
        : first(first), last(last) {}

public:
    class iterator {
        VarSymbol *const *slot;

    public:
        explicit iterator(VarSymbol *const *slot) : slot(slot) {}

        VarSymbol& operator*() const { return **slot; }

        VarSymbol *operator->() const { return *slot; }

        iterator& operator++() { ++slot; return *this; }

        bool operator==(const iterator& o) const { return slot == o.slot; }
    };

    iterator begin() const { return iterator(first); }

    iterator end() const { return iterator(last); }

    size_t size() const { return static_cast<size_t>(last - first); }

    bool empty() const { return first == last; }
};


/**
A symbol representing a function declaration
(function pointers and externally linked functions are handled by VarSymbol).
*/
class FuncSymbol : public PhysicalSymbol {
public:
    friend class SymbolTableWalker;

    FuncSymbol(
        Location loc, StringRef name, Scope *scope, types::FunctionType *signature,
        Vec<VarSymbol *> parameters)
        : PhysicalSymbol(Symbol::Kind::FUNC, loc, name, scope), parameters(std::move(parameters)) {
        symdata = make_rc<FuncSymData>(name, signature);
    }

    FuncSymbol(Location loc, StringRef name, Scope *scope, types::FunctionType *signature)
        : PhysicalSymbol(Symbol::Kind::FUNC, loc, name, scope) {
        symdata = make_rc<FuncSymData>(name, signature);
    }

    static Box<FuncSymbol>
    empty(Location loc, StringRef name, Scope *scope, types::FunctionType *signature) {

        auto ret = std::make_unique<FuncSymbol>(loc, name, scope, signature);

        ret->has_body_ = false;

        return ret;
    }

    bool has_body() const { return has_body_; }

    void set_body() { has_body_ = true; }

    void add_parameter(VarSymbol *param);

    void add_default_param(VarSymbol *param, const eval::Value& val);

    std::string to_string() const override;

    std::string mangle() const override;

    FuncSymData *get_symdata() const { return dyncast<FuncSymData>(symdata.get()); }

    LangLinkage get_lang_linkage() const { return get_symdata()->get_lang_linkage(); }

    types::FunctionType *get_signature() const { return get_symdata()->get_signature(); }

    size_t num_params() const { return parameters.size(); }

    size_t num_default_params() const;

    size_t num_non_default_params() const { return num_params() - num_default_params(); }

    /**
    Get an iterator over the parameters of the FuncSymbol.
    */
    FuncSymParams params();

    /**
    Get an iterator over the parameters of the FuncSymbol.
    */
    FuncSymParams params() const;

    /**
    Get an iterator over the default parameters of the FuncSymbol.
    */
    FuncSymParams default_params();

    /**
    Get an iterator over the default parameters of the FuncSymbol, after `idx`.

    If `idx` corresponds to a non-default parameters, an iterator starting at the
    first default parameter is returned.
    */
    FuncSymParams default_params_after(size_t idx);

    /// Create a function pointer VarSymbol from this FuncSymbol.
    Box<VarSymbol> as_funcptr(types::TypeContext& tctxt, bool is_const = false);

    FuncSymbol *as_funcsym() override { return this; }

    types::Type *get_type() const override { return get_symdata()->get_signature(); }

    static bool classof(const Symbol *sym) { return sym->kind == Kind::FUNC; }
private:
    /**
    Check that the parameter invariant is upheld.
    */
    bool params_well_ordered() const;

    bool has_body_ = true;

    /**
    The list of parameters to the function.

    INVARIANT: all non-default parameters must be at the front, all defaults at the back.
    */
    Vec<VarSymbol *> parameters;
};

/**
A symbol that aliases another PhysicalSymbol.

This is used with the `global <var>` construct.
*/
class AliasSymbol : public PhysicalSymbol {
public:
    friend class SymbolTableWalker;
    
    AliasSymbol(Location loc, PhysicalSymbol *aliasee, Scope *scope)
        : PhysicalSymbol(Kind::ALIAS, loc, aliasee->get_name(), scope), aliasee(aliasee) {
        symdata = aliasee->get_symdata_rc();
    }

    PhysicalSymbol *get_aliasee() { return aliasee; }

    VarSymbol *as_var() { return aliasee->as_varsym(); }

    FuncSymbol *as_func() { return aliasee->as_funcsym(); }
    
    bool aliases_var() const { return aliasee->is_var(); }

    bool aliases_func() const { return aliasee->is_func(); }

    types::Type *get_type() const override { return aliasee->get_type(); }

    std::string to_string() const override { return aliasee->to_string(); }

    std::string mangle() const override { return "ALIAS" + aliasee->mangle(); }

    AliasSymbol *as_alias() override { return this; }

    static bool classof(const Symbol *sym) { return sym->kind == Kind::ALIAS; }

private:
    PhysicalSymbol *aliasee;
};

/**
A symbol representing a type declaration (class, union, enum).
*/
class TypeSymbol : public AbstractSymbol {
public:
    TypeSymbol(Location loc, StringRef name, Scope *scope, types::BaseType *type)
        : AbstractSymbol(Symbol::Kind::TYPE, loc, name, scope), type(type) {}

    types::BaseType *type;

    std::string to_string() const override;

    std::string mangle() const override;

    TypeSymbol *as_typesym() override { return this; }

    static bool classof(const Symbol *sym) { return sym->kind == Kind::TYPE; }
};

/*
A symbol representing a label (for use by goto).
*/
class LabelSymbol : public AbstractSymbol {
public:
    LabelSymbol(Location loc, StringRef name, Scope *scope)
        : AbstractSymbol(Symbol::Kind::LABEL, loc, name, scope) {}

    std::string to_string() const override;

    std::string mangle() const override;

    LabelSymbol *as_labsym() override { return this; }

    static bool classof(const Symbol *sym) { return sym->kind == Kind::LABEL; }
};

/**
A class representing a lexical scoping level.

Scopes are the main container for Symbols. They represent one level of the nested SymbolTable,
and act as an associative container, mapping names to Symbols.

## Namespaces

Scopes maintain three namespaces: physical symbols, types, and labels. Each namespace is non-overlapping,
so a type and label can share a name within the same scope. A separate container for implicit variables
is also maintained, but it is semantically part of the physical symbol namespace, and any names within it
are shadowed by symbols in the main physical symbol container.

## Association

Scopes can be associated with a particular FuncSymbol or Type. When a function is defined, it implicitly
declares a new scope, and so it can be associated with that Scope object. Similarly, when a RecordType
(a class or union) is defined, it implicitly declares a new scope, and so can be associated with that
Scope object as well.
*/
class Scope {
public:
    class ScopeAssoc {
        enum Type : uint8_t { NONE, TYPE, FUNC, } type;

        union {
            FuncSymbol *func_assoc;
            types::RecordType *type_assoc;
        };
    public:
        ScopeAssoc() : type(NONE), func_assoc(nullptr) {}

        ScopeAssoc(FuncSymbol *func) : type(FUNC), func_assoc(func) {}

        ScopeAssoc(types::RecordType *type) : type(TYPE), type_assoc(type) {}

        ScopeAssoc& operator=(FuncSymbol *func) { 
            type = FUNC; func_assoc = func; return *this;
        }

        ScopeAssoc& operator=(types::RecordType *type) {
            this->type = TYPE; type_assoc = type; return *this;
        }

        bool is_func_assocd() const { return type == FUNC; }

        bool is_type_assocd() const { return type == TYPE; }

        FuncSymbol *as_func_assoc() const {
            if (is_type_assocd()) {
                return nullptr;
            } else {
                return func_assoc;
            }
        }

        types::RecordType *as_type_assoc() const {
            if (is_func_assocd()) {
                return nullptr;
            } else {
                return type_assoc;
            }
        }
    };
    
    Scope(FuncSymbol *assoc, Scope *outer, uint64_t id, int idx_in_nested)
        : outer(outer), id(id), idx_in_nested(idx_in_nested) {
        if (assoc) { this->assoc = assoc; }
    }

    Scope(FuncSymbol *assoc, Scope *outer, uint64_t id) : outer(outer), id(id) {
        if (assoc) { this->assoc = assoc; }
    }

    bool is_global() const { return idx_in_nested < 0; }

    bool has_assoc() const { return assoc.has_value(); }

    void set_assoc(FuncSymbol *sym, bool override = false);

    void set_assoc(types::RecordType *type, bool override = false);

    Scope *get_outer() { return outer; }

    const Scope *get_outer() const { return outer; }

    bool is_func_assocd() const { return assoc ? (*assoc).is_func_assocd() : false; }

    bool is_type_assocd() const { return assoc ? (*assoc).is_type_assocd() : false; }

    FuncSymbol *get_func_assoc() const;

    types::RecordType *get_type_assoc() const;

    bool locally_contains(StringRef sym) const;

    PhysicalSymbol *get(StringRef sym) const;

    uint64_t get_id() const { return id; }

    void print(std::stringstream& ss, int depth);

private:
    friend class SymbolTable;
    friend class SymbolTableWalker;

    // the outer scope enclosing the inner scope.
    Scope *outer;

    // A function associated with this scope.
    // if null, this is an anonymous scope.
    Optional<ScopeAssoc> assoc;

    uint64_t id;

    // the symbol tables. Keys are owned; StringRefHash/StringRefEq are transparent, so a
    // StringRef or string_view can be used as a lookup key without allocating.
    ds::StringMap<Box<PhysicalSymbol>> phys_symbols;
    ds::StringMap<Box<VarSymbol>> implicits;
    
    ds::StringMap<Box<TypeSymbol>> type_symbols;
    ds::StringMap<Box<LabelSymbol>> label_symbols;

    /**
    Bag for any shadowed symbols. No need to be a map, since they can no longer be looked up,
    but we still need to hold on to them because they are still referenced by code before
    the shadowing symbol was introduced.
    */
    HashSet<Box<PhysicalSymbol>> shadowed;

    // inner scopes contained within this scope.
    Vec<Box<Scope>> nested;

    // The index of the next nested scope to enter.
    int idx_in_nested = -1;
};

/*
The symbol table, storing all symbols in a given translation unit.
*/
class SymbolTable {
public:
    SymbolTable() : global(make_box<Scope>(nullptr, nullptr, 0)) {}

    // The global scope.
    Box<Scope> global;

    // Clear the entire SymbolTable.
    void clear();

    std::string to_string() const;
};

/**
Arguments for inserting a new VarSymbol.
*/
struct InsertVarArgs {
    Location loc;
    StringRef name;
    types::Type *type;
    Optional<eval::Value> val;
    Linkage linkage = Linkage::NONE;

    InsertVarArgs(Location loc, StringRef name, types::Type *type)
        : loc(loc), name(name), type(type) {}

    InsertVarArgs(Location loc, StringRef name, types::Type *type, const eval::Value& val)
        : loc(loc), name(name), type(type), val(val) {}

    InsertVarArgs(Location loc, StringRef name, types::Type *type, Linkage linkage)
        : loc(loc), name(name), type(type), linkage(linkage) {}

    InsertVarArgs(Location loc, StringRef name, types::Type *type, const eval::Value& val, Linkage linkage)
        : loc(loc), name(name), type(type), val(val), linkage(linkage) {}
};

/**
Arguments for inserting a new FuncSymbol.
*/
struct InsertFuncArgs {
    Location loc;
    StringRef name;
    types::FunctionType *signature;
    bool has_body = false;
    Vec<VarSymbol *> parameters;
    Linkage linkage = Linkage::NONE;
    LangLinkage langlink = LangLinkage::NONE;

    InsertFuncArgs(Location loc, StringRef name, types::FunctionType *signature)
        : loc(loc), name(name), signature(signature) {}

    InsertFuncArgs(Location loc, StringRef name, types::FunctionType *signature,
                   Linkage linkage)
        : loc(loc), name(name), signature(signature), linkage(linkage) {}

    InsertFuncArgs(Location loc, StringRef name, types::FunctionType *signature,
                   LangLinkage langlink)
        : loc(loc), name(name), signature(signature), linkage(Linkage::EXTERNAL), langlink(langlink) {}
};

struct InsertAliasArgs {
    Location loc;
    PhysicalSymbol *aliasee;

    InsertAliasArgs(Location loc, PhysicalSymbol *aliasee) : loc(loc), aliasee(aliasee) {}
};

struct InsertTypeArgs {
    Location loc;
    StringRef name;
    types::BaseType *type;

    InsertTypeArgs(Location loc, StringRef name, types::BaseType *type)
        : loc(loc), name(name), type(type) {}
};

struct InsertLabelArgs {
    Location loc;
    StringRef name;

    InsertLabelArgs(Location loc, StringRef name)
        : loc(loc), name(name) {}
};

/**
A Walker for the Symbol Table.

The SymbolTable itself is just POD. It holds a global scope, which itself holds pointers to
all nested scopes. The SymbolTableWalker is the stateful object that can walk, query, and modify
the symbol table.
*/
class SymbolTableWalker {
    Ref<SymbolTable> st;

public:
    SymbolTableWalker(SymbolTable& st) : st(st), current(st.global.get()) {}

    SymbolTableWalker(const SymbolTableWalker& stw)
        : st(stw.st), current(stw.current), next_id(stw.next_id),
          next_scope_idx(stw.next_scope_idx), prev_scope_idxs(stw.prev_scope_idxs) {}

    SymbolTableWalker(const SymbolTable&& stw) = delete; // fixme: implement

    mutable Scope *current;

    Scope *global() const { return st.get().global.get(); }

    // Create and enter a new scope.
    void push_scope(FuncSymbol *assoc = nullptr);

    /*
    Enter the currently indexed scope in current scope.

    Throw error if no scopes exist, or there are no more scopes left to enter.
    */
    void enter_scope();

    // Exit the current scope to the outer one.
    void pop_scope();

    // Reset the current scope to the global scope and first index.
    void reset();

    /**
    Lookup a symbol by name. Returns null of no symbol exists.

    Looks up the symbol in the following order: Var, Func, Type, Label. If a variable and
    label of the same name exist, the variable will be returned, and `lookup` will never
    work as intended if the label is what is needed. For a more reliable lookup, use
    the specific namespace lookup functions.
    */
    Symbol *lookup(StringRef sym, bool current = false) const;

    /**
    Lookup a VarSymbol by name; `current=true` searches only in the current scope.
    Returns `null` if no VarSymbol with that name is found.

    The search order is as follows: searches the main varsymbol namespace, then
    the implicit varsymbol namespace, then recurses to outer scopes.
    */
    VarSymbol *lookup_var(StringRef sym, bool current = false) const;

    /**
    Lookup a VarSymbol by name from a specific scope.
    */
    VarSymbol *lookup_var_from(Scope *from, StringRef sym, bool current = false) const;

    FuncSymbol *lookup_func(StringRef sym, bool current = false) const;

    FuncSymbol *lookup_func_from(Scope *from, StringRef sym, bool current = false) const;

    TypeSymbol *lookup_type(StringRef sym, bool current = false) const;

    TypeSymbol *lookup_type_from(Scope *from, StringRef sym, bool current = false) const;

    /**
    Look up a label from Scope `from`, up to the first function scope.

    Unlike other lookup functions, which continue on to global scope,
    `lookup_label` only recurses outwards until a function boundary.
    This is because labels are scoped to function scope specifically.
    */
    LabelSymbol *lookup_label(StringRef sym, bool current = false) const;

    LabelSymbol *lookup_label_from(Scope *from, StringRef sym, bool current = false) const;

    /**
    Associate the current scope with the given FuncSymbol `sym`.

    If current scope is already tied to something, replaces it with the new one depending on the
    value of `override`.
    */
    void tie_current_to(FuncSymbol *sym, bool override = false) const;

    /**
    Associate the current scope with the given RecordType `type`.

    If current scope is already tied to something, replaces it with the new one depending on the
    value of `override`.
    */
    void tie_current_to(types::RecordType *type, bool override = false) const;

    /** 
    Add a new (explicit) VarSymbol to the current scope.

    Returns a pointer to the inserted symbol for further use. If a non-implicit symbol with the same name
    already exists in the current scope, a `Symbol *` pointer to the existing PhysicalSymbol is thrown.
    */
    VarSymbol *insert_var(InsertVarArgs args) const;

    /** 
    Add a new VarSymbol at the specified scope.
    
    Returns a pointer to the inserted symbol for further use. If a symbol with the same name
    already exists in the current scope, a `Symbol *` pointer to the existing PhysicalSymbol is thrown.
    */
    VarSymbol *insert_var_at(Scope *at, InsertVarArgs args) const;

    /** 
    Add a new implicit VarSymbol to the current scope.
    
    Returns a pointer to the inserted symbol for further use. If a symbol with the same name
    already exists in the current scope, a `Symbol *` pointer to the existing VarSymbol is thrown.
    */
    VarSymbol *insert_implicit(InsertVarArgs args) const;

    /**
    Add a new alias to an existing PhysicalSymbol to the current scope.

    Returns a pointer to the inserted alias (not the aliasee). Insertion always succeeds, because
    it shadows any symbol in the current scope with the same name.
    */
    AliasSymbol *insert_alias(InsertAliasArgs args) const;

    /** 
    Add a new FuncSymbol at the current scope.
    
    Returns a pointer to the inserted symbol for further use. If a symbol with the same name
    already exists in the current scope, a `Symbol *` pointer to the existing VarSymbol is thrown.
    */
    FuncSymbol *insert_func(InsertFuncArgs args) const;

    /** 
    Add a new FuncSymbol at the specified scope.
    
    Returns a pointer to the inserted symbol for further use. If a symbol with the same name
    already exists in the current scope, a `Symbol *` pointer to the existing VarSymbol is thrown.
    */
    FuncSymbol *insert_func_at(Scope *at, InsertFuncArgs args) const;

    /** 
    Add a new TypeSymbol at the current scope.
    
    Returns a pointer to the inserted symbol for further use. If a symbol with the same name
    already exists in the current scope, a `Symbol *` pointer to the existing VarSymbol is thrown.
    */
    TypeSymbol *insert_type(InsertTypeArgs args) const;

    TypeSymbol *insert_type_at(Scope *at, InsertTypeArgs args) const;

    LabelSymbol *insert_label(InsertLabelArgs args) const;

    LabelSymbol *insert_label_at(Scope *at, InsertLabelArgs args) const;

private:
    // The ID to assign to the next scope.
    MonotonicCtr<uint64_t> next_id = 1;

    size_t next_scope_idx = 0;

    std::stack<size_t> prev_scope_idxs;
};

} // namespace ecc::sema::sym

#endif