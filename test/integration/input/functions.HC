extern "C" I32 somefunc();

I32 someinternalfunc(I32);

U32 add(U32 a, U32 b) {
    return a + b;
}

I32 someinternalfunc(I32 somearg) {
    return somearg;
}

public I32 compute(I32 x = 10, I32 y = 20, ...) {
    I32 new;
    new = add(x, y);

    U64 new2 = new + 1;

    return x * y;
}

class What {
    U32 hello;
} complex_class_func(U32 thing) {
    What ret = { thing };

    return ret;
}

class Point {
    I32 x;
    I32 y;
};

Point *compute_point(class Point *p, I32 dx, I32 dy) {
  p->x += dx;
  p->y += dy;

  return p;
}

compute(10, 4, "this", true);

"%d\n", compute(2,3);

// Calls that rely on default arguments being filled in for omitted trailing
// parameters (only the fully-specified call above is exercised otherwise).
"%d\n", compute(5);
"%d\n", compute();

// Recursion.
I32 Factorial(I32 n) {
    if (n <= 1) return 1;
    return n * Factorial(n - 1);
}

"%d\n", Factorial(5);

Void noop() {
    return;
}

// Test call statement
noop();

// Test call statement without parens
noop;

// test assigning to function pointer;
// noop should not expand to a call expr
Void (*funcptr) () = noop;

// Print statement test
"TestPrintStatement";

I64 CallbackFunction(I64 x)
{
  return x * x;
}

I64 MathFunc(I64 y, I64 (*proc)(I64 _val))
{
  return proc(y);
}

@[main]
Void Main(U32 argc, I8 *argv[])
{
  I64 x = MathFunc(10, &CallbackFunction);
  "%d\n",x;
}
