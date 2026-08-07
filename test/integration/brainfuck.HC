#define DEFAULT_DATA_SIZE 30000
#define DEFAULT_CODE_SIZE 1024

#define DEFAULT_STACK_CAP 32

U0 Execute();

/* a stack to store the position of opening brackets. */
class Stack {
    I32 cap;
    I32 len;
    I32 data[DEFAULT_STACK_CAP];
};

Stack nesting = { DEFAULT_STACK_CAP, 0 };

U0 StackPush(I32 item) {
    /* if item is less than zero, fail silently */
    if (item < 0) return;

    if (nesting.len + 1 > nesting.cap) {
        // no realloc
        return;
    }

    nesting.data[nesting.len] = item;
    nesting.len++;
}

I32 StackPeek() {
    if (nesting.len <= 0)
        return -1;

    return nesting.data[nesting.len - 1];
}

I32 StackPop() {
    if (nesting.len <= 0) return -1;

    I32 ret = nesting.data[nesting.len - 1];
    nesting.len--;

    return ret;
}

/* our code */
I8 code[DEFAULT_CODE_SIZE];
I32 insptr = 0;
I32 codesize = DEFAULT_CODE_SIZE;

/* our data */
I8 data[DEFAULT_DATA_SIZE];
I32 dataptr = 0;

enum ErrType {SYSTEM = 1, BAD_CODE = 2};
U0 Die(I8 *errmsg, enum ErrType errcode) {
    if (errcode == SYSTEM) {
        "ERROR: %s\n", errmsg;
    } else {
        "ERROR@[Pos %d: '%c']: %s\n", insptr, code[insptr], errmsg;
    }
    return;
}

I8 valid_chars[8] = {'>', '<', '+', '-', ',', '.', '[', ']'};
Bool IsValid(I8 c) {
    I32 i = 0;
    while (i < 8) {
        if (c == valid_chars[i]) return 1;
        i++;
    }

    return 0;
}

// code from string
U0 ReadCode(I8 *src) {
    I32 n = 0;
    I32 i = 0;

    while (src[i] != '\0') {
        if (IsValid(src[i])) {
            code[n++] = src[i];
        }

        if (n + 1 == codesize) {
            Die("code too large", SYSTEM);
            return;
        }
        i++;
    }

    code[n] = '\0';
    "got code: (", code, ")\n";
}

/* Jumps to the command after the matching closing bracket. */
U0 JumpToAfter() {
    I32 current = nesting.len;
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
            Die("malformed code, missing matching bracket", BAD_CODE);
            return;
        }

        insptr++;
    }
}

U0 Execute() {

    I8 c;
    while ((c = code[insptr]) != '\0') {
        if (insptr < 0) {
            Die("instruction pointer went below 0", BAD_CODE);
            return;
        }

        if (dataptr < 0) {
            Die("data pointer went below 0", BAD_CODE);
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
                    JumpToAfter();
                } else {
                    /* we are entering this bracket, so increment our nesting level */
                    StackPush(insptr);
                }
                break;

            // if current byte is not zero,
            // jump to command after *matching* opening bracket
            case ']': {
                I32 opening = StackPeek();

                if (opening < 0) {
                    Die("mismatched closing bracket", BAD_CODE);
                    return;
                }

                if (data[dataptr] != 0) {
                    insptr = opening;
                } else {
                    /* exit our loop, go down one level of nesting */
                    StackPop();
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

U0 Initialize() {
    I32 i = 0;

    while (i < DEFAULT_DATA_SIZE) {
        data[i] = 0;
        i++;
    }

    nesting.len = 0;
}

U0 PrintUsage(I8 *argname) {
    "usage: ", argname, " [-m MEMSIZE] CODE";
}

I32 Main() {

    Initialize();
    // hello world
    I8 program[] = ">++++++++[<+++++++++>-]<.>++++[<+++++++>-]<+.+++++++..+++.>>++++++[<+++++++>-]<++.------------.>++++++[<+++++++++>-]<+.<.+++.------.--------.>>>++++[<++++++++>-]<+.";

    ReadCode(program);
    Execute();

    return 0;
}

Main();