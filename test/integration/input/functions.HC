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
compute();

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

// ---------------------------------------------------------------------------
// Variadic function argument parsing. The single fully-specified call to
// compute() above is the only variadic case otherwise exercised here.
// ---------------------------------------------------------------------------

// Prototype: the ellipsis with no named parameters at all.
I32 v_only_proto(...);

// Definition: the ellipsis with no named parameters at all.
I32 v_only(...) {
    return 0;
}

// Prototype: one named parameter, then the ellipsis.
I32 v_one(I32 a, ...);

// Prototype: several named parameters before the ellipsis.
I32 v_many(I32 a, I32 b, I32 c, ...);

// Prototype: a pointer parameter immediately before the ellipsis.
I32 v_fmt(I8 *fmt, ...);

// Prototype: extern "C" variadic (forces the platform variadic ABI).
extern "C" I32 v_extern(I8 *fmt, ...);

// Definition (body present) rather than a bare prototype.
I32 v_def(I32 a, ...) {
    return a;
}

// A named parameter with a default value, then the ellipsis.
I32 v_default(I32 a = 7, ...) {
    return a;
}

// Variadic function returning a pointer.
I32 *v_ret_ptr(I32 a, ...) {
    return (I32 *) 0;
}

// Variadic function returning a class by value.
class VBox {
    I32 n;
};

VBox v_ret_class(I32 a, ...) {
    VBox b = { a };
    return b;
}

// An array parameter (decays to a pointer) before the ellipsis.
I32 v_arr(I32 xs[], ...) {
    return xs[0];
}

// A pointer to a variadic function used as a parameter type.
I32 v_takes_fp(Void (*cb)(I32, ...), I32 x) {
    return x;
}

// A pointer-to-variadic-function object, then a call through it.
Void v_sink(I32 a, ...) {
    return;
}

Void (*v_fp)(I32, ...) = v_sink;
v_fp(1, 2, 3);

// A pointer to a function that is only an ellipsis, then a call through it.
Void v_bare(...) {
    return;
}

Void (*v_bare_fp)(...) = v_bare;
v_bare_fp(1, 2, 3);

// Calls: an ellipsis-only function, with no arguments and with a tail.
v_only();
v_only(1, 2, 3);
v_only("a", true, 3.5);

// Calls: exactly the named arguments, no variadic tail.
v_one(1);
v_many(1, 2, 3);
v_fmt("x");

// Calls: a variadic tail of assorted types (int, bool, char, float, string).
v_one(1, 2, 3);
v_many(1, 2, 3, 4, 5, 6);
v_fmt("x", 1, true, 'c', 3.5, "y");

// Calls: default parameter filled in, with and without a variadic tail.
v_default();
v_default(9);
v_default(9, 10, 11);

// Call: a variadic call nested inside another call's argument list.
v_def(1, v_def(2, 3));

// Call: array-to-pointer decay of a variadic argument.
I32 nums[3] = { 4, 5, 6 };
v_arr(nums, "a", 'b', 3.0, true);

// Calls: the class-returning and pointer-returning variadic functions.
v_ret_class(1, 2, 3);
v_ret_ptr(1, "x", 'y');

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
