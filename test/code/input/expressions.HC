U32i x = 0;
U32i a = 1;
U32i b = 2;

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

x = (U32i) a + 2;
"%d\n", x;   // 3

x = (I32i) a;
"%d\n", x;   // 1

++x;
"%d\n", x;

x--;
"%d\n", x;

x += sizeof(U32i);
"%d\n", x;

x += sizeof x;
"%d\n", x;

x = (a, b, x);
"%d\n", x;