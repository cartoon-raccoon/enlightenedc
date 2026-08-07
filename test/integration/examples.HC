// HolyC General Test File


// Preprocessor

#define CONST_A 10
#define CONST_B 0xFF
#define TRUE true
#define FALSE false


// Storage classes + primitive types + const

public  I64 g_public = 1;
static  I32 g_static = 2;
extern  I64 g_extern;

const U32 g_const = 42;

U8   u8v  = 255;
U16  u16v = 65535;
U32  u32v = 1000;
U64  u64v = 100000;

I8   i8v  = -1;
I16  i16v = -2;
I32  i32v = -3;
I64  i64v = -4;

F64  f64v = 1.23e-4;
Bool bv   = TRUE;
Void *vp;


// Structs (forward, named, anonymous, bitfield)

class Node;

class Point {
    I64 x;
    I64 y;
};

class Bits {
    I64 a : 3;
    I64 : 2;
    I64 b : 4;
};

class {
    I32 anon_field;
} anon_class;


// Union

union Value {
    I64 i;
    F64 f;
};


// Enums

enum Color { RED, GREEN = 5, BLUE };

enum { A, B, C };

enum Forward;


// Arrays + Initializers

I64 arr1[3] = {1,2,3};
I64 arr2[3] = {4,5,6,};
I64 arr3[];


// Pointers + complex declarators

I64 *ptr;
I64 * const const_ptr = &i64v;

I64 (*func_ptr)(I64);


// Functions

I64 Add(I64 a, I64 b) {
    return a + b;
}

I64 DefaultParam(I64 x = 5) {
    return x;
}

I64 Variadic(I64 x, ...) {
    return x;
}

Void VoidFunc() {
}

I64 (*ReturnFuncPtr())(I64) {
    return Add;
}


// Compound statement with declarations + statements

{
    I64 local = 5;
    F64 localf = 2.5;

    local += 2;
    local *= 3;
    local <<= 1;
    local &= 7;
}


// Control Flow

I64 control = 2;

if (control > 1 && control < 10 || FALSE)
    control = 1;
else
    control = 0;

switch (control) {
    case 0:
        break;
    case 1 ... 5:
        control++;
        break;
    default:
        control = -1;
}


// Loops

I64 i = 0;

while (i < 3) {
    i++;
}

do {
    i--;
} while (i > 0);

for (i = 0; i < 5; i++) {
    if (i == 2)
        break;
}


// Goto + label

goto after_label;

label_here:
i = 100;

after_label:
i = 200;


// Expressions and operators

I64 a = 5;
I64 b = 2;

I64 arith  = a + b * 3 - 1 / 1 % 2;
I64 shift  = a << 1 >> 1;
I64 bitops = (a | b) ^ (a & b);
I64 rel    = a >= b;
I64 eq     = a == b;
I64 neq    = a != b;
I64 tern   = a > b ? a : b;

a += 1;
a -= 1;
a *= 2;
a /= 2;
a %= 2;
a |= 1;
a ^= 1;
a &= 1;
a <<= 1;
a >>= 1;

++a;
--a;
a++;
a--;

I64 comma = (a = 1, b = 2, a + b);


// Pointer and member access

class Point p;
p.x = 1;
p.y = 2;

ptr = &p.x;
*ptr = 10;

class Point *pp = &p;
pp->y = 20;


// sizeof

I64 sz1 = sizeof a;
I64 sz2 = sizeof(I64);
I64 sz3 = sizeof(class Point);


// Constants

I64 hexv = 0xABCD;
I64 octv = 0777;
I64 negv = -123;

U8 charv = 'A';


// Standalone print statement

"value:", a, b, "\n";


// Function usage

I64 result = Add(3,4);
func_ptr = Add;
result = func_ptr(5);


// Empty statement

;
