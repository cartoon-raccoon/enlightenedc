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

U32i arr[5];

U32i *ptr_array[5];

U32i (* ptr_to_arr) [5];

U32i *(*ptr_to_ptr_arr) [5];

I32i run() {
    Node n;
    Node *p = &n;

    n.value = 5;
    p->value++;

    return apply(square, n.value);
}
