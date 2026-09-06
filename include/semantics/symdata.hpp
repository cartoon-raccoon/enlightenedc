#pragma once

#ifndef ECC_SYMDATA_H
#define ECC_SYMDATA_H

#include <cstdint>
#include <string>

#include "semantics/types.hpp"
#include "util.hpp"

namespace ecc::sema::sym {

// The linkage of the symbol.
enum class Linkage : uint8_t {
    // The symbol is defined within this translation unit.
    INTERNAL,
    // The symbol is defined from another EnlightenedC object file.
    EXTERNAL,
    // The symbol is defined from another C object file, and so must follow cdecl.
    EXTERNC,
};

enum class Visibility : uint8_t {
    /**
    The symbol has static visibility, the semantics of which depend on scope
    and symbol type.

    If the symbol is a non-global variable, it is given static storage duration.
    If the symbol is a global function or variable, it is not visible inside the generated
    object's symbol table.
    */
    STATIC,
    // The symbol is visible outside this translation unit.
    PUBLIC,
    // The symbol is visible outside this translation unit, with C linkage.
    EXTERNC,
};

class VarSymData;
class FuncSymData;

/**
Metadata for physical symbols.
*/
class SymData {

    std::string name;

    std::string mangled_name;

    /**
    The linkage of the symbol.
    */
    Linkage linkage = Linkage::INTERNAL;

    /**
    The visibility of the symbol.
    */
    Visibility visibility = Visibility::PUBLIC;

    Optional<std::string> link_name;

public:
    enum class Kind : uint8_t {
        VAR,
        FUNC,
    };
    const Kind kind;

    SymData(Kind kind, std::string name) : name(std::move(name)), kind(kind) {}

    SymData(Kind kind, std::string name, Linkage linkage, Visibility visibility)
        : name(std::move(name)), linkage(linkage), visibility(visibility), kind(kind) {}

    SymData(const SymData& sd) = default;

    SymData(SymData&& sd) noexcept = default;

    Linkage get_linkage() { return linkage; }

    void set_linkage(Linkage linkage) { this->linkage = linkage; }

    Visibility get_visibility() { return visibility; }

    void set_visibility(Visibility visibility) { this->visibility = visibility; }

    const std::string& get_name() { return name; }

    const std::string& get_mangled_name() { return mangled_name; }

    void set_mangled_name(std::string mangled_name) {
        this->mangled_name = std::move(mangled_name);
    }

    bool has_link_name() { return link_name.has_value(); }

    void set_link_name(std::string name) { link_name = std::move(name); }

protected:
};

class VarSymData : public SymData {
    /**
    The type of the variable symbol.
    */
    types::Type *type;

public:
    VarSymData(std::string name, types::Type *type)
        : SymData(Kind::VAR, std::move(name)), type(type) {}

    VarSymData(std::string name, Linkage linkage, Visibility visibility, types::Type *type)
        : SymData(Kind::VAR, std::move(name), linkage, visibility), type(type) {}

    types::Type *get_type() { return type; }

    void set_type(types::Type *type) { this->type = type; }

    static bool classof(const SymData *data) { return data->kind == Kind::VAR; }
};

class FuncSymData : public SymData {
    /**
    The signature of the function symbol.
    */
    types::FunctionType *signature;

    /**
    Whether this function symbol is the entry point for this object file.
    */
    bool main_function = false;

    /**
    Whether this function symbol is the print function for this object file.
    */
    bool print_function = false;

public:
    FuncSymData(std::string name, types::FunctionType *signature)
        : SymData(Kind::FUNC, std::move(name)), signature(signature) {}

    FuncSymData(
        std::string name, Linkage linkage, Visibility visibility, types::FunctionType *signature,
        bool is_main = false)
        : SymData(Kind::FUNC, std::move(name), linkage, visibility), signature(signature),
          main_function(is_main) {}

    types::FunctionType *get_signature() { return signature; }

    bool is_main() const { return main_function; }

    void set_main(bool is_main) { main_function = is_main; }

    bool is_print() const { return print_function; }
    
    void set_print(bool is_print) { print_function = is_print; }

    static bool classof(const SymData *data) { return data->kind == Kind::FUNC; }
};

} // namespace ecc::sema::sym

#endif