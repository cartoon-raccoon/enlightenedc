public const U32 * const *ptrA;
static I16 a, b = -10, c[4];
extern F64 table[];

class Point {
    U32 x, y;
};

Point pt = { .x = 4, .y = 3 };

U8 bytes[3] = {[4] = 4};
I32 matrix[2] = {0, 1};

"%d\n", bytes[0];
"%d\n", bytes[1];
"%d\n", bytes[2];