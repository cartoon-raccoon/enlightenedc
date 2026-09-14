# Storage Class Specifiers

The storage class specifiers allowed in EnlightenedC are:

- `extern`
- `public`
- `static`
- `constexpr`
- `typedef`
- `global`

## Constraints

1. At most, one storage class specifier may be given in the declaration specifiers of a declaration, except that:
    - `constexpr` can appear with `static`.
2. An object declared with `constexpr` shall have an initializer, which shall be a constant expression.
3. A block-scope identifier declared with `extern` shall have no prior visible declaration of that identifier.
4. No file-scope identifier shall be declared with `global`.

## Semantics

Storage class specifiers specify various properties of identifiers and declared features:

- Storage duration (`static` in block scope),
- Linkage (`extern`, `static`, `constexpr` in file scope),
- Value (`constexpr`), and
- Type (`typedef`).

An object or function declared with `extern` or `public` at file scope shall have external linkage. The `public` specifier shall only be accepted under the `HolyC` standard, and shall have identical semantics to `extern`.

An object declared with `static` at file scope shall have internal linkage, and an object declared with `static` at block scope shall have `static` storage duration, with no linkage.

An object declared with `constexpr` shall have its value permanently fixed at translation time, and its type shall be implicitly qualified `const`. The declared identifier shall be considered a constant expression.

(`typedef` TBD)
