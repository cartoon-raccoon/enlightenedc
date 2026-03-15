// HolyC General Test File


// Preprocessor

#define CONST_A 10
#define CONST_B 0xFF


// Storage classes + primitive types + const

public  I64i g_public = 1;
static  I32i g_static = 2;
extern  I64i g_extern;

const U32i g_const = 42;

U8i   u8v  = 255;
U16i  u16v = 65535;
U32i  u32v = 1000;
U64i  u64v = 100000;

I8i   i8v  = -1;
I16i  i16v = -2;
I32i  i32v = -3;
I64i  i64v = -4;

F64i  f64v = 1.23e-4;
Bool bv   = TRUE;
U0i *vp;


// Structs (forward, named, anonymous, bitfield)

class Node;

class Point {
    I64i x;
    I64i y;
};

class Bits {
    I64i a : 3;
    I64i : 2;
    I64i b : 4;
};

class {
    I32i anon_field;
} anon_class;


// Union

union Value {
    I64i i;
    F64i f;
};


// Enums

enum Color { RED, GREEN = 5, BLUE };

enum { A, B, C };

enum Forward;


// Arrays + Initializers

I64i arr1[3] = {1,2,3};
I64i arr2[3] = {4,5,6,};
I64i arr3[];


// Pointers + complex declarators

I64i *ptr;
I64i * const const_ptr = &i64v;

I64i (*func_ptr)(I64i);


// Functions

I64i Add(I64i a, I64i b) {
    return a + b;
}

I64i DefaultParam(I64i x = 5) {
    return x;
}

I64i Variadic(I64i x, ...) {
    return x;
}

U0i VoidFunc() {
}

I64i (*ReturnFuncPtr())(I64i) {
    return Add;
}


// Compound statement with declarations + statements

{
    I64i local = 5;
    F64i localf = 2.5;

    local += 2;
    local *= 3;
    local <<= 1;
    local &= 7;
}


// Control Flow

I64i control = 2;

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

I64i i = 0;

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

I64i a = 5;
I64i b = 2;

I64i arith  = a + b * 3 - 1 / 1 % 2;
I64i shift  = a << 1 >> 1;
I64i bitops = (a | b) ^ (a & b);
I64i rel    = a >= b;
I64i eq     = a == b;
I64i neq    = a != b;
I64i tern   = a > b ? a : b;

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

I64i comma = (a = 1, b = 2, a + b);


// Pointer and member access

class Point p;
p.x = 1;
p.y = 2;

ptr = &p.x;
*ptr = 10;

class Point *pp = &p;
pp->y = 20;


// sizeof

I64i sz1 = sizeof a;
I64i sz2 = sizeof(I64i);
I64i sz3 = sizeof(class Point);


// Constants

I64i hexv = 0xABCD;
I64i octv = 0777;
I64i negv = -123;

U8i charv = 'A';


// Standalone print statement

"value:", a, b, "\n";


// Function usage

I64i result = Add(3,4);
func_ptr = Add;
result = func_ptr(5);


// Empty statement

;
