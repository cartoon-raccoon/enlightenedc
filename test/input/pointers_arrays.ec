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

I32i run() {
    class Node n;
    class Node *p = &n;

    n.value = 5;
    p->value++;

    return apply(square, n.value);
}
