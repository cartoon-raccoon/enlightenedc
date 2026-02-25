U32i add(U32i a, U32i b) {
    return a + b;
}

public I32i compute(I32i x = 10, I32i y = 20, ...) {
    return x * y;
}

compute(10, 4, "this", true);

U0i noop() {
    return;
}

noop();

noop;

I64i CallbackFunction(I64i x)
{
  return x * x;
}

U0i MathFunc(I64i y, I64i (*proc)(I64i _val))
{
  return proc(y);
}

U0i Main()
{
  I64i x = MathFunc(10, &CallbackFunction);
  "%d\n",x;
}
