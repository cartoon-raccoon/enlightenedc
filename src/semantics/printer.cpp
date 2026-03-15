#include <iostream>
#include <sstream>

#include "semantics/symbols.hpp"
#include "semantics/types.hpp"

using namespace ecc::sema;
using namespace ecc::sema::sym;
using namespace ecc::sema::types;

std::string VarSymbol::to_string() const {
    std::stringstream ss;

    ss << "VarSymbol ";

    if (is_const)
        ss << "const ";

    if (type)
        ss << type->to_string();
    else
        ss << "<nulltype>";

    if (is_extern)
        ss << " extern";

    if (is_static)
        ss << " static";

    if (is_public)
        ss << " public";

    return ss.str();
}

std::string FuncSymbol::to_string() const {
    std::stringstream ss;

    ss << "FuncSymbol ";

    if (signature)
        ss << signature->to_string();
    else
        ss << "<nullsig>";

    if (is_extern)
        ss << " extern";

    if (is_static)
        ss << " static";

    if (is_public)
        ss << " public";

    return ss.str();
}

std::string TypeSymbol::to_string() const {
    std::stringstream ss;

    ss << "TypeSymbol ";

    if (type)
        ss << type->to_string();
    else
        ss << "<nulltype>";

    return ss.str();
}

std::string LabelSymbol::to_string() const {
    std::stringstream ss;

    ss << "LabelSymbol";

    return ss.str();
}

std::string PrimitiveType::to_string() const {
    switch (primkind) {
    case U0:
        return "U0";
    case U8:
        return "U8";
    case U16:
        return "U16";
    case U32:
        return "U32";
    case U64:
        return "U64";
    case I0:
        return "I0";
    case I8:
        return "I8";
    case I16:
        return "I16";
    case I32:
        return "I32";
    case I64:
        return "I64";
    case F64:
        return "F64";
    case BOOL:
        return "bool";
    }

    return "<primitive>";
}

std::string ClassType::to_string() const {
    std::stringstream ss;

    ss << "class";

    if (complete) {
        ss << " { ";

        bool first = true;
        for (auto const& m : members) {
            if (!first)
                ss << "; ";
            first = false;

            ss << m->ty->to_string() << " " << m->name;
        }

        if (!members.empty())
            ss << "; ";

        ss << "}";
    }

    return ss.str();
}

std::string UnionType::to_string() const {
    std::stringstream ss;

    ss << "union";

    if (complete) {
        ss << " { ";

        bool first = true;
        for (auto const& m : members) {
            if (!first)
                ss << "; ";
            first = false;

            ss << m->ty->to_string() << " " << m->name;
        }

        if (!members.empty())
            ss << "; ";

        ss << "}";
    }

    return ss.str();
}

std::string EnumType::to_string() const {
    std::stringstream ss;

    ss << "enum";

    if (complete) {
        ss << " { ";

        bool first = true;
        for (auto const& e : enumerators) {
            if (!first)
                ss << ", ";
            first = false;

            ss << e->name << " = " << e->value;
        }

        ss << " }";
    }

    return ss.str();
}

std::string PointerType::to_string() const {
    std::stringstream ss;

    if (base)
        ss << base->to_string();
    else
        ss << "<null>";

    ss << " *";

    if (is_const)
        ss << " const";

    return ss.str();
}

std::string ArrayType::to_string() const {
    std::stringstream ss;

    if (base)
        ss << base->to_string();
    else
        ss << "<null>";

    ss << "[";

    if (size)
        ss << *size;

    ss << "]";

    return ss.str();
}

std::string FunctionType::to_string() const {
    std::stringstream ss;

    if (signature.returntype)
        ss << signature.returntype->to_string();
    else
        ss << "<nullret>";

    ss << " (";

    bool first = true;

    for (auto* p : signature.params) {
        if (!first)
            ss << ", ";
        first = false;

        if (p)
            ss << p->to_string();
        else
            ss << "<null>";
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

    for (auto const& [name, type] : base_types) {
        ss << "  " << name << " -> " << type.get() << " : " << type->to_string()
           << "\n";
    }

    ss << "\nPointer Types:\n";

    for (auto const& [key, ptr] : pointers) {
        ss << "  " << ptr.get() << " : " << ptr->to_string() << "\n";
    }

    ss << "\nArray Types:\n";

    for (auto const& [key, arr] : arrays) {
        ss << "  " << arr.get() << " : " << arr->to_string() << "\n";
    }

    ss << "\n";

    return ss.str();
}

static void print_scope(std::stringstream& ss, Scope* scope, int depth) {
    std::string indent(depth * 2, ' ');

    ss << indent << "Scope " << scope << "\n";

    for (auto const& [name, sym] : scope->symbols) {
        ss << indent << "  " << name << " -> " << sym.get() << " : "
           << sym->to_string() << "\n";
    }

    for (auto const& child : scope->nested) {
        print_scope(ss, child.get(), depth + 1);
    }
}

std::string symbol_table_to_string(SymbolTable const& table) {
    std::stringstream ss;

    ss << "--------- SYMBOL TABLE ---------\n\n";

    print_scope(ss, table.global.get(), 0);

    return ss.str();
}