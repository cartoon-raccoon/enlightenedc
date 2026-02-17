#include <cstddef>
#include <memory>

#include "compiler/types.hpp"

using namespace ecc::compiler::types;

bool Type::is_primitive() { return kind == Kind::Primitive; }

bool Type::is_pointer() { return kind == Kind::Pointer; }

void StructType::add_member(Box<StructType::StructTypeMember> member) {
    members.push_back(std::move(member));
}

void StructType::finalize() {
    // todo

    complete = true;
}

void UnionType::add_member(Box<UnionType::UnionTypeMember> member) {
    members.push_back(std::move(member));
}

// Marks a union as fully defined, and calculates offsets.
void UnionType::finalize() {
    // todo

    complete = true;
}

void EnumType::add_enumerator(std::string enumerator) {
    enumerators.push_back(enumerator);
}

// Marks an enum as fully defined.
void EnumType::finalize() {
    // todo

    complete = true;
}

std::size_t FunctionType::hash_sig() {
    std::size_t h = std::hash<Type *>{}(signature.returntype);

    for (auto *p : signature.params) {
        h ^= std::hash<Type*>{}(p) + 0x9e3779b9 + (h << 6) + (h >> 2);
    }

    return h;
}


TypeContext::TypeContext() {
    types.insert({"U0", std::make_unique<PrimitiveType>(PrimitiveType::Kind::U0)});
    types.insert({"U8", std::make_unique<PrimitiveType>(PrimitiveType::Kind::U8)});
    types.insert({"U16", std::make_unique<PrimitiveType>(PrimitiveType::Kind::U16)});
    types.insert({"U32", std::make_unique<PrimitiveType>(PrimitiveType::Kind::U32)});
    types.insert({"U64", std::make_unique<PrimitiveType>(PrimitiveType::Kind::U64)});
    types.insert({"I0", std::make_unique<PrimitiveType>(PrimitiveType::Kind::I0)});
    types.insert({"I8", std::make_unique<PrimitiveType>(PrimitiveType::Kind::I8)});
    types.insert({"I16", std::make_unique<PrimitiveType>(PrimitiveType::Kind::I16)});
    types.insert({"I32", std::make_unique<PrimitiveType>(PrimitiveType::Kind::I32)});
    types.insert({"I64", std::make_unique<PrimitiveType>(PrimitiveType::Kind::I64)});
    types.insert({"F64", std::make_unique<PrimitiveType>(PrimitiveType::Kind::F64)});
} // Initialize the TypeContext with primitive types.

PrimitiveType *TypeContext::get_primitive(PrimitiveType::Kind kind) {
    switch (kind) {
    case PrimitiveType::Kind::U0:
        return types.find("U0")->second.get()->as_primitive();
    case PrimitiveType::Kind::U8:
        return types.find("U8")->second.get()->as_primitive();
    case PrimitiveType::Kind::U16:
        return types.find("U16")->second.get()->as_primitive();
    case PrimitiveType::Kind::U32:
        return types.find("U32")->second.get()->as_primitive();
    case PrimitiveType::Kind::U64:
        return types.find("U64")->second.get()->as_primitive();
    case PrimitiveType::Kind::I0:
        return types.find("I0")->second.get()->as_primitive();
    case PrimitiveType::Kind::I8:
        return types.find("I8")->second.get()->as_primitive();
    case PrimitiveType::Kind::I16:
        return types.find("I16")->second.get()->as_primitive();
    case PrimitiveType::Kind::I32:
        return types.find("I32")->second.get()->as_primitive();
    case PrimitiveType::Kind::I64:
        return types.find("I64")->second.get()->as_primitive();
    case PrimitiveType::Kind::F64:
        return types.find("F64")->second.get()->as_primitive();
    }

    // should be unreachable
    return nullptr;
}

StructType *TypeContext::get_struct(std::string name) {
    if (types.contains(name)) {
        return types.find(name)->second.get()->as_struct();
    }

    // If no struct matching the name, make a new struct
    Box<StructType> s = std::make_unique<StructType>();

    auto ret = s.get();
    types.insert({name, std::move(s)});

    return ret;
}

StructType *TypeContext::get_struct() {
    return nullptr; // todo
}

UnionType *TypeContext::get_union(std::string name) {
    return nullptr; // todo
}

UnionType *TypeContext::get_union() {
    return nullptr; // todo
}

PointerType *TypeContext::get_pointer(Type *base) {
    // If base is a pointer, dereference it until we hit the base type
    int nesting = 0;
    while (base->is_pointer()) {
        // todo: add check that pointer does not resolve to null
        base = base->as_pointer()->base;
        nesting++;
    }

    return nullptr; // todo
}

FunctionType *TypeContext::get_function(Type *ret, Vec<Type *> params) {
    return nullptr; // todo
}

