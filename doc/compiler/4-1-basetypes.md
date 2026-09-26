# Base Types (`BaseType`)

These are the types that stand on their own, and do not rely on another type (i.e. they are not type constructors like pointers or arrays are).

The concrete base types are:

- `VoidType`: This is the representation of `Void`/`U0`, to support things like void pointers and void functions. It is always incomplete, so declaring a void array or a variable of void type is not allowed.
- `PrimitiveType`: These are the primitive types in EnlightenedC, such as `U8`. `I16`, `F64`, etc. They are distinguished by a `tokens::PrimType` enum tag, and are always complete.

Concrete base types are stored as singleton objects in the TypeContext itself.

## User Types (`UserType`)

This is a subclass of `BaseType`, and represents the types that are defined by the programmer, which are `enum`s, `class`es, and `union`s. They are incomplete when declared but not defined, and become complete once a definition for them is provided.

The single concrete `UserType` is the `EnumType`, which is a standard C-style enum. Enumerators are assigned a value, and become symbols and constant-evaluable expressions.

User types are stored in a hash map that maps mangled names to their corresponding types.

### `UserType` Namespace and Scope

There are three things that disambiguate a (named) `UserType`: its kind (`class`, `enum`, etc.), its name, and the scope in which it was declared. All `UserType`s share the same namespace, so declaring a `class Foo;` and then declaring an `enum Foo;` in the same scope is invalid. See the spec (TODO) for the rules of user type namespaces and scoping.

To properly disambiguate `UserTypes`, the `TypeContext` doesn't actually store a user type under just its name. It generates a mangled name (usually of the form `<kind>_<name>_<scope id>`, e.g. `class_Foo_42`), and uses that as the hash map key. The type itself carries its programmer-provided name.

Anonymous user types are a different story. EnlightenedC treats all anonymous user types as distinct type objects that cannot be re-referenced. No two anonymous classes, even it they have the exact same layout, are the same type. Therefore, anonymous types only need a unique numeric ID to distinguish them. For this case, the `TypeContext` maintains a monotonic counter for handing out unique IDs, and assigns the type an internal, mangled name like `__ecc_anon_<id>`.

### The `UserType` Lifecycle

A call to the corresponding `get_<usertype>()` method on `TypeContext` creates a forward-declared `UserType`. At this point, it is an incomplete, declaration-only type. Its definition process is controlled by the methods `start()` and `finish()`.

Because it is possible for a user type to remain a declaration throughout the translation unit (i.e. a definition is not compulsory), the start of a definition must be explicitly declared by calling `start()`. This sets an internal flag to mark the type as "in the middle of being defined", allowing us to catch things like nested re-definitions. Once definition is finished (e.g. all members/enumerators have been added), `finish()` is called. This sets the `complete` flag, allowing the type to return `is_complete() = true`. While this is not currently enforced, new members or enumerators should not be added after a type is marked complete. This will become an ICE in the future.

#### Record-Style Types (`RecordType`)

This is an abstract subclass of `UserType`, and represents types containing members that can be accessed using `.` or `->`.

(todo)
