# Expressions

(TODO: add all expressions eventually)

## Reinterpret Expressions

This is an adaptation of HolyC's union-based type-punning. See [section 1](1-holyc-differences.md) for details.

EnlightenedC adapts this into the *Reinterpret Expression*. The syntax is: `expression <dot> <primitive type>`. It allows primitive types to be punned as an array of smaller primitive types.

```holyc
42.I32; // reinterprets the literal 42 as an array of I32s
42.I32[1]; // the resulting array can be indexed
```
