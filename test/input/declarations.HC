public const U32i * const *ptrA;
static I16i a, b = -10, c[4];
extern F64i table[];

class Point {
    U32i x, y;
};

Point pt = { .x = 4, .y = 3 };

U8i bytes[3] = {[4] = 4};
I32i matrix[2] = {0, 1};

"%d\n", bytes[0];
"%d\n", bytes[1];
"%d\n", bytes[2];