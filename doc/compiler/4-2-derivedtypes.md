# Derived Types (`DerivedType`)

These are the types that are type constructors, i.e. they take some type as a base and use that base to create a concrete type. This is a concept deeply rooted in type theory. For example, a pointer cannot stand on its own, it must take another type to be its pointee. Similarly, arrays and functions all take a some sort of base (and in the case of functions, other types for parameters) to create concrete type.

All `DerivedTypes` take a single `Type` as their base, from which they are constructed. This does not have to be a `BaseType`; pointers can recursively point to pointers (`Void **`), and arrays can be multi-dimensional (arrays of arrays). For function types (which take multiple types as type parameters), their base type is their return type.

The single concrete `DerivedType` is `PointerType`. A `PointerType` is always complete, even if its base is not, since its size is always known. However, in order to compute pointer stride (for pointer arithmetic), its base must be complete.

`PointerType` objects are stored in a hash map keyed by the base type. Since pointer equality is type equality (see the interning invariant), `Type *`s are provably unique keys that can be used as hash map keys.

## Decayable Types (`DecayableType`)

These are types that implicitly convert to another type when in certain syntactic positions. For example, an identifier used to declare a function has a function type:

```holyc
U0 Foo(U32 a, I8 b) {}
```

The symbol `Foo` has type `Void (U32, I8)`. However when used as an rvalue (such as a assigning to a variable), it implicitly converts to a function pointer `Void (*) (U32, I8)`. Similarly, arrays implicitly convert to pointers in some rvalue positions (e.g. as function arguments). `DecayableType` encodes this functionality.

`DecayableType` defines a pure `decay()` method that returns a `PointerType`, since all decayable types decay into pointers (functions into function pointers, `T[]` into `T *`).

The two concrete `DecayableType`s are `ArrayType` and `FunctionType`.

### Array Types (`ArrayType`)

These are C-style arrays that hold multiple items of the same type. An array is uniquely identified by its base and its size. Arrays are complete with a size is provided, and are incomplete without a size. Unsized arrays are valid in EnlightenedC, but they must always decay to a sized type (i.e. a pointer) where they are used.

Arrays are stored in a hash map, keyed by an `ArrayKey` type, which is a `Pair<Type *, Optional<uint64_t>>`.

#### Array Ref-Counting

Arrays are unique in EnlightenedC (and all C-family languages) in that they can be declared unsized, and later assigned a size through size inference. However, we can't just set the size on the corresponding `ArrayType`, since it is a singleton object that represents *the* unsized `ArrayType` of its base. As such, we have to create a *new* `ArrayType` object of that size and base. This often results in leftover unsized `ArrayType`s with nothing pointing to them.
Unsized arrays cannot be finalized, and attempting to finalize one results in an ICE. Since validation will catch any illegal uses of unsized arrays, and most finalization is on demand at code generation time (after validation), in theory no unsized array should make it to finalization. However, to save space and avoid as much artifacts as possible, `ArrayType` uses a basic ref-counting scheme that deletes unsized array objects if nothing else holds a pointer to them. See `TypeContext::decay_array`, `TypeContext::decay_array_ref`, and `TypeContext::deallocate_unsized_array` for details and implementation.

### Function Types (`FunctionType`)

A `FunctionType` is an abstract type that represents the signature and language linkage of a given function. While a function as a type might seem strange, it is a useful representation when assessing whether a function can be assigned to a given function pointer.

A `FunctionType` is uniquely identified by two main things: its signature (represented by the `FunctionSignature` struct), and its language linkage (e.g. `extern "C"`). Since two distinct `FunctionType`s can have the same signature (differing only by language linkage), signatures are interned separately, and each `FunctionType` holds a pointer to it. Unlike the other derived types, which are keyed on their base types, `FunctionTypes` are keyed on a string, constructed from their signature hash.

(todo)

## `TypeBuilder`

(todo)
