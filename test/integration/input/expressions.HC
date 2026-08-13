U32 x = 0;
U32 a = 1;
U32 b = 2;

x = a + b * 3 - 4 / 2 % 5;
"%d\n", x;   // expect 1 + 6 - (4/2 % 5 = 2 % 5 = 2) = 1 + 6 - 2 = 5

x <<= 2;
"%d\n", x;   // 5 << 2 = 20

x |= 1 ^ 3 & 7;
"%d\n", x;

a = 2 ? 1 : 5;
"%d\n", a;   // 1

x = (a > b) ? a : b;
"%d\n", x;   // 2

x = a && b || a;
"%d\n", x;   // 1

x = (U32) a + 2;
"%d\n", x;   // 3

x = (I32) a;
"%d\n", x;   // 1

++x;
"%d\n", x;

x--;
"%d\n", x;

x += sizeof(U32);
"%d\n", x;

x += sizeof x;
"%d\n", x;

x = (a, b, x);
"%d\n", x;

x.U16[1] = 0;

// Equality/relational operators not exercised above.
U32 y = 6;
U32 z = 9;

"%d\n", y == 6;    // 1
"%d\n", y != z;    // 1
"%d\n", y <= 6;    // 1
"%d\n", z >= 10;   // 0

// Bare bitwise/shift operators (only their compound-assignment forms are used above).
"%d\n", 3 << 2;    // 12
"%d\n", 40 >> 3;   // 5

// Unary ~, !, and + (only unary - is exercised above, via negative literals).
y = ~y;
"%d\n", y;   // ~6

y = !y;
"%d\n", y;   // 0, y was nonzero

y = !0;
"%d\n", y;   // 1

I32 neg = -5;
I32 pos = +neg;
"%d\n", pos;   // -5

// Remaining compound-assignment operators.
z >>= 2;
"%d\n", z;   // 9 >> 2 = 2

z <<= 3;
"%d\n", z;   // 2 << 3 = 16

z &= 6;
"%d\n", z;   // 16 & 6 = 0

z |= 5;
z ^= 3;
"%d\n", z;   // (0 | 5) ^ 3 = 6

z *= 4;
"%d\n", z;   // 24

z /= 5;
"%d\n", z;   // 24 / 5 = 4

z %= 3;
"%d\n", z;   // 4 % 3 = 1

// Nested/chained ternary.
I32 t = 1 ? 2 ? 3 : 4 : 5;
"%d\n", t;   // 3