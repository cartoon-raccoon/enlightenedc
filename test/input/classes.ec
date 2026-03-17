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
    U0i (*dispatcher) (U64i count);
};

val->u.ch = 23;

vtable->dispatcher(69);