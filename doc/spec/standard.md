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

### HolyC

Under the `HolyC` standard, two symbols shall be defined within the body of a variadic function:
`argc` and `argv`. `argc` shall be an unsigned integer dependent on the platform pointer width, and
`argv` shall have the type `Void **`. A user-defined `argc` and/or `argv` shall cause the corresponding
implicit variable to be shadowed. A diagnostic may be issued.

### EnlightenedC

Under the `EnlightenedC` standard, the standard C method of accessing variadic arguments shall be used.
(Todo)
