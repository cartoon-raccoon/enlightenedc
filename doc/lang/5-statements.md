# Statements and Control Flow

(Todo: all control flow constructs)

## Top-Level Statements

In HolyC, you could put statements in the top level of a file, outside of any scope, and they would be executed immediately when the file was invoked, similar to a Python script. EnlightenedC inherits this behaviour:

```holyc
I32 i = initializeI32(); // runtime-initialize a global variable

"Value of i is %d\n", i; // print it out
```

## Entry Point

Unlike a language like C, where the entry point was hardcoded as `int main`, HolyC's ability to execute top-level statements meant that there was no real `main` function that could be called. EnlightenedC gives you the ability to name your entry point anything you want, and mark it with the `main` attribute:

```holyc
@[main]
I32 AppEntry() {
    // your entry point logic here
}
```

Any function marked with this attribute becomes the entry point to your program. All top-level statements execute before it gets called, so be careful about execution order!
