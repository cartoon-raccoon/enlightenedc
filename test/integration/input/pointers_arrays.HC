class Node {
    I32 value;
    class Node *next;
};

I32 apply(I32 (*fn)(I32), I32 v) {
    return fn(v);
}

I32 square(I32 x) {
    return x * x;
}

// Should infer U32[6]
U32 inferred_arr[] = {5, 4, 3, 2, 1, 0};

U32 complex_arr[][] = {{5, 4}, {6, 4}, {1}};

U32 arr[5];

U32 *ptr_array[5];

U32 (* ptr_to_arr) [5];

U32 *(*ptr_to_ptr_arr) [5];

I32 run() {
    Node n;
    Node *p = &n;

    n.value = 5;
    p->value++;

    I32 result = apply(square, n.value);
    "%d\n", result; // expect 36

    return result;
}

run();

// Pointer to an array element, then indexing through the pointer (none of
// arr/ptr_array/ptr_to_arr/ptr_to_ptr_arr above are ever indexed or written to).
U32 arr2[3];
arr2[0] = 1;
arr2[1] = 2;
arr2[2] = 3;

U32 *ap = &arr2[0];
"%d\n", *ap;
"%d\n", ap[1];

// Pointer to pointer.
I32 v = 5;
I32 *pv = &v;
I32 **ppv = &pv;
"%d\n", **ppv;

// Reinterpret through a pointer (postfix -> primitive_type), as opposed to
// the postfix . primitive_type form exercised in expressions.HC.
U32 rv = 0x12345678;
U32 *rp = &rv;
"%d\n", rp->U16[0];

// A small linked list, chaining member access through a pointer field
// (Node.next is declared but never traversed above).
I32 sum_list() {
    Node a;
    Node b;

    b.value = 2;
    a.value = 1;
    a.next = &b;

    return a.value + a.next->value;
}

"%d\n", sum_list(); // expect 3

// Array-to-pointer decay in a plain variable initializer (as opposed to a
// binary-expression operand, assignment right-hand side, or call argument,
// all exercised above).
U32 *decayed_ptr = arr2;
"%d\n", *decayed_ptr;

// Pointer compound-assignment arithmetic (`ptr += n`), as opposed to the
// plain `ptr + n` form (see *ap / ap[1] above).
ap += 2;
"%d\n", *ap;

// Explicit primitive<->pointer casts through a width other than the
// pointer-width integer type (only that exact width can reinterpret
// directly; any other width -- I32 here, U32 below -- is implicitly bridged
// through it). An implicit int-to-pointer coercion (e.g. `I32 *p = 0;`,
// with no cast) is intentionally still disallowed -- unlike the above, that
// is not a bug.
I32 *nullp = (I32 *) 0;
"%d\n", (U64) nullp;

I32 *nonnullp = (I32 *) 0x4000;
"%d\n", (U64) nonnullp;

U32 raw_addr = (U32) decayed_ptr;
"%d\n", raw_addr != 0;