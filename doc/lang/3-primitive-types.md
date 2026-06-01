# Primitive Types

EnlightenedC, like HolyC, has multiple primitive types, of the format `<type><size>`. For example,
`U64` is an unsigned 64-bit integer, `I32` is a signed 32-bit integer, and `F32` is a 32-bit float.

## The Types

The full list of types are:

- `Void`/`U0` - the Void type, identical to `void` in C.
- `U8` - An 8-bit unsigned integer.
- `I8` - An 8-bit signed integer.
- `U16` - A 16-bit unsigned integer.
- `I16` - A 16-bit signed integer.
- `U32` - A 32-bit unsigned integer.
- `I32` - A 32-bit signed integer.
- `U64` - A 64-bit unsigned integer.
- `I32` - A 64-bit signed integer.
- `F32` - A 32-bit single precision float.
- `F64` - A 64-bit floating point number.

### Machine Size Types

Note that unlike HolyC, which was designed to run exclusively on TempleOS (x64), EnlightenedC is designed
to run on multiple different architectures with different pointer widths. As such, `sizeof` will return
different types based on what architecture you are compiling for, and pointers will decay into different types
as well. Usually this will be 32-bit or 64-bit integers.

### Arrays

Arrays consist of a base type and a size, and are declared much like they are in C.
For example, to declare an array of 5 `U8`s:

```holyc
U8 array[5] = {1, 2, 3, 4, 5};
```

Initialization is also identical to C.

#### Array Designated Initializers

Like in C, you can designate initializers for specific indexes:

```holyc
U8 array[5] = {[1] = 6, [0] = 5};
```

### Pointers

Pointers work very much like in C.

#### Pointer Decay

Like in C, arrays can decay to pointers when context allows it, for example in pointer arithmetic,
or in function calls.

```holyc
Void somefunction(U8 *ptr);

U8 array[10] = {1, 2, 3};

array + 5; // equivalent to array[5];

somefunction(array); // this is valid
```
