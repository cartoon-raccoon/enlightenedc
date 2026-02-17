#ifndef ECC_TYPES_H
#define ECC_TYPES_H

#include "util.hpp"
#include <cstddef>
#include <string>
#include <unordered_map>

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

/*
The base Type class.

EnlightenedC uses an interning type system that stores singletons of each type encountered
in compilation of the translation unit. Derived types hold pointers to their base types,
making checking type equality as easy as a pointer comparison.
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
    // The size of the type in bytes.
    int size;

    Type(Kind kind) : kind(kind) {}

    bool is_primitive();
    bool is_pointer();

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
    
    // virtual bool operator==(const Type& rhs) {
    //     return false; // FIXME
    // }
};

/*
A class representing a primitive type (e.g. U8, U16, F64, etc.)

Primitive types do not hold pointers to other Types, i.e. all
derived types should resolve to a primitive type.
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
    };

    PrimitiveType(Kind kind) : Type(Type::Kind::Primitive), primkind(kind) {}

    Kind primkind;

    virtual PrimitiveType *as_primitive() override { return this; }
};

class StructType : public Type {
public:
    struct StructTypeMember {
        std::string name;
        Type *ty;
        int offset;
        
        StructTypeMember(std::string name, Type *ty) : name(name), ty(ty) {
            // ignore offset, have it populated when added to a StructType.
        }
    };

    // Construct an empty struct.
    StructType() : Type(Type::Kind::Struct), members() {}

    /*
    Whether the struct definition is complete.
    */
    bool complete = false;

    Vec<Box<StructTypeMember>> members;

    void add_member(Box<StructTypeMember> member);

    // Marks a struct as fully defined, and calculates offsets.
    void finalize();

    virtual StructType *as_struct() override { return this; }
};


class UnionType : public Type {
public:
    struct UnionTypeMember {
        std::string name;
        Type *ty;
    };

    UnionType() : Type(Type::Kind::Union) {}

    bool complete = false;

    Vec<Box<UnionTypeMember>> members;

    void add_member(Box<UnionTypeMember> member);

    // Marks a union as fully defined, and calculates offsets.
    void finalize();

    virtual UnionType *as_union() override { return this; }
};

class EnumType : public Type {
public:
    EnumType() : Type(Type::Kind::Enum), enumerators() {}

    bool complete = false;

    Vec<std::string> enumerators;

    void add_enumerator(std::string enumerator);

    // Marks an enum as fully defined.
    void finalize();

    virtual EnumType *as_enum() override { return this; }
};


class PointerType : public Type {
public:
    PointerType() : Type(Type::Kind::Pointer) {}

    Type *base;

    virtual PointerType *as_pointer() override { return this; }
};


class FunctionType : public Type {
public:
    struct FunctionSignature {
        Type *returntype;
        Vec<Type *> params;
    } signature;

    FunctionType() : Type(Type::Kind::Function) {}

    // Generate a hash based on the function signature.
    std::size_t hash_sig();

    virtual FunctionType *as_function() override { return this; }
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

    // Create or retrieve a struct with the name `name`.
    StructType *get_struct(std::string name);

    // Create an anonymous struct.
    StructType *get_struct();

    // Create or retrieve a union with the name `name`.
    UnionType *get_union(std::string name);

    // Create an anonymous union.
    UnionType *get_union();

    // Create a pointer with the given `base` type.
    // Returns null if no type corresponding to `base` exists.
    PointerType *get_pointer(Type *base);

    // Create a function type based on its signature.
    FunctionType *get_function(Type *ret, Vec<Type *> params);

    // Retrieve the name of a given type.
    std::string find_name(Type *type);
private:
    int anonymous_ctr = 0;
    std::unordered_map<std::string, Box<Type>> types;
};

}

#endif