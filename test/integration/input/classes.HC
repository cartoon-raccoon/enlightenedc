class Opaque;

Opaque *opaque_ptr;

public class Point {
    I32 x;
    I32 y;
};

union Data {
    U32 i;
    F64 f;
};

public U8 union U8U {
    U8 u8[1];
};

public U16 union U16U {
    U8   u8[2];
    U16 u16[1];
};

public U32 union U32U {
    U8   u8[4];
    U16  u16[2];
    U32  u32[1];
};

public U64 union U64U {
    U8   u8[8];
    U16  u16[4];
    U32  u32[2];
    U64  u64[1];
};

enum Color {
    RED,
    GREEN = 5,
    BLUE
};

// Enum with an explicit underlying primitive type (only the default
// underlying type is exercised by Color above).
U8 enum SmallFlags {
    FLAG_NONE,
    FLAG_A,
    FLAG_B
};

"%d\n", FLAG_A;

class Animal {
    I32 age;
};

class Dog : Animal {
    enum Color coat;
};

enum Forward;

class Flags {
    U8 a : 1;
    U8 b : 2;
    U8 : 5;
};

class Value {
  I32 type;
  union {
    F64 f;
    U8 ch;
  } u;
};

class VTable {
    Void (*dispatcher) (U64 count);
};

class Value val;

val.u.ch = 23;

class VTable vtable;

"%d\n", val.u.ch;

vtable.dispatcher(69);