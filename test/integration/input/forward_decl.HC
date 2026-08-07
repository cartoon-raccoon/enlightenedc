// Forward declaration; not defined until later in the file.
I32 Square(I32 x);

// Calls Square() before its definition appears below — this can only resolve
// correctly because the forward declaration above already registered the symbol.
I32 SquareAndAdd(I32 x, I32 y) {
    return Square(x) + y;
}

// The actual definition, reconciling with the forward declaration above.
I32 Square(I32 x) {
    return x * x;
}

// A redundant re-declaration after the definition; should be a no-op.
I32 Square(I32 x);

// A pure external declaration, never defined in this translation unit.
extern "C" I32 ExternallyDefined();

Void Main() {
    "%d\n", Square(5);
    "%d\n", SquareAndAdd(3, 4);
    ExternallyDefined();
}

Main();
