# Application Binary Interface

This section lays out the calling conventions and reserved symbols for the EnlightenedC language.

## Calling Convention

For all non-variadic functions, EnlightenedC functions shall follow the calling convention of the platform
for which they are compiled. For example, EnlightenedC compiled for Linux shall follow the System V ABI.

### Variadic Functions

This changes for variadic functions marked `extern "C"`, as the convention then becomes standard-dependent.
See [Standard](standard.md) for more information on the specifications.

#### `HolyC` Standard

Under the HolyC standard, the first parameter passed to a variadic function shall be the number of arguments
in the call, followed by all non-variadic parameters. This is then used to populate the `argc` implicit symbol.
A function prologue shall also be inserted to populate `argv`.

Functions marked `extern "C"` shall follow the platform-specific ABI for variadic functions. If a
non-`extern "C"` variadic function is called using the platform-specific ABI calling convention, that behaviour
is undefined.

#### `EnlightenedC` Standard

Under the EnlightenedC standard, the standard platform-specific ABI for variadic functions is followed, and
`extern "C"` has no effect.

## Reserved Symbols

Any symbol prefixed with two underscores (`__`) shall be reserved for compiler and standard use. If a
user defines such a symbol that overrides or conflicts with a reserved symbol, that behaviour is undefined.

### `__ec_implicit_main`

This symbol represents the function in which all top-level program items execute. It shall have the following
signature:

```holyc
Void __ec_implicit_main();
```

### `__ec_print`

This symbol represents the function to which all print statements lower to. That is, all print statements
eventually resolve to a call to this symbol. It shall have the following signature:

```holyc
Void __ec_print(I8*, ...);
```
