I64 n = 0;

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
        U32 i = 0;
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

for (U32 n = 0; n < 3; n++) "%d\n", n;

for (;;) {
    "loop_once\n";
    break;
}

for (n = 0; n < 3; n++) "%d\n", n;

// continue: skip even values
for (U32 j = 0; j < 5; j++) {
    if (j % 2 == 0) continue;
    "%d\n", j;   // 1, 3
}

// break only exits the innermost loop
U32 outer_count = 0;
for (U32 oi = 0; oi < 3; oi++) {
    for (U32 ii = 0; ii < 3; ii++) {
        if (ii == 1) break;
        outer_count++;
    }
}
"%d\n", outer_count;   // 3

// switch fallthrough: case 1 has no break, so it falls into case 2
switch (1) {
    case 1:
        "fallthrough1\n";
    case 2:
        "fallthrough2\n";
        break;
    case 3:
        "unreached\n";
        break;
}

// forward goto
goto skip;
"unreached\n";
skip:
"after_skip\n";

// multiple declarators in a for-loop initializer, with a comma-expression increment
for (U32 fi = 0, fj = 10; fi < 3; fi++, fj--) {
    "%d %d\n", fi, fj;
}