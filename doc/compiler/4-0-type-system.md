# Type System

This chapter of the documentation is dedicated to how `ecc` internally represents EnlightenedC types. While types are not that much of a big deal in Davis' HolyC, EnlightenedC introduces strong, static typing, and so there has to be a robust system for tracking the type of any given expression, so as to ensure that any given operation is legal.

## Architecture

The type system revolves around two main objects: The `Type` class which represents a general type, and the `TypeContext` factory, that produces `Type` objects and hands out pointers to them. These two components allow us to implement an *interning* scheme.

### Type Interning

The entire type system is built around this invariant:

> Type equality <=> `Type *` equality.

That is, if two pointers to a given `Type` are equal, then they are, semantically, the same type. You will see this pattern used everywhere throughout this compiler, where comparisons are performed between `Type *`s to determine semantic type equality.

This is done through the `TypeContext`. This is the global, singleton factory for producing `Type` objects. One `TypeContext` exists per translation unit. It stores each object internally, and hands out pointers to the created object through its `get_<type>` methods. Each `Type` declares `TypeContext` as a friend class, and their constructors are marked `protected`.
This ensures that a `Type` object can never be created outside of a `TypeContext`, and any instance of a `Type` ever created during compilation of a translation unit is owned by `TypeContext` itself.

## The `Type` Object

`Type` is an abstract class that represents a semantic type in EnlightenedC. It is an abstract class that defines the entire `Type` object API, that subclasses override to define their specific functionality. There is a `Kind` enum class that defines a variant for each concrete `Type` subclass, for use with the manual RTTI system, and also for cheap subclass checking.

### Downcasting

Often, you will need to use a `Type` as its specific subclass, to gain access to functionality defined by that subclass. Normally, this would be performed using `dynamic_cast`, but since `ecc` compiles without RTTI, `Type` makes heavy use of virtual functions to perform downcasting.
There are two families of virtual functions for this conversion: the `is_` functions, and the `as_` functions. `is_` functions simply check if the `Type *` is actually of the subclass requested, and `as_` performs the actual conversion, returning `nullptr` if the `Type *` it was called on was not actually that subclass. You will see this pattern used a lot throughout the compiler:

```c++
if (auto *some_type->as_class()) {
    // do something
}
```

or:

```c++
if (some_type->is_primitive()) {
    // do something
}
```

### Effective Type

The `Type` object represents its nominal type, that is, the type it is declared as. Sometimes this type has a different representation in memory. For example, enums are just underlyingly integers, and type-represented unions are also underlyingly whatever type representative they are declared to be. In order for them to validly take part in operations that expect another type, they need to present themselves as what they *effectively* are. This is known as a type's **effective type**.

To enable this, `Type` defines a virtual `effective_type` method, for types to return their effective type. For most types, their effective type is themselves (which is why `effective_type` defaults to identity), but types that have a different effective type override this to return a different type. For example, a union with a type representative returns whatever its representative is, and an enum returns its underlying primitive type (which defaults to I32). Derived types like pointers return themselves, with the base as their base's effective type. For example, a pointer to an `I32`-represented union returns `I32 *` as its effective type.

### Type ID

In addition to pointer identity, types can also be identified by their ID: a stable, deterministic `size_t` obtained from hashing their components. Composite types recursively combine the hashes of their composite types, and derived types apply a "salt" value to their base types. Function types combine the hashes of their signatures and language linkage.

Unlike pointer identity, Type IDs are deterministic and persist across different translation units. This makes them useful for cheaply comparing types between invocations of the compiler, or storing type identity within an LSP server.

### The `Type` API

As mentioned earlier, the `Type` object also defines the API that all its subclasses must implement. Most of it is a host of `is_` predicates (`is_callable`, `is_assignable`, etc). Most of these are self explanatory (read the [header](../../include/semantics/types.hpp) for the full API), but there are a few worth discussing.

#### Type Completeness

Like C, EnlightenedC has the notion of type completeness. This affects how types can be used. For example, a forward declaration of a class (e.g. `class Foo;`) creates an incomplete `ClassType` in the `TypeContext`, and can only be used behind a pointer, (e.g. declaring a variable of type `class Foo` without defining it first is not allowed). `Void/U0` is always incomplete, and so declaring `Void bar;` or `U0 bar;` is also not allowed. Unsized arrays are also incomplete, and variables declared with them have to come with an initializer for array size inference. The `is_complete` virtual method is used to check this.

#### Conversions

Converting between types is of course an extremely common occurrence in any program, no matter the language. There are two types of conversions: implicit (where the compiler inserts the cast for the programmer), and explicit (where the programmer explicitly declares the conversion). This behaviour is encoded using the `coercible_to(Type *)` (for implicit conversions) and `castable_to(Type *)` (for explicit conversions) methods. Each `Type` subclass overrides this to define its own conversion rules, with a single invariant enforced:

> If something is coercible to a given type `T`, then it is also castable to `T`.

Basically, if `coercible_to(T)` returns true, then `castable_to(T)` must also return true.

#### Finalization

Despite all this machinery, at the end of the day, `Type` is still a representation of an abstract concept. In order to be useful, it needs to be convertible to a representation on memory. This is where `finalize()` comes in. This basically hands off the type object to the backend, to convert it to its backend representation for use in code generation. Each `CodeGenUnit` defines a `finalize()` method for each concrete `Type` subclass, and `Type::finalize` simply calls the backend through its back-pointer to the `TypeContext`, which itself holds a reference to the `CodeGenUnit`.

## The `Type` Class Hierarchy

The class hierarchy for `Type` is:

```text
Type (abstract)
 |- BaseType (abstract)
 |   |- VoidType (concrete)
 |   |- PrimitiveType (concrete)
 |   |- UserType (abstract)
 |       |- EnumType (concrete)
 |       |- RecordType (abstract)
 |           |- ClassType (concrete)
 |           |- UnionType (concrete)
 |- DerivedType (abstract)
 |   |- PointerType (concrete)
 |   |- DecayableType (abstract)
 |       |- PointerType (concrete)
 |       |- FunctionType (concrete)
 |- QualifiedType (abstract)
     |- ConstType (concrete)
     |- AtomicType (concrete)
     |- VolatileType (concrete)
```

Each level of the `Type` hierarchy represents certain concepts that are added at that level.

### Subcontents

1. [Base Types](4-1-basetypes.md)
2. [Derived Types](4-2-derivedtypes.md)
3. [Qualified Types](4-3-qualifiedtypes.md)
