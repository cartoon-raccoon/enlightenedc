I64i n = 0;

start:
if (n < 10) {
    n++;
} else if (n > 11) {
    n--;
} else {
    n += 2;
}

"%d\n", n;

switch (n) {
    case 1:
        "case1\n";
        break;
    case 2 ... 5:
        n += 2;
        "%d\n", n;
        U32i i = 0;
        break;
    default:
        goto start;
}

while (n > 0)
    n--;

"%d\n", n;

do {
    n++;
} while (n < 3);

"%d\n", n;

for (U32i n = 0; n < 3; n++) "%d\n", n;

for (;;) {
    "loop_once\n";
    break;
}

for (n = 0; n < 3; n++) "%d\n", n;