#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <utility>
#include <variant>

#include "semantics/types.hpp"
#include "util.hpp"

using namespace ecc::sema::types;

bool Type::is_primitive() { return kind == Kind::PRIMITIVE; }

bool Type::is_pointer() { return kind == Kind::POINTER; }

bool PrimitiveType::is_integer() {
    // We maintain strict integral definitions for determining whether
    // this type is an integer: it cannot be Bool or Float, and it has to be sized.
    switch (primkind) {
        case U8:
        case U16:
        case U32:
        case U64:
        case I8:
        case I16:
        case I32:
        case I64:
        return true;
        
        case U0:
        case I0:
        case BOOL:
        case F64:
        return false;
    }
    
    return false;
}

bool PrimitiveType::is_compatible_with(Type *dst) {
    // Only primitive types allowed
    if (dst->kind != Type::Kind::PRIMITIVE) {
        return false;
    }

    PrimitiveType *new_other = static_cast<PrimitiveType *>(dst);

    // If either is not an integer, return false
    if (!this->is_integer() || !new_other->is_integer()) {
        return false;
    }

    int my_size = this->size();

    return true ? 0 < my_size && my_size <= new_other->size() : false;
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

std::string PrimitiveType::formal() {
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
        case BOOL:
        return "Bool";
        case F64:
        return "F64";
    }
}

void ClassType::add_member(Box<ClassType::ClassTypeMember> member) {
    if (member->name)
        dbprint("ClassType: adding member with name ", *member->name, " type ", member->ty);
    else {
        dbprint("ClassType: adding anonymous member with type ", member->ty);
    }
    members.push_back(std::move(member));
}

ClassType::ClassTypeMember *ClassType::find(std::string& name) {
    for (auto& mem : members) {
        if (mem->name) {
            if (mem->name == name) {
                return mem.get();
            }
        }
    }

    return nullptr;
}

std::string ClassType::formal() {
    if (name) {
        return base() + *name;
    } else {
        return "class_anon";
    }
}

void UnionType::add_member(Box<UnionType::UnionTypeMember> member) {
    if (member->name)
        dbprint("UnionType: adding member with name ", *member->name, " type ", member->ty);
    else {
        dbprint("UnionType: adding anonymous member with type ", member->ty);
    }
    members.push_back(std::move(member));
}

UnionType::UnionTypeMember *UnionType::find(std::string& name) {
    for (auto& mem : members) {
        if (mem->name) {
            if (mem->name == name) {
                return mem.get();
            }
        }
    }

    return nullptr;
}

std::string UnionType::formal() {
    if (name) {
        return base() + *name;
    } else {
        return "anon_union";
    }
}

void EnumType::add_enumerator(std::string enumerator, Location loc) {

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
    
    add_enumerator(enumerator, value, loc);
}

void EnumType::add_enumerator(std::string enumerator, uint64_t value, Location loc) {
    Box<EnumTypeMember> member = std::make_unique<EnumTypeMember>(enumerator, value, loc);

    values.insert(value);

    enumerators.push_back(std::move(member));
}

EnumType::EnumTypeMember *EnumType::find(std::string& name) {
    for (auto& member : enumerators) {
        if (name == member->name)
            return member.get();
    }

    return nullptr;
}

bool EnumType::is_compatible_with(Type *dst) {
    if (!dst->is_primitive()) {
        return false;
    }

    PrimitiveType *prim = dst->as_primitive();
    return prim->is_integer();
}

std::string EnumType::formal() {
    if (name) {
        return std::format("enum_{}", *name);
    } else {
        return "anon_enum";
    }
}

int PointerType::nesting_lvl() {
    int lvl = 1;

    Type *current = base;
    while (current->is_pointer()) {
        current = current->as_pointer()->base;
        lvl++;
    }

    return lvl;
}

bool PointerType::is_compatible_with(Type *dst) {
    return false; // todo
}

std::size_t FunctionType::hash_sig() {
    std::size_t h = std::hash<Type *>{}(signature.returntype);

    for (auto *p : signature.params) {
        h ^= std::hash<Type*>{}(p) + 0x9e3779b9 + (h << 6) + (h >> 2);
    }

    return h;
}

bool ArrayType::is_compatible_with(Type *dst) {
    return false; // todo
}

void TypeBuilder::add_array(uint64_t size) {
    type_stack.push(Arr {size});
}

void TypeBuilder::add_array() {
    type_stack.push(Arr {{}});
}

void TypeBuilder::add_pointer(bool is_const) {
    type_stack.push(Ptr {is_const});
}

void TypeBuilder::add_function(Vec<FuncParam> params, bool variadic) {
    type_stack.push(FnParams {std::move(params), variadic});
}

void TypeBuilder::set_base(BaseType *base) {
    this->base = base;
}

Type *TypeBuilder::finalize() {
    if (!base) {
        throw std::runtime_error("TypeBuilder::finalize: cannot construct type from null base");
    }

    dbprint("TypeBuilder: finalizing type");
    
    Type *curr = base;
    while (!type_stack.empty()) {
        auto next_cstrctr = type_stack.top();
        std::visit(overloaded{
            [this, &curr] (Arr& a) mutable {
                // Wrap the base in an array.
                if (a.size) {
                    curr = this->ctxt.get_array(curr, *a.size);
                } else {
                    curr = this->ctxt.get_array(curr);
                }
            },
            [this, &curr] (Ptr& p) mutable {
                // Wrap the base in a pointer.
                curr = this->ctxt.get_pointer(curr, p.is_const);
            },
            [this, &curr] (FnParams& fn) mutable {
                Vec<Type *> params;
                // map out the identifiers.
                params.reserve(fn.params.size());
                for (auto& param : fn.params) {
                    params.push_back(param.type);
                }
                // Wrap the base as the return type in a function type.
                curr = this->ctxt.get_function(curr, std::move(params), fn.variadic);
            }
        }, next_cstrctr);

        // Pop the stack to the next constructor
        type_stack.pop();
    }

    return curr;
}

TypeContext::TypeContext() {
    // Initialize the TypeContext with primitive types.
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
}

TypeBuilder TypeContext::builder() {
    return TypeBuilder(*this);
}

PrimitiveType *TypeContext::get_primitive(PrimitiveType::Kind pkind) {
    dbprint("TypeContext: getting primitive type with value ", pkind);
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

ClassType *TypeContext::get_class(std::string name, sym::Scope *scope) {
    dbprint("TypeContext: class type '", name, "' on scope ", scope);
    std::string mangled = mangle<ClassType>(name, scope);

    if (base_types.contains(mangled)) {
        dbprint("TypeContext: existing class found");
        return base_types.find(mangled)->second.get()->as_class();
    }

    // If no struct matching the name, make a new struct
    return make_insert_type<ClassType>(mangled, name);
}

ClassType *TypeContext::get_class(sym::Scope *scope) {
    /*
    In EnlightenedC (and most C-family languages), two structs are considered
    equal iff they have the same name. (If two structs are named the same but
    have different definitions, that is a separate semantic violation.)

    An anonymous struct that has the exact same members as a named struct
    is not the same type as the named struct.
    */
    dbprint("TypeContext: anonymous class type on scope ", scope);
    auto name = "anon_struct_" + std::to_string(anonymous_ctr);
    anonymous_ctr++;
    auto mangled = mangle<ClassType>(name, scope);

    return make_insert_type<ClassType>(mangled);
}

UnionType *TypeContext::get_union(std::string name, sym::Scope *scope) {
    dbprint("TypeContext: union type '", name, "' on scope ", scope);
    std::string mangled = mangle<UnionType>(name, scope);

    if (base_types.contains(mangled)) {
        dbprint("TypeContext: existing union found");
        return base_types.find(mangled)->second.get()->as_union();
    }

    // If no struct matching the name, make a new struct
    return make_insert_type<UnionType>(mangled, name);
}

UnionType *TypeContext::get_union(sym::Scope *scope) {
    dbprint("TypeContext: anonymous union type on scope ", scope);
    auto name = "anon_union_" + std::to_string(anonymous_ctr);
    anonymous_ctr++;
    auto mangled = mangle<UnionType>(name, scope);

    return make_insert_type<UnionType>(mangled);
}

EnumType *TypeContext::get_enum(std::string name, sym::Scope *scope) {
    dbprint("TypeContext: enum type '", name, "' on scope ", scope);
    std::string mangled = mangle<EnumType>(name, scope);

    if (base_types.contains(mangled)) {
        dbprint("TypeContext: existing enum found");
        return base_types.find(mangled)->second.get()->as_enum();
    }

    // If no struct matching the name, make a new struct
    return make_insert_type<EnumType>(mangled, name);
}

EnumType *TypeContext::get_enum(sym::Scope *scope) {
    dbprint("TypeContext: anonymous enum type on scope ", scope);
    auto name = "anon_enum_" + std::to_string(anonymous_ctr);
    anonymous_ctr++;
    auto mangled = mangle<EnumType>(name, scope);

    return make_insert_type<EnumType>(mangled);
}

PointerType *TypeContext::get_pointer(Type *base, bool is_const) {
    dbprint("TypeContext: pointer with base ", base);
    // Check for base in our pointer store, if exists, return immediately
    auto it = pointers.find({base, is_const});
    if (it != pointers.end()) {
        return it->second.get();
    }

    // We don't need to check if base exists in our base type store,
    // since base is a Type *, we can assume it has already been created.

    // If not found, create a new pointer.
    Box<PointerType> ptr = std::make_unique<PointerType>(base);
    ptr->is_const = is_const;
    auto ret = ptr.get();
    pointers[{base, is_const}] = std::move(ptr);

    return ret;
}

ArrayType *TypeContext::get_array(Type *base, uint64_t size) {
    dbprint("TypeContext: array type with base ", base, ", size ", size);
    ArrayKey key(base, size);
    auto it = arrays.find(key);
    if (it != arrays.end()) {
        return it->second.get();
    }

    Box<ArrayType> arr = std::make_unique<ArrayType>(base, size);
    auto ret = arr.get();
    arrays[key] = std::move(arr);

    return ret;
}

ArrayType *TypeContext::get_array(Type *base) {
    dbprint("TypeContext: array type with base ", base, ", no size");
    ArrayKey key(base, {});
    auto it = arrays.find(key);
    if (it != arrays.end()) {
        return it->second.get();
    }

    Box<ArrayType> arr = std::make_unique<ArrayType>(base);
    auto ret = arr.get();
    arrays[key] = std::move(arr);

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

    dbprint("TypeContext: getting function type");
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

    if (base_types.contains(name)) {
        dbprint("TypeConstext: existing function type found");
        return base_types.find(name)->second.get()->as_function();
    }

    base_types[name] = std::move(func);

    return ret;
}

