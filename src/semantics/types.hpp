#ifndef ECC_TYPES_H
#define ECC_TYPES_H

#include <cstddef>
#include <stack>
#include <string>
#include <sstream>
#include <concepts>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>

#include "util.hpp"

/*
Forward declaration of Scope from symbols
*/
namespace ecc::sema::sym {
    class Scope;
}

namespace ecc::sema::types {
/*
The type system implementation for EnlightenedC.

Ecc uses an interning type system, where a TypeContext manages internal type
objects, and hands out pointers to them. This makes checking for type equality
equivalent to a single pointer comparison (with the exception of arrays; see below).
*/

using namespace util;
using namespace ecc;

class PrimitiveType;
class ClassType;
class UnionType;
class EnumType;
class PointerType;
class ArrayType;
class FunctionType;
class TypeContext;

/*
The base Type class.

EnlightenedC uses an interning type system that stores singletons of each type encountered
in compilation of the translation unit. Derived types hold pointers to their base types,
making checking type equality as easy as a pointer comparison.

In order to enforce the type equality = pointer equality property, Types cannot be created externally.
They are instead created and managed by a `TypeContext`, and are retrieved using its `get_*` methods.

Type size and member alignment is calculated at compile time, using LLVM's `DataLayout` class.
*/
class Type {
public:
    enum Kind {
        PRIMITIVE,
        CLASS,
        UNION,
        ENUM,
        POINTER,
        ARRAY,
        FUNCTION,
    };

    // The kind of the type.
    Kind kind;

    // The location where the type was defined.
    Location loc;

    bool is_primitive();
    bool is_pointer();

    virtual int size() { return 0; };

    /*
    Check if a type can be implicitly coerced into `dst` without a cast,
    i.e. the compiler will insert a cast expression to handle it.

    Note that this relationship is not symmetric; if `this` is compatible
    with `other`, this does not mean `other` is compatible with `this`.
    */
    virtual bool is_compatible_with(Type * dst) {
        // at the minimum, enforce strict equality (no subtyping).
        return dst == this;
    }

    // Cast this type to a PrimitiveType *.
    // Returns null if the underlying type is not a PrimitiveType.
    virtual PrimitiveType *as_primitive() { return nullptr; }

    // Cast this type to a ClassType *.
    // Returns null if the underlying type is not a ClassType.
    virtual ClassType *as_class() { return nullptr; }

    // Cast this type to a UnionType *.
    // Returns null if the underlying type is not a ClassType.
    virtual UnionType *as_union() { return nullptr; }

    // Cast this type to an EnumType *.
    // Returns null if the underlying type is not a ClassType.
    virtual EnumType *as_enum() { return nullptr; }

    // Cast this type to a PointerType *.
    // Returns null if the underlying type is not a ClassType.
    virtual PointerType *as_pointer() { return nullptr; }

    // Cast this type to an ArrayType *.
    // Returns null if the underlying type is not a ClassType.
    virtual ArrayType *as_array() { return nullptr; }

    // Cast this type to a FunctionType *.
    // Returns null if the underlying type is not a ClassType.
    virtual FunctionType *as_function() { return nullptr; }

    // Whether the type is callable.
    // Only functions should be callable.
    virtual bool is_callable() { return false; };

    // Whether the type is subscriptable/indexable.
    // Only arrays and pointers should be subscriptable.
    virtual bool is_subscriptable() { return false; };

    virtual bool has_members() { return false; }

    virtual std::string to_string() const { return "()"; }

    virtual std::optional<std::string> get_name() { return {}; };

    // Returns the formal name of the type.
    virtual std::string formal() { return "()"; }

protected:
    Type(Kind kind) : kind(kind) {}
    
    friend class TypeContext;
};

/*
An abstract class representing a type that does not depend on a base type
(i.e. it is not a type constructor). These include the primitive types,
classes, unions, and enums.
*/
class BaseType : public Type {
protected:
    BaseType(Kind kind) : Type(kind) {}
};

/*
An abstract class representing a type that is constructed from BaseTypes,
or is user-defined. These are the unions, structs, and enums.
*/
class RecordType : public BaseType {
public:
    // Returns the kind of type: class, union, enum.
    static std::string base();
protected:
    RecordType(Kind kind) : BaseType(kind) {}
};

class DerivedType : public Type {
public:
    Type *base;

protected:
    DerivedType(Kind kind, Type *base) : Type(kind), base(base) {} 
};

/*
A primitive type (e.g. U8, U16, F64, etc.)

Primitive types do not hold pointers to other Types, i.e. all
derived types should resolve to a primitive type.

## Compatibility

Any non-zero-sized PrimitiveType a can implicitly resolve to another
PrimitiveType b if `b.size() >= a.size()`, regardless of signedness.

Floats are not compatible with any other primitive type.

Zero-sized types (U0, I0) cannot be implicitly converted to other types.
*/
class PrimitiveType : public BaseType {
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

    int size() override;

    bool is_compatible_with(Type *dst) override;

    // Whether this Primitive type can be implicitly represented as an integer.
    // Returns true for all primitive types except F64.
    bool is_integer();

    PrimitiveType *as_primitive() override { return this; }

    std::string to_string() const override;

    std::string formal() override;

protected:
    friend class TypeContext;

    friend constexpr Box<PrimitiveType> std::make_unique<PrimitiveType>(Kind&&);

    PrimitiveType(Kind kind) : BaseType(Type::Kind::PRIMITIVE), primkind(kind) {}
};

class ClassType : public RecordType {
public:
    struct ClassTypeMember {
        std::optional<std::string> name;
        Type *ty;
        Location loc;
        
        ClassTypeMember(Type *ty, Location loc)
            : ty(ty), loc(loc) {}
        ClassTypeMember(std::string name, Type *ty, Location loc)
            : name(name), ty(ty), loc(loc) {}
        ClassTypeMember(std::optional<std::string> name, Type *ty, Location loc)
            : name(name), ty(ty), loc(loc) {}
    };

    /*
    Whether the class definition is complete.

    This is used in forward declarations of classes that may not have been fully defined.
    Once a class has been defined (i.e. members have been set), this is marked true,
    preventing the double-definition problem.
    */
    bool complete = false;

    int size() override;

    std::optional<std::string> name;

    Vec<Box<ClassTypeMember>> members;

    void add_member(Box<ClassTypeMember> member);

    ClassTypeMember *find(std::string& name);

    ClassType *as_class() override { return this; }

    bool has_members() override { return true; }

    std::string to_string() const override;

    std::optional<std::string> get_name() override { return name; }

    std::string formal() override;

    static std::string base() { return "class_"; }

protected:
    friend class TypeContext;

    friend constexpr Box<ClassType> std::make_unique<ClassType>();
    friend constexpr Box<ClassType> std::make_unique<ClassType>(std::string& name);

    // Construct an empty class.
    ClassType() : RecordType(Kind::CLASS), members() {}
    ClassType(std::string name) : RecordType(Kind::CLASS), members(), name(name) {}
};


class UnionType : public RecordType {
public:
    struct UnionTypeMember {
        std::optional<std::string> name;
        Type *ty;
        Location loc;

        UnionTypeMember(Type *ty, Location loc)
            : ty(ty), loc(loc) {}
        UnionTypeMember(std::string name, Type *ty, Location loc)
            : name(name), ty(ty), loc(loc) {}
        UnionTypeMember(std::optional<std::string> name, Type *ty, Location loc)
            : name(name), ty(ty), loc(loc) {}
    };

    bool complete = false;

    Vec<Box<UnionTypeMember>> members;

    std::optional<std::string> name;

    void add_member(Box<UnionTypeMember> member);

    UnionTypeMember *find(std::string& name);

    UnionType *as_union() override { return this; }

    bool has_members() override { return true; }

    std::string to_string() const override;

    std::optional<std::string> get_name() override { return name; }

    std::string formal() override;

    static std::string base() { return "union_"; }

protected:
    friend class TypeContext;

    friend constexpr Box<UnionType> std::make_unique<UnionType>();
    friend constexpr Box<UnionType> std::make_unique<UnionType>(std::string& name);

    UnionType() : RecordType(Kind::UNION), members() {}
    UnionType(std::string name) : RecordType(Kind::UNION), members(), name(name) {}
};


/*
An enumerated type (e.g. `enum State { DONE, PENDING }`).

## Compatibility

An enum type is compatible with any integer primitive type, but not vice versa.
The compiler will throw an error if the number of enum variants exceed the maximum
value of the integer primitive to be cast to.
*/
class EnumType : public RecordType {
public:
    struct EnumTypeMember {
        // the name of the enum variant as declared in the source.
        std::string name;
        // the assigned value of the enum variant.
        uint64_t value;
        // the location of the declared enumerator.
        Location loc;
    };

    bool complete = false;

    Vec<Box<EnumTypeMember>> enumerators;

    std::optional<std::string> name;

    // Create an enumerator with an automatically chosen value.
    void add_enumerator(std::string enumerator, Location loc);

    // Create an enumerator with a provided value.
    void add_enumerator(std::string enumerator, uint64_t value, Location loc);

    // Check if an enum already contains an enumerator. 
    EnumTypeMember *find(std::string& name);

    bool is_compatible_with(Type *dst) override;

    EnumType *as_enum() override { return this; }

    std::string to_string() const override;

    std::optional<std::string> get_name() override { return name; }

    std::string formal() override;

    static std::string base() { return "enum_"; }

protected:
    friend class TypeContext;

    friend constexpr Box<EnumType> std::make_unique<EnumType>();
    friend constexpr Box<EnumType> std::make_unique<EnumType>(std::string& name);

    EnumType() : RecordType(Kind::ENUM), enumerators() {}
    EnumType(std::string name) : RecordType(Kind::ENUM), enumerators(), name(name) {}

private:
    // existing values that have already been declared.
    std::unordered_set<uint64_t> values;
};


/*
A pointer type (U8 *, I32 **, etc.)

## Compatibility

A pointer `ptr` is only compatible with another pointer `other` if:

the base type of `other` is a void type (U0 or U0), and
the level of nesting is the same (i.e. U8 ** is compatible with U0 **, but not U0 ***).

Pointers can be subscripted like arrays, the compiler will treat the subscript as pointer
arithmetic. However, pointers cannot be converted to sized arrays.
*/
class PointerType : public DerivedType {
public:
    bool is_const;

    // Returns the level of nesting the pointer has (i.e. how many *'s there are).
    int nesting_lvl();

    PointerType *as_pointer() override { return this; }

    bool is_compatible_with(Type *other) override;

    std::string to_string() const override;

protected:
    friend class TypeContext;
    friend class TypeBuilder;

    friend constexpr Box<PointerType> std::make_unique<PointerType>(Type *&);
    
    PointerType(Type *base) : DerivedType(Kind::POINTER, base) {}
};


/*
A sized array type (U8 [4], U32 [6], etc.).

## Compatibility

An array is compatible with a pointer of the same base through pointer decay.
Similarly, pointers can be subscripted like an array, the compiler treats
it as pointer arithmetic.

A sized array is strictly only compatible with another array of the exact same base
and size.

## Semantics

EnlightenedC follows C's array semantics: Declared arrays must be sized,
and the compiler will infer the size of an array if an initializer is provided.
If an array is declared with no size and no initializer is provided, the compiler
will throw an error.

An unsized array declarator can be used as a function argument, where it will be
decayed to a pointer.
*/
class ArrayType : public DerivedType {
public:
    // The size of the array, populated after elaboration.
    std::optional<uint64_t> size;

    ArrayType *as_array() override { return this; }

    bool is_compatible_with(Type *other) override;

    std::string to_string() const override;
    
protected:
    friend class TypeContext;

    friend constexpr Box<ArrayType> std::make_unique<ArrayType>(Type *&, uint64_t&);

    friend constexpr Box<ArrayType> std::make_unique<ArrayType>(Type *&);

    ArrayType(Type *base, uint64_t size) : DerivedType(Kind::ARRAY, base), size(size) {}

    ArrayType(Type *base) : DerivedType(Kind::ARRAY, base) {}
};

// A function parameter containing an optional name.
struct FuncParam {
    Type *type;
    std::optional<std::string> name;
    Location loc;
    bool is_const;
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

    std::string to_string() const override;

protected:
    friend class TypeContext;
    friend class TypeBuilder;

    friend constexpr Box<FunctionType> std::make_unique<FunctionType>();

    FunctionType() : Type(Type::Kind::FUNCTION) {}
};

/*
A constructor type used for building up a type whose base type is not yet resolved.

The standard order of type construction is to derive the base type first,
then add constructors onto it such as arrays and pointers. However, this does not
always work, as we might have the constructors before we know the base type.

The TypeBuilder allows us to reverse the order of type construction; it applies
constructors to an opaque type first, then once the base type is set, it finalizes
the type and returns a pointer to the created concrete type.
*/
class TypeBuilder {
public:

    // Add an array to the type.
    void add_array(uint64_t size);

    // Add an unsized array to the type.
    void add_array();

    void add_pointer(bool is_const);

    void add_function(Vec<FuncParam> params, bool variadic);

    void set_base(BaseType *type);

    Type *finalize();
    struct Ptr {
        bool is_const;
    };
    struct Arr {
        std::optional<uint64_t> size;
    };
    struct FnParams {
        Vec<FuncParam> params;
        bool variadic;
    };

    BaseType *base;

    TypeContext& ctxt;

    std::stack<std::variant<Ptr, Arr, FnParams>> type_stack;

protected:

    friend class TypeContext;
    friend constexpr Box<TypeBuilder> std::make_unique<TypeBuilder>(TypeContext&&);

    TypeBuilder(TypeContext& ctxt) : ctxt(ctxt), base(nullptr) {}
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

The TypeContext stores keys to types as mangled names, incorporating the kind of type
(class, union, enum), name of the type (if any), and the scope it is declared in.
*/
class TypeContext {
public:
    TypeContext();

    TypeBuilder builder();

    // Return a pointer to the PrimitiveType object with `kind`.
    PrimitiveType *get_primitive(PrimitiveType::Kind kind);

    /*
    Create or retrieve a class with the name `name`.

    Returns `nullptr` if a type with `name` is already declared, but is not a class.
    */
    ClassType *get_class(std::string name, sema::sym::Scope *scope);

    // Create an anonymous class.
    ClassType *get_class(sema::sym::Scope *scope);

    /*
    Create or retrieve a union with the name `name`.

    Returns `nullptr` if a type with `name` is already declared, but is not a union.
    */
    UnionType *get_union(std::string name, sema::sym::Scope *scope);

    // Create an anonymous union.
    UnionType *get_union(sema::sym::Scope *scope);

    /*
    Create or retrieve an enum with the name `name`.

    Returns `nullptr` if a type with `name` is already declared, but is not an enum.
    */
    EnumType *get_enum(std::string name, sema::sym::Scope *scope);

    // Create an anonymous enum.
    EnumType *get_enum(sema::sym::Scope *scope);

    // Create a pointer with the given `base` type.
    PointerType *get_pointer(Type *base, bool is_const);

    // Create or get an array with the given `base` type and specified size.
    ArrayType *get_array(Type *base, uint64_t size);

    // Create or get an array with the given `base` type and no specified size.
    ArrayType *get_array(Type *base);

    // Create a function type based on its signature.
    FunctionType *get_function(Type *ret, Vec<Type *> params, bool variadic);

    std::string to_string() const;

private:
    int anonymous_ctr = 0;

    template<typename T>
    struct pair_hash {
        std::size_t operator() (const std::pair<Type *, T> p) const {
            auto h1 = std::hash<Type *>{}(p.first);
            auto h2 = std::hash<T>{}(p.second);

            return h1 ^ h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2);
        }
    };

    using ArrayKey = std::pair<Type *, std::optional<uint64_t>>;
    using PointerKey = std::pair<Type *, bool>;

    // The map of base types (i.e. non-pointers).
    std::unordered_map<std::string, Box<Type>> base_types;

    // The map of pointer types, mapped by their base type.
    std::unordered_map<PointerKey, Box<PointerType>, pair_hash<bool>> pointers;

    // The map of array types, mapped by their base type (todo: add size)
    std::unordered_map<ArrayKey, Box<ArrayType>, pair_hash<std::optional<uint64_t>>> arrays;

    // Generate a mangled, unique name for a type incorporating its associated scope.
    template <typename T>
    requires std::derived_from<T, Type>
    std::string mangle(std::string name, sema::sym::Scope *sc) {
        std::stringstream ss;

        ss << T::base() << name << "_" << static_cast<const void *>(sc);

        return ss.str();
    }


    template <typename T>
    requires std::derived_from<T, Type>
    // Create and insert a named type.
    T *make_insert_type(std::string mangled, std::string name) {
        Box<T> s = std::make_unique<T>(name);

        auto ret = s.get();
        for (auto const& [key, type] : base_types) {
            // we find a type with the same name
            if (name == type->get_name()) {
                throw type.get();
            }
        }
        base_types.insert({mangled, std::move(s)});

        return ret;
    }

    template <typename T>
    requires std::derived_from<T, Type>
    T *make_insert_type(std::string mangled) {
        Box<T> s = std::make_unique<T>();

        auto ret = s.get();
        // no collision check required for anonymous types.
        base_types.insert({mangled, std::move(s)});

        return ret;
    }
};

}

#endif