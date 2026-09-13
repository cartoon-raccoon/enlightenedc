# Classes, Unions, and Enums

EnlightenedC also supports user-defined types. These are classes, unions, and enums.

## Declaration and Definition

Like in C, user types can be forward-declared before they are defined further down.

```holyc
// forward declaration
class Shape;

// definition
class Shape {
    U32 x;
    U32 y;
};
```

Once a user type is declared, it is considered *complete*, before which it is known as *incomplete*.
Incomplete types have various limitations; any members that are declared in their definitions cannot be used
until they are defined, and they cannot be used in other types.

For example:

```holyc
// class position declared but not defined
class Position;

class Shape {
    // this is illegal - Position has not been defined
    Position pos;
};

class Position {
    U32 x;
    U32 y;
};
```

## Classes

Classes are similar to structs in C. They represent Plain Old Data (POD), and do not support methods.
They are declared using the `class` keyword, and members are defined like in C:

```holyc
class Shape {
    U32 x;
    U32 y;
};
```

### Initializing Classes

Classes are initialized much like how structs are in C:

```holyc
Shape s = { 5, 6 };
```

#### Designated Initializers

Like in C, member names can be used in the initializer to initialize members out of order:

```holyc
Shape s = { .y = 6, .x = 5 };
```

### Inheritance

Unlike C structs, EnlightenedC classes also support inheritance. Building on the previously defined `Shape`
class:

```holyc
class Rectangle: Shape {
    U32 width;
    U32 height;
};

// create a rectangle of width 4, height 9, at coordinates (5, 6)
Rectangle rect = { 5, 6, 4, 9 };
```

Only single inheritance is supported. HolyC only supported one level of inheritance. EnlightenedC removes that
limit.

## Unions

Unions behave much like C unions, where their members must be accessed one at a time. They essentially
represent a blob of bytes that can be interpreted as the types of their members. They are declared using
the `union` keyword, and are defined with similar syntax to classes:

```holyc
union SomeData {
    U32 asInt;
    I8 *asStr;
};
```

### Type Represented Unions

Like in HolyC where a primitive type placed before a union definition indicated the type it would be treated
as, the same syntax can be used in HolyC:

```holyc
U64 union SomeData {
    U32[2] asIntArray;
    I8 *asString;
};
```

This means that the union can be treated, semantically, as the type it is represented by, and can be used
as that type in any place a primitive type is normally expected:

```holyc
// data is treated implicitly as a U64
SomeData data = 6;

U64 someResult = data + 5;
```

A type-represented union containing a member larger than its type representative is a semantic error:

```holyc
U32 SomeUnion {
    // illegal - U64 is larger than a U32
    U64 wrongSize;
};
```

## Enums

EnlightenedC reintroduces enums, similar to how they work in C. They are declared with the `enum` keyword
and are defined similarly to how C enums are:

```holyc
enum Color {
    // Can assign specific values to enumerators
    RED = 6,
    // Any enumerator without a value is implicitly assigned one
    GREEN,
    BLUE,
};

Color someColor = RED;
```

### Underlying Types

Similar to C enums, EnlightenedC enums are just integers. The exact integer type can be specified by placing
a primitive type before an enum definition, identical to union type representatives:

```holyc
// Color is now underlyingly a U32
U32 enum Color {
    RED = 6,
    GREEN,
    BLUE,
};
```

If no underlying type is given, it defaults to I32.
