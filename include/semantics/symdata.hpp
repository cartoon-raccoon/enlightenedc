#pragma once

#ifndef ECC_SYMDATA_H
#define ECC_SYMDATA_H

#include <cstdint>
#include <string>

#include "semantics/types.hpp"
#include "prelude.hpp"

namespace ecc::sema::sym {

// The linkage of the symbol.
enum class Linkage : uint8_t {
    // The symbol has no linkage.
    NONE,
    // The symbol has internal linkage.
    INTERNAL,
    // The symbol has external linkage.
    EXTERNAL,
};

enum class LangLinkage : uint8_t {
    NONE, // The symbol has no language linkage.
    C, // The symbol has "C" language linkage.
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
    Linkage linkage = Linkage::NONE;

    Optional<std::string> link_name;

public:
    enum class Kind : uint8_t {
        VAR,
        FUNC,
    };
    const Kind kind;

    SymData(Kind kind, StringRef name) : name(name), kind(kind) {}

    SymData(Kind kind, StringRef name, Linkage linkage)
        : name(name), linkage(linkage), kind(kind) {}

    SymData(const SymData& sd) = default;

    SymData(SymData&& sd) noexcept = default;

    Linkage get_linkage() { return linkage; }

    void set_linkage(Linkage linkage) { this->linkage = linkage; }

    const std::string& get_name() const { return name; }

    const std::string& get_mangled_name() const { return mangled_name; }

    void set_mangled_name(StringRef mangled_name) { this->mangled_name = mangled_name; }

    bool has_link_name() { return link_name.has_value(); }

    void set_link_name(StringRef name) { link_name = std::string(name); }

protected:
};

class VarSymData : public SymData {
    /**
    The type of the variable symbol.
    */
    types::Type *type;

public:
    VarSymData(StringRef name, types::Type *type) : SymData(Kind::VAR, name), type(type) {}

    VarSymData(StringRef name, Linkage linkage, types::Type *type)
        : SymData(Kind::VAR, name, linkage), type(type) {}

    types::Type *get_type() { return type; }

    void set_type(types::Type *type) { this->type = type; }

    static bool classof(const SymData *data) { return data->kind == Kind::VAR; }
};

class FuncSymData : public SymData {
    /**
    The signature of the function symbol.
    */
    types::FunctionType *signature;

    LangLinkage langlink = LangLinkage::NONE;

    /**
    Whether this function symbol is the entry point for this object file.
    */
    bool main_function = false;

    /**
    Whether this function symbol is the print function for this object file.
    */
    bool print_function = false;

public:
    FuncSymData(StringRef name, types::FunctionType *signature)
        : SymData(Kind::FUNC, name), signature(signature) {}

    FuncSymData(
        StringRef name, Linkage linkage,
        types::FunctionType *signature, bool is_main = false)
        : SymData(Kind::FUNC, name, linkage), signature(signature),
          main_function(is_main) {}

    types::FunctionType *get_signature() { return signature; }

    void set_lang_linkage(LangLinkage langlink) { this->langlink = langlink; }

    LangLinkage get_lang_linkage() { return langlink; }

    bool is_main() const { return main_function; }

    void set_main(bool is_main) { main_function = is_main; }

    bool is_print() const { return print_function; }

    void set_print(bool is_print) { print_function = is_print; }

    static bool classof(const SymData *data) { return data->kind == Kind::FUNC; }
};

} // namespace ecc::sema::sym

#endif