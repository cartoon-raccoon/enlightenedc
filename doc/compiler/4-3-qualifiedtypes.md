# Qualified Types (`QualifiedType`)

`QualifiedType`s represent types that have been marked with a type qualifier, e.g. `const`, `atomic`, or `volatile`. This is represented by the `QualifiedType` abstract class, which sits in its own type hierarchy, and works completely unlike a normal `Type`. A `QualifiedType` composes over a base `Type`, and forwards the entire `Type` API to it. Therefore, `QualifiedType`s inherit the functionality (e.g. conversion rules, assignability rules) defined by their base type, and therefore act completely like their base, unless the `QualifiedType` overrides it (for example, in the case of `const`, assignability is overridden to always return false).

Since `QualifiedType`s take `Type *` as a base, they can also take another `QualifiedType` as a base. This allows for representation of types with multiple qualifiers, such as `const atomic I32`.

In addition to the standard `Type` API, `QualifiedType` adds on the `QualFlags` system, which allows for identification of all the qualifiers on a given type. This is implemented using the `Flags` enum, and is a simple bit flag system. See `QualifiedType::get_base` and the two overloads of `QualifiedType::unqual` for details.

## Qualified Type Ordering

(todo)
