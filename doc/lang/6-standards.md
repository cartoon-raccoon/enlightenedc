# EnlightenedC Standards

There are two EnlightenedC standards that can be used with `ecc`, that control how certain features of the language appear and can be used, each one with its own useful scenarios. These are the `EnlightenedC` standard and the `HolyC` standard.

## Why two standards?

It has to do with two different, somewhat conflicting goals of this project. Firstly, Terry Davis introduced certain ergonomics that made HolyC useful as a scripting language, that we intend to preserve.
However, this also conflicts with the other goal of this project, which is to make it compatible with C interop and to be able to call into other libraries via the C ABI. Adding a switch to the language to switch between standards gives you the flexibility to choose either.

### Example

The biggest difference between the two standards is the environment of a variadic function. In HolyC, variadic functions defined two implicit variables in the function scope, `argc` and `argv`, which held the variadic argument count and the variadic arguments themselves respectively. You could then loop over these two implicit variables:

```holyc
for (U8 i = 0; i < argc; i++) {
    processs(argv[i]);
}
```

This is very convenient and ergonomic for scripting, as you don't have to wrangle with C's `va_start` and `va_end` macros. However, because `EnlightenedC` doesn't use TempleOS's calling convention (which allowed argc and argv to be populated directly), a function prologue is required to populate `argc` and `argv`, which might not be what you want, especially if it's code you don't want to have run at the start of every variadic function.
Additionally, implementing this requires some changes to the calling convention, which means every function incorporating this ergonomic change becomes incompatible with the C ABI calling convention for variadic arguments. This is where two differing standards become useful, so you can choose what behaviour you want.

## The Standards

`ecc` implements the two standards, and which one to be used is controllable by the `-std` command line argument. `-std=holyc` enables the `HolyC` standard, and `-std=enlightenedc` enables the `EnlightenedC` standard.

Preprocessor macros are also defined to reflect the standard in use, so you can write code that compiles differently under either standard. For example, under the `HolyC` standard, the `__EC_STD_HOLYC` macro is defined.

### The `HolyC` Standard

This implements the ergonomics of `HolyC` as designed by Davis. Under this standard, every variadic function has `argc` and `argv` implicitly inserted into scope. `argc` contains the number of variadic arguments, and `argv` is the array of variadic arguments. Declaring your own `argc` and `argv` causes the corresponding implicit variable to be shadowed and become inaccessible.

When starting `ecc` in REPL mode, `ecc` defaults to `-std=holyc` unless otherwise specified.

### The `EnlightenedC` Standard

This standard implements the C method of accessing variadic arguments. Under this standard, you can use the `va_*` family of macros to access and iterate over the variadic arguments of the function. (TBD)
This standard is fully compatible with the C variadic argument ABI, and should be used if you need your variadic function to be callable from C.

### `extern "C"`

If a function is marked with `extern "C"`, it becomes strictly `EnlightenedC` standard conforming, to ensure it is callable from C. Therefore, no `argc` or `argv` is defined in such functions, even if you are using the `HolyC` standard.
