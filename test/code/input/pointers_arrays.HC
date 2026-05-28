class Node {
    I32i value;
    class Node *next;
};

I32i apply(I32i (*fn)(I32i), I32i v) {
    return fn(v);
}

I32i square(I32i x) {
    return x * x;
}

// Should infer U32i[6]
U32i inferred_arr[] = {5, 4, 3, 2, 1, 0};

U32i complex_arr[][] = {{5, 4}, {6, 4}, {1}};

U32i arr[5];

U32i *ptr_array[5];

U32i (* ptr_to_arr) [5];

U32i *(*ptr_to_ptr_arr) [5];

I32i run() {
    Node n;
    Node *p = &n;

    n.value = 5;
    p->value++;

    I32i result = apply(square, n.value);
    "%d\n", result; // expect 36

    return result;
}

run();