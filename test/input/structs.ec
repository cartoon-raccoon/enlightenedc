struct Point {
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

enum Forward;

struct Flags {
    U8i a : 1;
    U8i b : 2;
    U8i : 5;
};

struct Value {
  I32i type;
  union {
    F64i f;
    U8i ch;
  } u;
};