public class Point {
    I32i x;
    I32i y;
};

union Data {
    U32i i;
    F64i f;
};

public U8i union U8 {
    U8i u8[1];
};

public U16i union U16 {
    U8   u8[2];
    U16i u16[1];
};

public U32i union U32 {
    U8   u8[4];
    U16  u16[2];
    U32i  u32[1];
};

public U64i union U64 {
    U8   u8[8];
    U16  u16[4];
    U32  u32[2];
    U64i  u64[1];
};

enum Color {
    RED,
    GREEN = 5,
    BLUE
};

class Animal {
    I32i age;
};

class Dog : Animal {
    enum Color coat;
};

enum Forward;

class Flags {
    U8i a : 1;
    U8i b : 2;
    U8i : 5;
};

class Value {
  I32i type;
  union {
    F64i f;
    U8i ch;
  } u;
};

class VTable {
    Void (*dispatcher) (U64i count);
};

class Value val;

val->u.ch = 23;

class VTable vtable;

vtable->dispatcher(69);