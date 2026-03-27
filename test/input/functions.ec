extern "C" I32i somefunc();

I32i someinternalfunc(I32i);

U32i *add(U32i a, U32i b) {
    return a + b;
}

I32i someinternalfunc(I32i somearg) {
    return somearg;
}

public I32i compute(I32i x = 10, I32i y = 20, ...) {
    I32i new;
    new = add(x, y, new);

    U64i new2 = new + 1;

    return x * y;
}

class What {
    U32i hello;
} complex_class_func(U32i thing) {
    What ret = { thing };

    return ret;
}

class Point {
    I32i x;
    I32i y;
};

Point *compute_point(class Point *p, I32i dx, I32i dy) {
  p->x += dx;
  p->y += dy;

  return p;
}

compute(10, 4, "this", true);

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

I64i CallbackFunction(I64i x)
{
  return x * x;
}

Void MathFunc(I64i y, I64i (*proc)(I64i _val))
{
  return proc(y);
}

Void Main()
{
  I64i x = MathFunc(10, &CallbackFunction);
  "%d\n",x;
}
