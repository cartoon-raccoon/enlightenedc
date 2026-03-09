#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>

#include "semantics/types.hpp"

using namespace ecc::sema::types;

bool Type::is_primitive() { return kind == Kind::Primitive; }

bool Type::is_pointer() { return kind == Kind::Pointer; }

void ClassType::add_member(Box<ClassType::ClassTypeMember> member) {
    members.push_back(std::move(member));
}

void UnionType::add_member(Box<UnionType::UnionTypeMember> member) {
    members.push_back(std::move(member));
}

void EnumType::add_enumerator(std::string enumerator) {

    /*
    Determine the value to use: If no values exist, start at 1.
    If values already exist, take the max value + 1.
    */
    auto it = std::max_element(values.begin(), values.end());
    int value;
    if (it != values.end()) {
        value = *it + 1;
    } else {
        value = 1;
    }
    
    add_enumerator(enumerator, value);
}

void EnumType::add_enumerator(std::string enumerator, int value) {
    Box<EnumTypeMember> member = std::make_unique<EnumTypeMember>(enumerator, value);

    values.insert(value);

    enumerators.push_back(std::move(member));
}

std::size_t FunctionType::hash_sig() {
    std::size_t h = std::hash<Type *>{}(signature.returntype);

    for (auto *p : signature.params) {
        h ^= std::hash<Type*>{}(p) + 0x9e3779b9 + (h << 6) + (h >> 2);
    }

    return h;
}


TypeContext::TypeContext() {
    base_types.insert({"U0", std::make_unique<PrimitiveType>(PrimitiveType::Kind::U0)});
    base_types.insert({"U8", std::make_unique<PrimitiveType>(PrimitiveType::Kind::U8)});
    base_types.insert({"U16", std::make_unique<PrimitiveType>(PrimitiveType::Kind::U16)});
    base_types.insert({"U32", std::make_unique<PrimitiveType>(PrimitiveType::Kind::U32)});
    base_types.insert({"U64", std::make_unique<PrimitiveType>(PrimitiveType::Kind::U64)});
    base_types.insert({"I0", std::make_unique<PrimitiveType>(PrimitiveType::Kind::I0)});
    base_types.insert({"I8", std::make_unique<PrimitiveType>(PrimitiveType::Kind::I8)});
    base_types.insert({"I16", std::make_unique<PrimitiveType>(PrimitiveType::Kind::I16)});
    base_types.insert({"I32", std::make_unique<PrimitiveType>(PrimitiveType::Kind::I32)});
    base_types.insert({"I64", std::make_unique<PrimitiveType>(PrimitiveType::Kind::I64)});
    base_types.insert({"F64", std::make_unique<PrimitiveType>(PrimitiveType::Kind::F64)});
    base_types.insert({"bool", std::make_unique<PrimitiveType>(PrimitiveType::Kind::BOOL)});
} // Initialize the TypeContext with primitive types.

PrimitiveType *TypeContext::get_primitive(PrimitiveType::Kind pkind) {
    switch (pkind) {
    case PrimitiveType::Kind::U0:
        return base_types.find("U0")->second.get()->as_primitive();
    case PrimitiveType::Kind::U8:
        return base_types.find("U8")->second.get()->as_primitive();
    case PrimitiveType::Kind::U16:
        return base_types.find("U16")->second.get()->as_primitive();
    case PrimitiveType::Kind::U32:
        return base_types.find("U32")->second.get()->as_primitive();
    case PrimitiveType::Kind::U64:
        return base_types.find("U64")->second.get()->as_primitive();
    case PrimitiveType::Kind::I0:
        return base_types.find("I0")->second.get()->as_primitive();
    case PrimitiveType::Kind::I8:
        return base_types.find("I8")->second.get()->as_primitive();
    case PrimitiveType::Kind::I16:
        return base_types.find("I16")->second.get()->as_primitive();
    case PrimitiveType::Kind::I32:
        return base_types.find("I32")->second.get()->as_primitive();
    case PrimitiveType::Kind::I64:
        return base_types.find("I64")->second.get()->as_primitive();
    case PrimitiveType::Kind::F64:
        return base_types.find("F64")->second.get()->as_primitive();
    case PrimitiveType::Kind::BOOL:
        return base_types.find("bool")->second.get()->as_primitive();
    default:
        return nullptr;
    }

    std::unreachable();
}

int PrimitiveType::size() {
    switch (primkind) {
        case U0:
        case I0:
        return 0;

        case U8:
        case I8:
        case BOOL:
        return 1;

        case U16:
        case I16:
        return 2;

        case U32:
        case I32:
        return 4;

        case U64:
        case I64:
        case F64:
        return 8;
    }

    std::unreachable();
}

bool PrimitiveType::is_integer() {
    switch (primkind) {
        case U0:
        case U8:
        case U16:
        case U32:
        case U64:
        case I0:
        case I8:
        case I16:
        case I32:
        case I64:
        case BOOL:
        return true;
        case F64:
        return false;
    }
    
    return false;
}

bool PrimitiveType::is_compatible_with(Type *other) {
    // Only primitive types allowed
    if (other->kind != Type::Kind::Primitive) {
        return false;
    }

    PrimitiveType *new_other = static_cast<PrimitiveType *>(other);

    // If either is not an integer, return false
    if (!this->is_integer() || !new_other->is_integer()) {
        return false;
    }

    int my_size = this->size();

    return true ? 0 < my_size && my_size <= new_other->size() : false;
}

bool PointerType::is_compatible_with(Type *other) {
    return false; // todo
}

bool EnumType::is_compatible_with(Type *other) {
    return false; // todo
}

ClassType *TypeContext::get_class(std::string name, sym::Scope *scope) {
    std::string mangled = mangle<ClassType>(name, scope);

    if (base_types.contains(mangled)) {
        return base_types.find(mangled)->second.get()->as_class();
    }

    // If no struct matching the name, make a new struct
    return make_insert_type<ClassType>(mangled);
}

ClassType *TypeContext::get_class(sym::Scope *scope) {
    /*
    In EnlightenedC (and most C-family languages), two structs are considered
    equal iff they have the same name. (If two structs are named the same but
    have different definitions, that is a separate semantic violation.)

    An anonymous struct that has the exact same members as a named struct
    is not the same type as the named struct.
    */
    
    auto name = "anon_struct_" + std::to_string(anonymous_ctr);
    anonymous_ctr++;
    auto mangled = mangle<ClassType>(name, scope);

    return make_insert_type<ClassType>(mangled);
}

UnionType *TypeContext::get_union(std::string name, sym::Scope *scope) {
    std::string mangled = mangle<UnionType>(name, scope);

    if (base_types.contains(mangled)) {
        return base_types.find(mangled)->second.get()->as_union();
    }

    // If no struct matching the name, make a new struct
    return make_insert_type<UnionType>(name);
}

UnionType *TypeContext::get_union(sym::Scope *scope) {
    auto name = "anon_union_" + std::to_string(anonymous_ctr);
    anonymous_ctr++;
    auto mangled = mangle<UnionType>(name, scope);

    return make_insert_type<UnionType>(mangled);
}

EnumType *TypeContext::get_enum(std::string name, sym::Scope *scope) {
    std::string mangled = mangle<EnumType>(name, scope);

    if (base_types.contains(mangled)) {
        return base_types.find(mangled)->second.get()->as_enum();
    }

    // If no struct matching the name, make a new struct
    return make_insert_type<EnumType>(mangled);
}

EnumType *TypeContext::get_enum(sym::Scope *scope) {
    auto name = "anon_enum_" + std::to_string(anonymous_ctr);
    anonymous_ctr++;
    auto mangled = mangle<EnumType>(name, scope);

    return make_insert_type<EnumType>(mangled);
}

PointerType *TypeContext::get_pointer(Type *base) {
    // Check for base in our pointer store, if exists, return immediately
    auto it = pointers.find(base);
    if (it != pointers.end()) {
        return it->second.get();
    }

    // We don't need to check if base exists in our base type store,
    // since base is a Type *, we can assume it has already been created.

    // If not found, create a new pointer.
    Box<PointerType> ptr = std::make_unique<PointerType>(base);
    auto ret = ptr.get();
    pointers[base] = std::move(ptr);

    return ret;
}

FunctionType *TypeContext::get_function(Type *returntype, Vec<Type *> params, bool variadic) {
    /*
    Function types do not need to be scope-aware, since their names are purely symbolic, and
    have no bearing on type equality. If a pointer to a function that was declared in a non-global scope
    is used where it is not declared, the SymbolTable resolves that automatically. We have to account for
    scope with named types like class, union, and enum because their name is the main key to type equality,
    and named types in a nested scope shadow any type with the same name in an outer scope.
    */

    Box<FunctionType> func = std::make_unique<FunctionType>();
    FunctionType *ret = func.get();

    FunctionType::FunctionSignature sig = {returntype, std::move(params), variadic};
    func->signature = std::move(sig);
    
    /*
    function type naming convention:

    "function_v_<sig hash>" if variadic,
    "function_<sig hash>" if not.
    */
    size_t hash = func->hash_sig();
    std::string name;
    if (variadic)
        name = "function_v" + std::to_string(hash);
    else
        name = "function_" + std::to_string(hash);

    base_types[name] = std::move(func);

    return ret;
}

