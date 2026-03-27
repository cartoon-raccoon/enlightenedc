#ifndef ECC_TYPES_H
#define ECC_TYPES_H

#include <cassert>
#include <cstddef>
#include <memory>
#include <stack>
#include <string>
#include <sstream>
#include <concepts>
#include <type_traits>
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

// Forward declaration of ExprMIR from MIR
namespace ecc::sema::mir {
    class ExprMIR;
}

/**
The type system implementation for EnlightenedC.

Ecc uses an interning type system, where a TypeContext manages internal type
objects, and hands out pointers to them. This makes checking for type equality
equivalent to a single pointer comparison (with the exception of arrays; see below).
*/
namespace ecc::sema::types {

using namespace util;
using namespace ecc;

class Type;
class PrimitiveType;
class ClassType;
class UnionType;
class EnumType;
class PointerType;
class ArrayType;
class FunctionType;
class TypeContext;

/**
A handle to a Type object, ensuring that the internal Type * can never be null.
*/
template <typename Ty>
requires std::derived_from<Ty, Type>
class TypeHandle {
    static_assert(std::is_base_of_v<Type, Ty>, "Ty must be a Type" );

    Ty *ptr;
    TypeContext& tyctxt;
    
    TypeHandle(Ty *p, TypeContext& tyctxt) : ptr(p), tyctxt(tyctxt) {
        assert(p != nullptr && "tried to create TypeHandle from null Type pointer");
    }
    friend class TypeContext;
public:
    static Optional<TypeHandle<Ty>> create(Ty *ptr);

    /**
    Express the TypeHandle as a pointer to a different type.

    If the internal TypeHandle cannot be dynamically cast to the requested type,
    an empty optional is returned.
    */
    template <typename Dst>
    requires std::derived_from<Ty, Type>
    Optional<TypeHandle<Dst>> as() noexcept {
        Dst *targ = dynamic_cast<Dst *>(ptr);
        if (!targ) {
            return {};
        }

        return TypeHandle(targ, tyctxt);
    }

    template <typename Dst>
    TypeHandle(const TypeHandle<Dst>& dst) : ptr(dst.ptr) {}

    template<typename Other>
    bool operator==(const TypeHandle<Other>& other) {
        return ptr == other.ptr;
    }

    Ty *operator->() const { return ptr; }
    Ty& operator*() const { return *ptr; }
};

/**
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

    /**
    Get the size of the type as reported by LLVM.

    Before `finalize()` is called, calling this will throw an error.
    */
    virtual size_t size();

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

    // Whether the type is a scalar type
    // e.g. U8, F64, pointers, bools, enums.
    virtual bool is_scalar() { return false; }

    // Whether the type is strictly an integer type.
    virtual bool is_integral() { return false; }

    /*
    Finalize the type with LLVM, creating the equivalent LLVM type.
    Before this function has been called on a type, calling `size()` will
    produce undefined results.
    */
    virtual void finalize() = 0;

    codegen::LLVMType *get_llvmtype() { return llvm_type; }

    virtual std::string to_string() const { return "()"; }

    virtual Optional<std::string> get_name() { return {}; };

    // Returns the formal name of the type.
    virtual std::string formal() { return "()"; }

    Type& operator=(const Type other) = delete;

protected:
    Type(Kind kind, TypeContext& tyctxt) : kind(kind), tyctxt(tyctxt) {}
    virtual ~Type() = default;
    codegen::LLVMType *llvm_type = nullptr;

    friend class TypeContext;
    TypeContext& tyctxt;

    // Whether the Type has been finalized.
    bool finalized = false;
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
    BaseType(Kind kind, TypeContext& tyctxt) : Type(kind, tyctxt) {}
};

/*
An abstract class representing a type that is constructed from BaseTypes,
or is user-defined. These are the unions, classes, and enums.
*/
class UserType : public BaseType {
    /**
    Whether the type definition is complete.

    This is used in forward declarations of user types that may not have been fully defined.
    Once a class has been defined (i.e. members have been set), this is marked true,
    preventing the double-definition problem.
    */
    bool complete = false;

public:

    bool is_complete() const { return complete; }

    bool is_user() override { return true; }

    // Returns the kind of type: class, union, enum.
    static std::string base();

    virtual bool is_fully_defined() { return false; };

    /**
    Set the location where the type was defined and mark it as complete.
    */
    void finish(Location loc) { def_loc = loc; complete = true; }

    // The scope where the type was declared.
    sema::sym::Scope *scope;

    /** The location that the type was declared. */
    Location decl_loc;
    /** The location that the type was defined. */
    Location def_loc;
protected:
    UserType(Location decl_loc, Kind kind, TypeContext& tyctxt, sema::sym::Scope *scope) 
        : BaseType(kind, tyctxt), scope(scope), decl_loc(decl_loc) {}
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
    DerivedType(Kind kind, TypeContext& tyctxt, Type *base) : Type(kind, tyctxt), base(base) {} 
};

/**
The Void Type in EnlightenedC.
*/
class VoidType : public BaseType {
public:
    void finalize() override;

    virtual std::string to_string() const override { return "Void"; }
protected:
    friend class TypeContext;

    friend constexpr Box<VoidType> std::make_unique<VoidType>(TypeContext&);

    VoidType(TypeContext& tyctxt) : BaseType(Type::Kind::VOID, tyctxt) {}
};

/**
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

    bool is_compatible_with(Type *dst) override;

    // Whether this Primitive type can be represented as an integer.
    // Returns true for all primitive types except F64 and Bool.
    bool is_integer();

    bool is_float();

    bool is_bool();

    bool is_signed();

    PrimitiveType *as_primitive() override { return this; }

    bool is_scalar() override { return true; }

    bool is_integral() override { return is_integer(); }

    void finalize() override;

    std::string to_string() const override;

    std::string formal() override;

protected:
    friend class TypeContext;

    friend constexpr Box<PrimitiveType> 
        std::make_unique<PrimitiveType>(tokens::PrimType&&, TypeContext&);

    PrimitiveType(tokens::PrimType kind, TypeContext& tyctxt)
        : BaseType(Type::Kind::PRIMITIVE, tyctxt), primkind(kind) {}
};

/*
The ClassType in EnlightenedC.
*/
class ClassType : public UserType {
public:
    struct ClassTypeMember {
        Optional<std::string> name;
        Type *ty;
        Location loc;
        
        ClassTypeMember(Type *ty, Location loc)
            : ty(ty), loc(loc) {}
        ClassTypeMember(std::string name, Type *ty, Location loc)
            : name(name), ty(ty), loc(loc) {}
        ClassTypeMember(Optional<std::string> name, Type *ty, Location loc)
            : name(name), ty(ty), loc(loc) {}
    };

    /**
    The name of the class.

    If the name is empty, the class is anonymous.
    */
    Optional<std::string> name;

    /**
    The members of the class.
    */
    Vec<Box<ClassTypeMember>> members;

    /*
    Whether the class is fully defined,

    Unlike `complete`, which only marks whether this class has had all its members added,
    this recursively checks whether any of the class members, if a UserType, is fully defined.
    */
    bool is_fully_defined() override;

    void add_member(Box<ClassTypeMember> member);

    ClassTypeMember *find(std::string& name);

    ClassTypeMember *index(int idx);

    int num_members() { return members.size(); }

    ClassType *as_class() override { return this; }

    void finalize() override;

    std::string to_string() const override;

    Optional<std::string> get_name() override { return name; }

    std::string formal() override;

    static std::string base() { return "class_"; }

protected:
    friend class TypeContext;

    friend constexpr Box<ClassType>
        std::make_unique<ClassType>(
            Location&, sema::sym::Scope *&, TypeContext&);
    friend constexpr Box<ClassType>
        std::make_unique<ClassType>(
            Location&, std::string&, sema::sym::Scope *&, TypeContext&);

    /** Construct an anonymous empty class. */
    ClassType(Location decl_loc, sema::sym::Scope *scope, TypeContext& tyctxt)
        : UserType(decl_loc, Kind::CLASS, tyctxt, scope), members() {}
    /** Construct an empty class with a given name. */
    ClassType(Location decl_loc, std::string name, sema::sym::Scope *scope, TypeContext& tyctxt)
        : UserType(decl_loc, Kind::CLASS, tyctxt, scope), members(), name(name) {}

private:
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
        Optional<std::string> name;
        Type *ty;
        Location loc;

        UnionTypeMember(Type *ty, Location loc)
            : ty(ty), loc(loc) {}
        UnionTypeMember(std::string name, Type *ty, Location loc)
            : name(name), ty(ty), loc(loc) {}
        UnionTypeMember(Optional<std::string> name, Type *ty, Location loc)
            : name(name), ty(ty), loc(loc) {}
    };

    Vec<Box<UnionTypeMember>> members;

    Optional<std::string> name;

    Optional<PrimitiveType *> type_rep = {};

    size_t size() override;

    bool is_fully_defined() override;

    bool is_compatible_with(Type *dst) override;

    void add_member(Box<UnionTypeMember> member);

    UnionTypeMember *find(std::string& name);

    UnionTypeMember *index(int idx);

    int num_members() { return members.size(); }

    UnionType *as_union() override { return this; }

    void finalize() override;

    std::string to_string() const override;

    Optional<std::string> get_name() override { return name; }

    std::string formal() override;

    static std::string base() { return "union_"; }

protected:
    friend class TypeContext;

    friend constexpr Box<UnionType> 
        std::make_unique<UnionType>(
            Location&, sema::sym::Scope *&, TypeContext&);
    friend constexpr Box<UnionType> 
        std::make_unique<UnionType>(
            Location&, std::string&, sema::sym::Scope *&, TypeContext&);

    UnionType(Location decl_loc, sema::sym::Scope *scope, TypeContext& tyctxt)
        : UserType(decl_loc, Kind::UNION, tyctxt, scope), members() {}
    UnionType(Location decl_loc, std::string name, sema::sym::Scope *scope, TypeContext& tyctxt)
        : UserType(decl_loc, Kind::UNION, tyctxt, scope), members(), name(name) {}
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

    Optional<std::string> name;

    Vec<Box<EnumTypeMember>> enumerators;

    PrimitiveType *underlying;

    bool is_fully_defined() override { return is_complete(); }

    /** Create an enumerator with an automatically chosen value. */
    int64_t add_enumerator(std::string enumerator, Location loc);

    /** Create an enumerator with a provided value. */
    int64_t add_enumerator(std::string enumerator, int64_t value, Location loc);

    // Check if an enum already contains an enumerator. 
    EnumTypeMember *find(std::string& name);

    // Find enumerator at the specified index.
    EnumTypeMember *find(size_t i);

    bool is_compatible_with(Type *dst) override;

    EnumType *as_enum() override { return this; }

    void finalize() override;

    std::string to_string() const override;

    Optional<std::string> get_name() override { return name; }

    std::string formal() override;

    static std::string base() { return "enum_"; }

protected:
    friend class TypeContext;

    friend constexpr Box<EnumType>
        std::make_unique<EnumType>( Location&, sema::sym::Scope *&, TypeContext&);
    friend constexpr Box<EnumType>
        std::make_unique<EnumType>(Location&, std::string& name, sema::sym::Scope *&, TypeContext&);
    
    EnumType(
        Location decl_loc, 
        sema::sym::Scope *scope, 
        TypeContext& tyctxt);

    EnumType(Location decl_loc, std::string name, sema::sym::Scope *scope, TypeContext& tyctxt);
};


/**
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

    // Get the true base type of the pointer.
    // A base type can also be an array, hence the Type * return type.
    Type *true_base();

    PointerType *as_pointer() override { return this; }

    bool is_subscriptable() override { return true; }

    bool is_callable() override;

    bool is_compatible_with(Type *other) override;

    bool is_scalar() override { return true; };

    bool is_integral() override { return false; };

    void finalize() override;

    std::string to_string() const override;

protected:
    friend class TypeContext;
    friend class TypeBuilder;

    friend constexpr Box<PointerType> std::make_unique<PointerType>(Type *&, TypeContext&);
    
    PointerType(Type *base, TypeContext& tyctxt) : DerivedType(Kind::POINTER, tyctxt, base) {}
};


/**
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
    // The number of elements in the array, populated after elaboration.
    Optional<uint64_t> arr_size;

    ArrayType *as_array() override { return this; }

    bool is_subscriptable() override { return true; }

    bool is_compatible_with(Type *other) override;

    void finalize() override;

    std::string to_string() const override;
    
protected:
    friend class TypeContext;

    friend constexpr Box<ArrayType>
        std::make_unique<ArrayType>(Type *&, uint64_t&, TypeContext&);

    friend constexpr Box<ArrayType>
        std::make_unique<ArrayType>(Type *&, TypeContext&);

    ArrayType(Type *base, uint64_t size, TypeContext& tyctxt)
        : DerivedType(Kind::ARRAY, tyctxt, base), arr_size(size) {}

    ArrayType(Type *base, TypeContext& tyctxt)
        : DerivedType(Kind::ARRAY, tyctxt, base) {}
};

// A function parameter containing an optional name.
struct FuncParam {
    Type *type;
    Optional<std::string> name;
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

    size_t size() override;

    // Generate a hash based on the function signature.
    std::size_t hash_sig();

    Type *returntype() { return signature.returntype; }

    Vec<Type *>& params() { return signature.params; }

    int num_params() { return signature.params.size(); }

    Type *param_idx(int idx) { return signature.params[idx]; }

    bool no_params() { return signature.params.empty(); }

    bool is_callable() override { return true; }

    FunctionType *as_function() override { return this; }

    void finalize() override;

    std::string to_string() const override;

    static std::string base() { return "function_"; }

protected:
    friend class TypeContext;
    friend class TypeBuilder;

    friend constexpr Box<FunctionType>
        std::make_unique<FunctionType>(Type *&, TypeContext&);

    FunctionType(Type *base, TypeContext& tyctxt)
        : DerivedType(Type::Kind::FUNCTION, tyctxt, base) {}
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

    /** Add an unsized array to the type. */
    void add_array();

    void add_pointer(bool is_const);

    void add_function(Vec<FuncParam> params, bool variadic);

    void set_base(BaseType *type);

    Type *finalize();
    struct Ptr {
        bool is_const;
    };
    struct Arr {
        Optional<uint64_t> size;
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
    friend constexpr Box<TypeBuilder> std::make_unique<TypeBuilder>(TypeContext&);

    TypeBuilder(TypeContext& ctxt) : ctxt(ctxt), base(nullptr) {}
};

/**
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
    friend class Type;
    friend class VoidType;
    friend class PrimitiveType;
    friend class ClassType;
    friend class UnionType;
    friend class EnumType;
    friend class PointerType;
    friend class ArrayType;
    friend class FunctionType;

    TypeContext(codegen::LLVMUnit&);

    TypeContext(const TypeContext&) = delete;
    TypeContext& operator=(const TypeContext&) = delete;

    TypeBuilder builder();

    /** Return a pointer to the Void Type object. */
    VoidType *get_void();

    /** Return a pointer to the PrimitiveType object with `kind`. */
    PrimitiveType *get_primitive(tokens::PrimType kind);

    /** Return a pointer to the U8 Type object. */
    PrimitiveType *get_u8()   { return u8.get();    }

    /** Return a pointer to the U16 Type object. */
    PrimitiveType *get_u16()  { return u16.get();   }

    /** Return a pointer to the U32 Type object. */
    PrimitiveType *get_u32()  { return u32.get();   }

    /** Return a pointer to the U64 Type object. */
    PrimitiveType *get_u64()  { return u64.get();   }

    /** Return a pointer to the I8 Type object. */
    PrimitiveType *get_i8()   { return i8.get();    }

    /** Return a pointer to the I16 Type object. */
    PrimitiveType *get_i16()  { return i16.get();   }

    /** Return a pointer to the I32 Type object. */
    PrimitiveType *get_i32()  { return i32.get();   }

    /** Return a pointer to the I64 Type object. */
    PrimitiveType *get_i64()  { return i64.get();   }

    /** Return a pointer to the F64 Type object. */
    PrimitiveType *get_f64()  { return f64.get();   }

    /** Return a pointer to the Bool Type object. */
    PrimitiveType *get_bool() { return boolt.get(); }

    /**
    Create or retrieve a class with the name `name`.

    Returns `nullptr` if a type with `name` is already declared, but is not a class.
    */
    ClassType *get_class(Location decl_loc, std::string name, sema::sym::Scope *scope);

    /**
    Create an anonymous class.
    */
    ClassType *get_class(Location decl_loc, sema::sym::Scope *scope);

    /**
    Create or retrieve a union with the name `name`.

    Returns `nullptr` if a type with `name` is already declared, but is not a union.
    */
    UnionType *get_union(Location decl_loc, std::string name, sema::sym::Scope *scope);

    /**
    Create an anonymous union.
    */
    UnionType *get_union(Location decl_loc, sema::sym::Scope *scope);

    /*
    Create or retrieve an enum with the name `name`.

    Returns `nullptr` if a type with `name` is already declared, but is not an enum.
    */
    EnumType *get_enum(Location decl_loc, std::string name, sema::sym::Scope *scope);

    // Create an anonymous enum.
    EnumType *get_enum(Location decl_loc, sema::sym::Scope *scope);

    // Create a pointer with the given `base` type.
    PointerType *get_pointer(Type *base, bool is_const);

    // Create or get an array with the given `base` type and specified size.
    ArrayType *get_array(Type *base, uint64_t size);

    // Create or get an array with the given `base` type and no specified size.
    ArrayType *get_array(Type *base);

    // Create a function type based on its signature.
    FunctionType *get_function(Type *ret, Vec<Type *> params, bool variadic);

    /**
    Finalize all primitive types.
    */
    void finalize_primitives();

    /**
    Finalize all user-defined types.
    */
    void finalize_usertypes();

    /**
    Finalize all function types.
    */
    void finalize_functions();

    /**
    Finalize all function types.
    */
    void finalize_pointers();

    /**
    Finalize all array types.
    */
    void finalize_arrays();

    std::string to_string() const;

private:
    int anonymous_ctr = 0;
    codegen::LLVMUnit& llvm;

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


    template<typename T>
    struct pair_hash {
        std::size_t operator() (const std::pair<Type *, T> p) const {
            auto h1 = std::hash<Type *>{}(p.first);
            auto h2 = std::hash<T>{}(p.second);

            return h1 ^ h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2);
        }
    };

    using ArrayKey = std::pair<Type *, Optional<uint64_t>>;
    using PointerKey = std::pair<Type *, bool>;

    // The map of record types (i.e. structs, unions, enums).
    std::unordered_map<std::string, Box<UserType>> user_types;

    // The map of function types.
    std::unordered_map<std::string, Box<FunctionType>> function_types;

    // The map of pointer types, mapped by their base type.
    std::unordered_map<PointerKey, Box<PointerType>, pair_hash<bool>> pointers;

    // The map of array types, mapped by their base type (todo: add size)
    std::unordered_map<ArrayKey, Box<ArrayType>, pair_hash<Optional<uint64_t>>> arrays;

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
}; // class TypeContext

}

#endif