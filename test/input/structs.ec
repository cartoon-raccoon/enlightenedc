public class Point {
    I32i x;
    I32i y;
} extern;

union Data {
    U32i i;
    F64i f;
};

enum Color {
    RED,
    GREEN = 5,
    BLUE
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

vtable->dispatcher(69);