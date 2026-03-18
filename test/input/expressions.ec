U32i x = 0;
U32i a = 1;
U32i b = 2;
x = a + b * 3 - 4 / 2 % 5;
x <<= 2;
x |= 1 ^ 3 & 7;

a = 2 ? 1 : 5;
x = (a > b) ? a : b;
x = a && b || a;
x = (U32i) a + 2;
x = (I32i) a;

U8i *s;
s = "hello!";

++x;
x--;
x += sizeof(U32i);
x += sizeof x;

x = (a, b, x);