# Attributes

Attributes are metadata that can be attached to program items to specify to an implementation how they are to be treated or behave. They are to be specified with the syntax `@[<attribute-name>]`.

Attributes may also take arguments, using the syntax `@[<attribute-name> = "value"]`, or with function-style parentheses syntax (`@[<attribute-name>(<value>)]`).

## Specification-Defined Attributes

### `main`

This attribute targets functions, and is used to mark the entry point of a translation unit. In the execution environment, execution of the program shall start from this function, after all top-level program items have run.

Each TU may only have one function marked `@[main]`. A TU with more than one function marked `@[main]` is considered ill-formed. Types marked with this attribute are considered ill-formed.

Under a hosted environment, a function marked with this attribute shall have a return type of either `Void` or `I32`, and have one of the following signatures:

```holyc
Void ();

I32 ();

Void (I32 argc, I8 *argv[]);

Void (I32 argc, I8 *argv[], I8 *env[]);

I32 (I32 argc, I8 *argv[]);

I32 (I32 argc, I8 *argv[], I8 *env[]);
```

If present, the first argument shall be populated with the number of arguments passed to the program, and the second argument shall be a pointer to the arguments of the program. The third argument is optional, but if present, is implementation defined.

Under a freestanding environment, the signature of a function marked with this attribute is unspecified.

### `print`

This attribute targets functions, and is used to mark a function to be used as the target for `print-statement`s. It shall have the following signature:

```holyc
extern "C" Void (I8 *, ...);
```

All `print-statements` shall resolve to a call to this function during the execution of a program. A function marked with this attribute without this signature is considered ill-formed.
