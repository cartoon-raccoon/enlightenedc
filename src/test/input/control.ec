start:
if (n < 10) {
    n++;
} else {
    n--;
}

switch (n) {
    case 1:
        break;
    case 2 ... 5:
        n += 2;
        break;
    default:
        goto start;
}

while (n > 0)
    n--;

do {
    n++;
} while (n < 3);

for (n = 0; n < 5; n++) "hello";