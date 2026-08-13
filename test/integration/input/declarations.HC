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

// Classic declarator gotcha: '*' only binds to the declarator immediately
// following it, so p1 is I32* but p2 is a plain I32, not a pointer.
I32 v1 = 1;
I32 *p1 = &v1, p2 = 2;

"%d %d\n", *p1, p2;

// const-qualified (non-pointer) declaration.
const I32 limit = 100;

"%d\n", limit;

// Trailing comma in an initializer list.
I32 vec[3] = {1, 2, 3,};

"%d %d %d\n", vec[0], vec[1], vec[2];

// Fully-specified 2D array initializer (no designators, unlike matrix above).
I32 grid[2][2] = {{1, 2}, {3, 4}};

"%d %d %d %d\n", grid[0][0], grid[0][1], grid[1][0], grid[1][1];