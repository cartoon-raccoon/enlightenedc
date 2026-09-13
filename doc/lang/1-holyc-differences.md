# Differences from HolyC

EnlightenedC introduces several new features to HolyC. These include:

- Full type checking and semantic validation
- Removal of the single level inheritance limit present in HolyC
- Addition of `Void` as a keyword type
- Addition of bitfields on classes
- Addition of enums
- Addition of a single-precision 32-bit float `F32`, absent in HolyC
- Addition of `break` and `continue` as control flow keywords
- Support for `extern "C"` syntax, for linking in C functions

Additionally, most features of HolyC that were kept in EnlightenedC are *adaptations* of HolyC features, not wholesale copies. This is because Davis' HolyC compiler did not implement type checking, so any
expression could be used anywhere and the compiler would allow it.
Since EnlightenedC is a production-ready language, the compiler implements type checking, and a lot of features that implicitly existed in HolyC because of the lack of type checking now have to become explicitly supported in EnlightenedC. Consider the ability of HolyC to reinterpret primitive types as byte arrays. This is implicitly supported because of the compiler's lack of type checking. In HolyC, intrinsic primitive types were in fact defined with an `i` suffix, e.g. `U32i`, `I16i`, and the actual types as called in regular code were just unions defined in a header file:

```holyc
// putting U64i before a union declaration causes it to be treated as that type
U64i union U64 {
    U64i u64[1];
    U32  u32[2];
    U16  u16[4];
    U8   u8[8];
};
```

And since the compiler did not support type checking, this syntax was perfectly valid:

```holyc
// declare a union (U64) as a literal 4;
U64 i = 4;
// use a union in a binary expression with another primitive
i + 5;
// access its inner bytes
i.u16[2] = 5;
```

In a language with type checking, this syntax would immediately fail validation, since in standard C, unions cannot be used in binary expressions (only pointers and primitives can). Therefore, introducing type checking to HolyC while still preserving its many useful features requires introducing explicit support for them.
The equivalent syntax in EnlightenedC is a first-class language construct instead of a header definition, known as the reinterpret expression:

```holyc
U64 i = 69;
// expression <dot> <primitive type> reinterprets the expression (of a primitive type) as an array of smaller
// primitive types.
i.U16[3] = 5;
```

Type-represented unions are also explicitly supported in the syntax and type checker. Primitive types in EnlightenedC lose their `i` suffix, and type-represented unions are now a first-class language feature, with similar syntax:

```holyc
// prepending a union definition with a primitive type causes it to be treated as that primitive type,
// and accepted wherever a primitive type is required
U64 union MyU64 {
    U64 u64[1];
    I32 i32[2];
};

MyU64 i = 69; // assign a numeric literal directly to your type-represented union
I32 lowerHalf = i.i32[0]; // member access still works
i += 5; // assignment expressions require a primitive type to work, and `i` works, even though it's a union!
```
