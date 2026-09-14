# Standards

Two standards for EnlightenedC are defined: `HolyC` and `EnlightenedC`. These determine the calling
conventions and structure of various parts of the language, in whether to support HolyC constructs,
or EnlightenedC constructs.

## Preprocessor Macros

If the `HolyC` standard is in use, the preprocessor macro `__EC_STD_HOLYC` shall be defined. Otherwise,
if the `EnlightenedC` standard is in use, the preprocessor macro `__EC_STD_ENLIGHTENEDC` shall be defined.

If both macros are simultaneously defined, that behaviour is undefined.

## Variadic Functions

Variadic functions have different specifications depending on the standard in use. This also affects the
calling convention used.

### Variadic Argument Manipulation

Regardless of standard, the following symbols shall be defined:

```holyc
__ec_builtin_va_start();
__ec_builtin_va_end();
__ec_builtin_va_next();
```

(Todo: match this to the C `va` macros)

### HolyC

Under the `HolyC` standard, two symbols shall be defined within the body of a variadic function:
`argc` and `argv`. `argc` shall be an unsigned integer dependent on the platform pointer width, and
`argv` shall have the type `Void **`. A user-defined `argc` and/or `argv` shall cause the corresponding
implicit variable to be shadowed. A diagnostic may be issued.

`argc` shall contain the number of *variadic* arguments passed to the function, and `argv` shall be an array of pointers to variadic arguments passed to the function. All non-variadic arguments shall be defined in the function's signature, and accessed from there.

#### Example

Given a function defined as such:

```holyc
Void someVariadicFunction(U32 nonVarArg1, I64 nonVarArg2, ...);
```

And given a call to the function as follows:

```holyc
someVariadicFunction(nonVarArg1, nonVarArg2, varArg1 /*U32*/, varArg2 /*U64*/);
```

The environment of the function shall be:

```text
argc = 2
argv = Void ** // pointer to varArg1 *
```

### EnlightenedC

Under the `EnlightenedC` standard, the standard C method of accessing variadic arguments shall be used.
(Todo)

## Storage Class Specifiers

The `public` storage-class specifier shall only be accepted under the `HolyC` standard. Under this standard, `public` and `extern` shall have equivalent semantics.
