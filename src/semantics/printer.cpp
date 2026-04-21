#include <iostream>
#include <sstream>

#include "semantics/symbols.hpp"
#include "semantics/types.hpp"
#include "tokens.hpp"
#include "util.hpp"

using namespace ecc::sema;
using namespace ecc::sema::sym;
using namespace ecc::sema::types;
using namespace ecc::tokens;

std::string VarSymbol::to_string() const {
    std::stringstream ss;

    ss << "VarSymbol: " << name;

    if (is_const)
        ss << "const ";

    if (type) {
        ss << " :: " << type->formal();
    } else {
        ss << " :: <nulltype>";
    }

    if (linkage == PhysicalSymbol::Linkage::EXTERNAL)
        ss << " extern";

    if (linkage == PhysicalSymbol::Linkage::EXTERNC)
        ss << " extern C";

    if (is_static)
        ss << " static";

    if (is_public)
        ss << " public";

    return ss.str();
}

std::string FuncSymbol::to_string() const {
    std::stringstream ss;

    ss << "FuncSymbol: " << name;

    if (signature) {
        ss << " :: " << signature->to_string();
    } else {
        ss << " :: <nullsig>";
    }

    if (is_static)
        ss << " static";

    if (is_public)
        ss << " public";

    return ss.str();
}

std::string TypeSymbol::to_string() const {
    std::stringstream ss;

    ss << "TypeSymbol: " << name;

    if (type) {
        ss << " :: " << type->to_string();
    } else {
        ss << " :: <nulltype>";
    }

    return ss.str();
}

std::string LabelSymbol::to_string() const {
    std::stringstream ss;

    ss << "LabelSymbol: " << name;

    return ss.str();
}

std::string PrimitiveType::to_string() const {
    return tokens::primitive_to_string(primkind);
}

std::string ClassType::to_string() const {
    std::stringstream ss;

    ss << "class";

    if (name) {
        ss << " " << *name;
    }

    if (is_complete()) {
        ss << " { ";

        bool first = true;
        for (auto const& m : members) {
            if (!first)
                ss << "; ";
            first = false;

            ss << m->ty->to_string() << " " << (m->name ? *m->name : "");
        }

        if (!members.empty())
            ss << "; ";

        ss << "}";
    } else {
        ss << "[INCOMPLETE]";
    }

    return ss.str();
}

std::string UnionType::to_string() const {
    std::stringstream ss;

    ss << "union";

    if (name) {
        ss << " " << *name;
    }

    if (type_rep) {
        ss << ": " << (*type_rep)->to_string() << " ";
    }

    if (is_complete()) {
        ss << " { ";

        bool first = true;
        for (auto const& m : members) {
            if (!first)
                ss << "; ";
            first = false;

            ss << m->ty->to_string() << " " << (m->name ? *m->name : "");
        }

        if (!members.empty())
            ss << "; ";

        ss << "}";
    } else {
        ss << "[INCOMPLETE]";
    }

    return ss.str();
}

std::string EnumType::to_string() const {
    std::stringstream ss;

    ss << "enum";

    if (name) {
        ss << " " << *name;
    }

    if (underlying) {
        ss << ": " << (*underlying).to_string() << " ";
    }

    if (is_complete()) {
        ss << " { ";

        bool first = true;
        for (auto const& e : enumerators) {
            if (!first)
                ss << ", ";
            first = false;

            ss << e->name << " = " << e->value;
        }

        ss << " }";
    } else {
        ss << "[INCOMPLETE]";
    }

    return ss.str();
}

std::string PointerType::to_string() const {
    std::stringstream ss;

    if (base) {
        if (base->get_name()) {
            ss << *(base->get_name());
        } else {
            ss << base->to_string();
        }
    } else {
        ss << "<null>";
    }

    ss << " *";

    if (is_const)
        ss << " const";

    return ss.str();
}

std::string PointerType::formal() {
    std::stringstream ss;

    if (base) {
        ss << base->formal();
    } else {
        ss << "<null>";
    }

    ss << " *";

    if (is_const)
        ss << " const";

    return ss.str();
}

std::string ArrayType::to_string() const {
    std::stringstream ss;

    if (base) {
        if (base->get_name()) {
            ss << *(base->get_name());
        } else {
            ss << base->to_string();
        }
    } else {
        ss << "<null>";
    }

    ss << "[";

    if (arr_size)
        ss << *arr_size;

    ss << "]";

    return ss.str();
}

std::string ArrayType::formal() {
    std::stringstream ss;

    if (base) {
        ss << base->formal();
    } else {
        ss << "<null>";
    }

    ss << "[";

    if (arr_size)
        ss << *arr_size;

    ss << "]";

    return ss.str();
}

std::string FunctionType::to_string() const {
    std::stringstream ss;

    assert(signature.returntype);
    ss << signature.returntype->to_string();

    ss << " (";

    bool first = true;

    for (auto *p : signature.params) {
        if (!first)
            ss << ", ";
        first = false;

        assert(p);
        ss << p->to_string();
    }

    if (signature.variadic) {
        if (!first)
            ss << ", ";
        ss << "...";
    }

    ss << ")";

    return ss.str();
}

std::string FunctionType::formal() {
    std::stringstream ss;

    assert(signature.returntype);
    ss << signature.returntype->formal();

    ss << " (";

    bool first = true;

    for (auto *p : signature.params) {
        if (!first)
            ss << ", ";
        first = false;

        assert(p);
        ss << p->formal();
    }

    if (signature.variadic) {
        if (!first)
            ss << ", ";
        ss << "...";
    }

    ss << ")";

    return ss.str();
}

std::string TypeContext::to_string() const {
    std::stringstream ss;

    ss << "--------- TYPE CONTEXT ---------\n\n";

    ss << "Base Types:\n";

    for (auto const& [name, type] : user_types) {
        ss << "  " << name << " : " << type->to_string() << "\n";
    }

    ss << "\nFunction Types:\n";

    for (auto const& [name, type] : function_types) {
        ss << "  " << name << " : " << type->to_string() << "\n";
    }

    ss << "\nPointer Types:\n";

    for (auto const& [key, ptr] : pointers) {
        ss << "  :" << ptr->to_string() << "\n";
    }

    ss << "\nArray Types:\n";

    for (auto const& [key, arr] : arrays) {
        ss << " : " << arr->to_string() << " #" << arr->ref_count << "\n";
    }

    ss << "\n";

    return ss.str();
}

static void print_scope(std::stringstream& ss, Scope *scope, int depth) {
    std::string indent(depth * 2, ' ');

    ss << indent << "Scope " << scope->id;
    if (scope->assoc) {
        ss << ": " << scope->assoc->to_string();
    } else {
        ss << ": Compound Statement";
    }
    ss << "\n";

    if (!scope->phys_symbols.empty()) {
        ss << indent << "Physical Symbols:\n";
        for (auto const& [name, sym] : scope->phys_symbols) {
            ss << indent << "  " << name << " <" << sym.get() << "> : " << sym->to_string()
               << "\n";
        }
    }

    if (!scope->type_symbols.empty()) {
        ss << "\n" << indent << "Type Symbols:\n";
        for (auto const& [name, sym] : scope->type_symbols) {
            ss << indent << "  " << name << " <" << sym.get() << "> : " << sym->to_string()
               << "\n";
        }
    }

    if (!scope->label_symbols.empty()) {
        ss << "\n" << indent << "Label Symbols:\n";
        for (auto const& [name, sym] : scope->label_symbols) {
            ss << indent << "  " << name << " <" << sym.get() << "> : " << sym->to_string()
               << "\n";
        }
    }

    ss << "-----------------------------------------------------------\n";

    for (auto const& child : scope->nested) {
        print_scope(ss, child.get(), depth + 1);
    }
}

std::string SymbolTable::to_string() const {
    std::stringstream ss;

    ss << "--------- SYMBOL TABLE ---------\n\n";

    print_scope(ss, global.get(), 0);

    return ss.str();
}