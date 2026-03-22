#ifndef ECC_TYPES_H
#define ECC_TYPES_H

#include <cstddef>
#include <memory>
#include <stack>
#include <string>
#include <sstream>
#include <concepts>
#include <unordered_map>
#include <utility>
#include <variant>

#include "codegen/llvm.hpp"
#include "frontend/tokens.hpp"
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
        VOID,
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

    bool is_void();
    bool is_primitive();
    bool is_pointer();
    bool is_array();
    bool is_function();

    virtual bool is_base() { return false; }

    virtual bool is_user() { return false; } 

    virtual bool is_derived() { return false; }

    virtual std::size_t size() { return 0; }

    /*
    Check if a type can be implicitly coerced into `dst` without a cast,
    i.e. the compiler will insert a cast expression to handle it.

    Note that this relationship is not symmetric; if `this` is compatible
    with `other`, this does not mean `other` is compatible with `this`.

    This function will return false if `this` and `dst` are exactly the same;
    it is to be used for types that are not equal, but might be able to be
    coerced into the other.
    */
    virtual bool is_compatible_with(Type * dst) {
        // by default, types cannot be coerced and require explicit casting.
        return false;
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
public:
    bool is_base() override { return true; }

protected:
    BaseType(Kind kind) : Type(kind) {}
};

/*
An abstract class representing a type that is constructed from BaseTypes,
or is user-defined. These are the unions, structs, and enums.
*/
class UserType : public BaseType {
public:

    bool is_user() override { return true; }

    // Returns the kind of type: class, union, enum.
    static std::string base();

    // The scope where the type was declared.
    sema::sym::Scope *scope;

    Location loc;
protected:
    UserType(Kind kind, sema::sym::Scope *scope) 
        : BaseType(kind), scope(scope) {}
};

/*
An abstract class representing a type constructor whose final type
relies on a base type. For example, a pointer relies on the type that
it points to, in order for its full type to be known.

In EnlightenedC, functions are considered derived types; their base
type being their return type.
*/
class DerivedType : public Type {
public:
    bool is_derived() { return true; }

    Type *base;

protected:
    DerivedType(Kind kind, Type *base) : Type(kind), base(base) {} 
};

class VoidType : public BaseType {
public:
    virtual std::string to_string() const { return "Void"; }
protected:
    friend class TypeContext;

    friend constexpr Box<VoidType> std::make_unique<VoidType>();

    VoidType() : BaseType(Type::Kind::VOID) {}
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

    tokens::PrimType primkind;

    std::size_t size() override;

    bool is_compatible_with(Type *dst) override;

    // Whether this Primitive type can be implicitly represented as an integer.
    // Returns true for all primitive types except F64.
    bool is_integer();

    PrimitiveType *as_primitive() override { return this; }

    std::string to_string() const override;

    std::string formal() override;

protected:
    friend class TypeContext;

    friend constexpr Box<PrimitiveType> std::make_unique<PrimitiveType>(tokens::PrimType&&);

    PrimitiveType(tokens::PrimType kind) : BaseType(Type::Kind::PRIMITIVE), primkind(kind) {}
};

class ClassType : public UserType {
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

    
    std::optional<std::string> name;
    
    Vec<Box<ClassTypeMember>> members;

    std::size_t size() override;

    void add_member(Box<ClassTypeMember> member);

    ClassTypeMember *find(std::string& name);

    ClassTypeMember *index(int idx);

    int num_members() { return members.size(); }

    ClassType *as_class() override { return this; }

    std::string to_string() const override;

    std::optional<std::string> get_name() override { return name; }

    std::string formal() override;

    static std::string base() { return "class_"; }

protected:
    friend class TypeContext;

    friend constexpr Box<ClassType> std::make_unique<ClassType>(sema::sym::Scope *&);
    friend constexpr Box<ClassType> std::make_unique<ClassType>(std::string&, sema::sym::Scope *&);

    // Construct an empty class.
    ClassType(sema::sym::Scope *scope) 
        : UserType(Kind::CLASS, scope), members() {}
    ClassType(std::string name, sema::sym::Scope *scope) 
        : UserType(Kind::CLASS, scope), members(), name(name) {}
};

/*
The union type.

The union type can exist as each of its members, just not at the same time.
Accessing a union's members essentially performs a bitcast to reinterpret the
union's bits as the bits of the member's type.

## Type Representative
Unions can also be represented by a primitive type, where it is essentially that
type in memory. Syntactically and semantically, however, it acts like a union.
Such unions can be declared by specifying a primitive type before the `union`
keyword, e.g. `U64i union U64`.

The above example is used to alias the `U64i` intrinsic type and allow the
programmer to access individual bytes within a U64i value. It is defined as:

```
U64i union U64 {
    U8i  u8[8];
    U16i u16[4];
    U32i u32[2];
    U64i u64[1];
};
```

Then some member expression like `unn->u8` treats the U64i as an array of 8 bytes,
and can be accessed as if it were one.
*/
class UnionType : public UserType {
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

    std::optional<PrimitiveType *> type_rep;

    std::size_t size() override;

    bool is_compatible_with(Type *dst) override;

    void add_member(Box<UnionTypeMember> member);

    UnionTypeMember *find(std::string& name);

    UnionTypeMember *index(int idx);

    int num_members() { return members.size(); }

    UnionType *as_union() override { return this; }

    std::string to_string() const override;

    std::optional<std::string> get_name() override { return name; }

    std::string formal() override;

    static std::string base() { return "union_"; }

protected:
    friend class TypeContext;

    friend constexpr Box<UnionType> 
        std::make_unique<UnionType>(sema::sym::Scope *&);
    friend constexpr Box<UnionType> 
        std::make_unique<UnionType>(PrimitiveType *&, sema::sym::Scope *&);
    friend constexpr Box<UnionType> 
        std::make_unique<UnionType>(std::string&, sema::sym::Scope *&);
    friend constexpr Box<UnionType> 
        std::make_unique<UnionType>(std::string&, PrimitiveType *&, sema::sym::Scope *&);

    UnionType(sema::sym::Scope *scope)
        : UserType(Kind::UNION, scope), members() {}
    UnionType(PrimitiveType *type_rep, sema::sym::Scope *scope)
        : UserType(Kind::UNION, scope), members(), type_rep(type_rep) {}
    UnionType(std::string name, sema::sym::Scope *scope)
        : UserType(Kind::UNION, scope), members(), name(name) {}
    UnionType(std::string name, PrimitiveType *type_rep, sema::sym::Scope *scope)
        : UserType(Kind::UNION, scope), members(), name(name), type_rep(type_rep) {}
};


/*
An enumerated type (e.g. `enum State { DONE, PENDING }`).

## Compatibility

An enum type is compatible with any integer primitive type, but not vice versa.
The compiler will throw an error if the number of enum variants exceed the maximum
value of the integer primitive to be cast to.
*/
class EnumType : public UserType {
public:
    struct EnumTypeMember {
        // the name of the enum variant as declared in the source.
        std::string name;
        // the assigned value of the enum variant.
        int64_t value;
        // the location of the declared enumerator.
        Location loc;
    };

    bool complete = false;

    Vec<Box<EnumTypeMember>> enumerators;

    std::optional<std::string> name;

    std::size_t size() override;

    // Create an enumerator with an automatically chosen value.
    int64_t add_enumerator(std::string enumerator, Location loc);

    // Create an enumerator with a provided value.
    int64_t add_enumerator(std::string enumerator, int64_t value, Location loc);

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

    friend constexpr Box<EnumType> std::make_unique<EnumType>(sema::sym::Scope *&);
    friend constexpr Box<EnumType> std::make_unique<EnumType>(std::string& name, sema::sym::Scope *&);

    EnumType(sema::sym::Scope *scope)
        : UserType(Kind::ENUM, scope), enumerators() {}
    EnumType(std::string name, sema::sym::Scope *scope)
        : UserType(Kind::ENUM, scope), enumerators(), name(name) {}
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

    std::size_t size() override;

    // Returns the level of nesting the pointer has (i.e. how many *'s there are).
    int nesting_lvl();

    // Get the true base type of the pointer.
    // A base type can also be an array, hence the Type * return type.
    Type *true_base();

    PointerType *as_pointer() override { return this; }

    bool is_subscriptable() override { return true; }

    bool is_callable() override;

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

    bool is_subscriptable() override { return true; }

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

class FunctionType : public DerivedType {
public:
    struct FunctionSignature {
        Type *returntype;
        Vec<Type *> params;
        bool variadic;

        // Test if two function signatures are the same.
        bool operator== (FunctionSignature& other) {
            // Test return type
            if (other.returntype != returntype) {
                return false;
            }

            // Test param count
            if (params.size() != other.params.size()) {
                return false;
            }

            // Test the type of each param
            for (int i = 0; i < params.size(); i++) {
                if (params[i] != other.params[i]) {
                    return false;
                }
            }

            // Test variadic flag
            if (variadic != other.variadic) {
                return false;
            }

            return true;
        }
    };

    FunctionSignature signature;

    // Generate a hash based on the function signature.
    std::size_t hash_sig();

    Type *returntype() { return signature.returntype; }

    Vec<Type *>& params() { return signature.params; }

    int num_params() { return signature.params.size(); }

    Type *param_idx(int idx) { return signature.params[idx]; }

    bool no_params() { return signature.params.empty(); }

    bool is_callable() override { return true; }

    FunctionType *as_function() override { return this; }

    std::string to_string() const override;

    static std::string base() { return "function_"; }

protected:
    friend class TypeContext;
    friend class TypeBuilder;

    friend constexpr Box<FunctionType> std::make_unique<FunctionType>(Type *&);

    FunctionType(Type *base) : DerivedType(Type::Kind::FUNCTION, base) {}
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
    TypeContext(codegen::LLVM&);

    TypeContext(const TypeContext&) = delete;
    TypeContext& operator=(const TypeContext&) = delete;

    TypeBuilder builder();

    VoidType *get_void();

    // Return a pointer to the PrimitiveType object with `kind`.
    PrimitiveType *get_primitive(tokens::PrimType kind);

    PrimitiveType *get_u8()   { return u8.get();    }

    PrimitiveType *get_u16()  { return u16.get();   }

    PrimitiveType *get_u32()  { return u32.get();   }

    PrimitiveType *get_u64()  { return u64.get();   }

    PrimitiveType *get_i8()   { return i8.get();    }

    PrimitiveType *get_i16()  { return i16.get();   }

    PrimitiveType *get_i32()  { return i32.get();   }

    PrimitiveType *get_i64()  { return i64.get();   }

    PrimitiveType *get_f64()  { return f64.get();   }

    PrimitiveType *get_bool() { return boolt.get(); }

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

    /*
    Create or retrieve a union with the name `name` and type representative `type_rep`.

    Returns `nullptr` if a type with `name` is already declared, but is not a union.
    */
    UnionType *get_union(std::string name, PrimitiveType *type_rep, sema::sym::Scope *scope);

    // Create an anonymous union.
    UnionType *get_union(sema::sym::Scope *scope);

    // Create an anonymous union with type representative `type_rep`.
    UnionType *get_union(PrimitiveType *type_rep, sema::sym::Scope *scope);

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

    // intern the primitive language-defined types directly in the context.
    Box<VoidType> voidt;
    Box<PrimitiveType> u8;
    Box<PrimitiveType> u16;
    Box<PrimitiveType> u32;
    Box<PrimitiveType> u64;
    Box<PrimitiveType> i8;
    Box<PrimitiveType> i16;
    Box<PrimitiveType> i32;
    Box<PrimitiveType> i64;
    Box<PrimitiveType> f64;
    Box<PrimitiveType> boolt;

    codegen::LLVM& llvm;

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

    // The map of record types (i.e. structs, unions, enums).
    std::unordered_map<std::string, Box<UserType>> user_types;

    // The map of function types.
    std::unordered_map<std::string, Box<FunctionType>> function_types;

    // The map of pointer types, mapped by their base type.
    std::unordered_map<PointerKey, Box<PointerType>, pair_hash<bool>> pointers;

    // The map of array types, mapped by their base type (todo: add size)
    std::unordered_map<ArrayKey, Box<ArrayType>, pair_hash<std::optional<uint64_t>>> arrays;

    // Generate a mangled, unique name for a type incorporating its associated scope.
    template <typename T>
    requires std::derived_from<T, UserType>
    std::string mangle(std::string name, sema::sym::Scope *sc) {
        std::stringstream ss;

        ss << T::base() << name << "_" << static_cast<const void *>(sc); // fixme: use scope id

        return ss.str();
    }


    template <typename Ty>
    requires std::derived_from<Ty, UserType>
    // Create and insert a named type.
    Ty *make_insert_type(std::string mangled, sema::sym::Scope *scope, Box<Ty> type) {

        auto ret = type.get();
        if (type->get_name()) {
            for (auto const& [key, etype] : user_types) {
                    // if type has name and we find another type with the same name
                if (*type->get_name() == etype->get_name()) {
                    // if the scopes match
                    if (scope == etype->scope) {
                        throw etype.get();
                    }
                }
            }
        }
        user_types.insert({mangled, std::move(type)});

        return ret;
    }
};

}

#endif