#define DEFAULT_DATA_SIZE 30000
#define DEFAULT_CODE_SIZE 1024

#define DEFAULT_STACK_CAP 32

/* a stack to store the position of opening brackets. */
struct stack {
    I32i cap;
    I32i len;
    I32i data[DEFAULT_STACK_CAP];
};

struct stack nesting = { DEFAULT_STACK_CAP, 0 };

void stack_push(I32i item) {
    /* if item is less than zero, fail silently */
    if (item < 0) return;

    if (nesting.len + 1 > nesting.cap) {
        // no realloc
        return;
    }

    nesting.data[nesting.len] = item;
    nesting.len++;
}

I32i stack_peek() {
    if (nesting.len <= 0)
        return -1;

    return nesting.data[nesting.len - 1];
}

I32i stack_pop() {
    if (nesting.len <= 0) return -1;

    I32i ret = nesting.data[nesting.len - 1];
    nesting.len--;

    return ret;
}

/* our code */
U8i code[DEFAULT_CODE_SIZE];
I32i insptr = 0;
I32i codesize = DEFAULT_CODE_SIZE;

/* our data */
U8i data[DEFAULT_DATA_SIZE];
I32i dataptr = 0;

enum errtype {SYSTEM = 1, BAD_CODE = 2};
void die(U8i *errmsg, enum errtype errcode) {
    if (errcode == SYSTEM) {
        "ERROR: ", errmsg;
    } else {
        "ERROR@[Pos ", insptr, ": '", code[insptr], "']: ", errmsg;
    }
    return;
}

U8i valid_chars[8] = {'>', '<', '+', '-', ',', '.', '[', ']'};
Bool is_valid(U8i c) {
    I32i i = 0;
    while (i < 8) {
        if (c == valid_chars[i]) return 1;
        i++;
    }

    return 0;
}

// code from string
void read_code(U8i *src) {
    I32i n = 0;
    I32i i = 0;

    while (src[i] != '\0') {
        if (is_valid(src[i])) {
            code[n++] = src[i];
        }

        if (n + 1 == codesize) {
            die("code too large", SYSTEM);
            return;
        }
        i++;
    }

    code[n] = '\0';
    "got code: (", code, ")\n";
}

/* Jumps to the command after the matching closing bracket. */
void jump_to_after() {
    I32i current = nesting.len;
    while (code[insptr] != '\0') {
        if (code[insptr] == '[') {
            current++;
        } else if (code[insptr] == ']') {
            current--;
            if (current == nesting.len) {
                insptr++;
                return;
            }
        }

        if (current < nesting.len) {
            die("malformed code, missing matching bracket", BAD_CODE);
            return;
        }

        insptr++;
    }
}

void execute() {

    U8i c;
    while ((c = code[insptr]) != '\0') {
        if (insptr < 0) {
            die("instruction pointer went below 0", BAD_CODE);
            return;
        }

        if (dataptr < 0) {
            die("data pointer went below 0", BAD_CODE);
            return;
        }
        "got code: (", code, ")\n";
        switch (c) {
            /* increment data pointer */
            case '>':
                dataptr++;
                break;

            /* decrement data pointer */
            case '<':
                dataptr--;
                break;

            /* increment current byte */
            case '+':
                data[dataptr]++;
                break;

            /* decrement current byte */
            case '-':
                data[dataptr]--;
                break;

            case ',':
                data[dataptr] = 0;
                break;

            case '.':
                data[dataptr];
                break;

            // if the current byte is zero,
            // jump to command after *matching* closing bracket
            case '[':
                if (data[dataptr] == 0) {
                    /* skip this bracket */
                    jump_to_after();
                } else {
                    /* we are entering this bracket, so increment our nesting level */
                    stack_push(insptr);
                }
                break;

            // if current byte is not zero,
            // jump to command after *matching* opening bracket
            case ']': {
                I32i opening = stack_peek();

                if (opening < 0) {
                    die("mismatched closing bracket", BAD_CODE);
                    return;
                }

                if (data[dataptr] != 0) {
                    insptr = opening;
                } else {
                    /* exit our loop, go down one level of nesting */
                    stack_pop();
                }
            } break;
        }

        /* advance instruction pointer */
        insptr++;

        if (insptr + 1 == codesize) {
            return;
        }
    }
}

void initialize() {
    I32i i = 0;

    while (i < DEFAULT_DATA_SIZE) {
        data[i] = 0;
        i++;
    }

    nesting.len = 0;
}

void print_usage(U8i *argname) {
    "usage: ", argname, " [-m MEMSIZE] CODE";
}

I32i main() {

    initialize();
    // hello world
    U8i program[] = ">++++++++[<+++++++++>-]<.>++++[<+++++++>-]<+.+++++++..+++.>>++++++[<+++++++>-]<++.------------.>++++++[<+++++++++>-]<+.<.+++.------.--------.>>>++++[<++++++++>-]<+."

    read_code(program);
    execute();

    return 0;
}