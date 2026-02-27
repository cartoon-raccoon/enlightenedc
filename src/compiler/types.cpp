#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>

#include "compiler/types.hpp"

using namespace ecc::compiler::types;

bool Type::is_primitive() { return kind == Kind::Primitive; }

bool Type::is_pointer() { return kind == Kind::Pointer; }

void StructType::add_member(Box<StructType::StructTypeMember> member) {
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

bool PrimitiveType::is_compatible_with(Type *other) {
    if (other->kind != Type::Kind::Primitive) {
        return false;
    }

    PrimitiveType *new_other = static_cast<PrimitiveType *>(other);

    int my_size = this->size();

    return true ? 0 < my_size && my_size <= new_other->size() : false;
}

StructType *TypeContext::get_struct(std::string name) {
    if (base_types.contains(name)) {
        return base_types.find(name)->second.get()->as_struct();
    }

    // If no struct matching the name, make a new struct
    return make_insert_type<StructType>(name);
}

StructType *TypeContext::get_struct() {
    /*
    In EnlightenedC (and most C-family languages), two structs are considered
    equal iff they have the same name. (If two structs are named the same but
    have different definitions, that is a separate semantic violation.)

    An anonymous struct that has the exact same members as a named struct
    is not the same type as the named struct.
    */
    
    auto name = "anon_struct_" + std::to_string(anonymous_ctr);
    anonymous_ctr++;

    return make_insert_type<StructType>(name);
}

UnionType *TypeContext::get_union(std::string name) {
    if (base_types.contains(name)) {
        return base_types.find(name)->second.get()->as_union();
    }

    // If no struct matching the name, make a new struct
    return make_insert_type<UnionType>(name);
}

UnionType *TypeContext::get_union() {
    auto name = "anon_union_" + std::to_string(anonymous_ctr);
    anonymous_ctr++;

    return make_insert_type<UnionType>(name);
}

EnumType *TypeContext::get_enum(std::string name) {
    if (base_types.contains(name)) {
        return base_types.find(name)->second.get()->as_enum();
    }

    // If no struct matching the name, make a new struct
    return make_insert_type<EnumType>(name);
}

EnumType *TypeContext::get_enum() {
    auto name = "anon_enum_" + std::to_string(anonymous_ctr);
    anonymous_ctr++;

    return make_insert_type<EnumType>(name);
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
        name = "function_v_" + std::to_string(hash);
    else
        name = "function_" + std::to_string(hash);

    base_types[name] = std::move(func);

    return ret;
}

