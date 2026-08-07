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