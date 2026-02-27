#ifndef ECC_TYPES_H
#define ECC_TYPES_H

#include "util.hpp"
#include <cstddef>
#include <string>
#include <concepts>
#include <unordered_map>
#include <unordered_set>

namespace ecc::compiler::types {
/*
The type system implementation for EnlightenedC.
*/

using namespace util;
using namespace ecc;

class PrimitiveType;
class StructType;
class UnionType;
class EnumType;
class PointerType;
class FunctionType;
class TypeContext;

/*
The base Type class.

EnlightenedC uses an interning type system that stores singletons of each type encountered
in compilation of the translation unit. Derived types hold pointers to their base types,
making checking type equality as easy as a pointer comparison.

Type size and member alignment is calculated at compile time, using LLVM's `DataLayout` class.
*/
class Type {
public:
    enum Kind {
        Primitive,
        Struct,
        Union,
        Enum,
        Pointer,
        Function,
    };

    // The kind of the type.
    Kind kind;

    bool is_primitive();
    bool is_pointer();

    /*
    Check if a type can be implicitly coerced into `other`.
    */
    virtual bool is_compatible_with(Type * other) {
        // at the minimum, enforce strict equality (no subtyping).
        return other == this;
    }

    /*
    Cast this type to a PrimitiveType *.

    Returns null if the underlying type is not a PrimitiveType.
    */
    virtual PrimitiveType *as_primitive() { return nullptr; }

    /*
    Cast this type to a StructType *.

    Returns null if the underlying type is not a StructType.
    */
    virtual StructType *as_struct() { return nullptr; }
    virtual UnionType *as_union() { return nullptr; }
    virtual EnumType *as_enum() { return nullptr; }
    virtual PointerType *as_pointer() { return nullptr; }
    virtual FunctionType *as_function() { return nullptr; }

protected:
    Type(Kind kind) : kind(kind) {}
    
    friend class TypeContext;
};

/*
A class representing a primitive type (e.g. U8, U16, F64, etc.)

Primitive types do not hold pointers to other Types, i.e. all
derived types should resolve to a primitive type.

## Compatibility

Any non-zero-sized PrimitiveType a can implicitly resolve to another
PrimitiveType b if `b.size() >= a.size()`.
*/
class PrimitiveType : public Type {
public:
    enum Kind {
        U0,
        U8,
        U16,
        U32,
        U64,
        I0,
        I8,
        I16,
        I32,
        I64,
        F64,
        BOOL,
    };

    Kind primkind;

    int size();

    /*
    Checks if 
    */
    bool is_compatible_with(Type *other) override;

    PrimitiveType *as_primitive() override { return this; }

protected:
    friend class TypeContext;

    friend constexpr Box<PrimitiveType> std::make_unique<PrimitiveType>(Kind&&);

    PrimitiveType(Kind kind) : Type(Type::Kind::Primitive), primkind(kind) {}
};

class StructType : public Type {
public:
    struct StructTypeMember {
        std::string name;
        Type *ty;
        
        StructTypeMember(std::string name, Type *ty) : name(name), ty(ty) {}
    };

    /*
    Whether the struct definition is complete.

    This is used in forward declarations of structs that may not have been fully defined.
    Once a struct has been defined (i.e. members have been set), this is marked true,
    preventing the double-definition problem.
    */
    bool complete = false;

    Vec<Box<StructTypeMember>> members;

    void add_member(Box<StructTypeMember> member);

    StructType *as_struct() override { return this; }

protected:
    friend class TypeContext;

    friend constexpr Box<StructType> std::make_unique<StructType>();

    // Construct an empty struct.
    StructType() : Type(Type::Kind::Struct), members() {}
};


class UnionType : public Type {
public:
    struct UnionTypeMember {
        std::string name;
        Type *ty;
    };

    bool complete = false;

    Vec<Box<UnionTypeMember>> members;

    void add_member(Box<UnionTypeMember> member);

    // Marks a union as fully defined, and calculates offsets.
    void finalize();

    UnionType *as_union() override { return this; }

protected:
    friend class TypeContext;

    friend constexpr Box<UnionType> std::make_unique<UnionType>();

    UnionType() : Type(Type::Kind::Union) {}
};

class EnumType : public Type {
public:
    struct EnumTypeMember {
        // the name of the enum variant as declared in the source.
        std::string name;
        // the assigned value of the enum variant.
        int value;
    };

    bool complete = false;

    Vec<Box<EnumTypeMember>> enumerators;

    // Create an enumerator with an automatically chosen value.
    void add_enumerator(std::string enumerator);

    // Create an enumerator with a provided value.
    void add_enumerator(std::string enumerator, int value);

    // Marks an enum as fully defined.
    void finalize();

    EnumType *as_enum() override { return this; }

protected:
    friend class TypeContext;

    friend constexpr Box<EnumType> std::make_unique<EnumType>();

    EnumType() : Type(Type::Kind::Enum), enumerators() {}

private:
    // existing values that have already been declared.
    std::unordered_set<int> values;
};


class PointerType : public Type {
public:
    Type *base;

    PointerType *as_pointer() override { return this; }

protected:
    friend class TypeContext;

    friend constexpr Box<PointerType> std::make_unique<PointerType>(Type *&);
    
    PointerType(Type *base) : Type(Type::Kind::Pointer), base(base) {}
};


class FunctionType : public Type {
public:
    struct FunctionSignature {
        Type *returntype;
        Vec<Type *> params;
        bool variadic;
    } signature;

    // Generate a hash based on the function signature.
    std::size_t hash_sig();

    FunctionType *as_function() override { return this; }

protected:
    friend class TypeContext;

    friend constexpr Box<FunctionType> std::make_unique<FunctionType>();

    FunctionType() : Type(Type::Kind::Function) {}
};

/*
A TypeContext tracks all types that exist within a given translation unit.
It provides methods to retrieve and create types as the AST is walked.

The TypeContext stores types as singleton objects within itself, and hands out
pointers to these objects. Only one copy of any given type object exists
at any given time, and is owned by the TypeContext.

Type objects can be compared by a simple pointer comparison: if the two
pointers are equal, then they are the same type.

Inherently anonymous types (i.e. types that are not differentiated by name (e.g. functions))
are assigned generated names.
*/
class TypeContext {
public:
    TypeContext();

    // Return a pointer to the PrimitiveType object with `kind`.
    PrimitiveType *get_primitive(PrimitiveType::Kind kind);

    /*
    Create or retrieve a struct with the name `name`.

    Returns `nullptr` if a type with `name` is already declared, but is not a struct.
    */
    StructType *get_struct(std::string name);

    // Create an anonymous struct.
    StructType *get_struct();

    /*
    Create or retrieve a union with the name `name`.

    Returns `nullptr` if a type with `name` is already declared, but is not a union.
    */
    UnionType *get_union(std::string name);

    // Create an anonymous union.
    UnionType *get_union();

    /*
    Create or retrieve an enum with the name `name`.

    Returns `nullptr` if a type with `name` is already declared, but is not an enum.
    */
    EnumType *get_enum(std::string name);

    // Create an anonymous enum.
    EnumType *get_enum();

    // Create a pointer with the given `base` type.
    PointerType *get_pointer(Type *base);

    // Create a function type based on its signature.
    FunctionType *get_function(Type *ret, Vec<Type *> params, bool variadic);

private:
    int anonymous_ctr = 0;

    // The map of base types (i.e. non-pointers).
    std::unordered_map<std::string, Box<Type>> base_types;

    // The map of pointer types, mapped by their base type.
    std::unordered_map<Type *, Box<PointerType>> pointers;

    template <typename T>
    requires std::derived_from<T, Type>
    T *make_insert_type(std::string name) {
        Box<T> s = std::make_unique<T>();

        auto ret = s.get();
        base_types.insert({name, std::move(s)});

        return ret;
    }
};

}

#endif