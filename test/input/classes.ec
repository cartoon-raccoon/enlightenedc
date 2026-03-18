public class Point {
    I32i x;
    I32i y;
};

union Data {
    U32i i;
    F64i f;
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