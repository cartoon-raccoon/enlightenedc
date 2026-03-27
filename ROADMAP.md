# Roadmap

This document lays out the planned improvements and updates for the EnlightenedC project.

## Short-Term

- Fully implement compilation pipeline
- Implement missing features (e.g. class inheritance, default arguments, etc.)
- Implement Range expressions

## Medium-Term

- Implement various optimizations using CFG, etc.
- Implement a basic stdlib
- Add the JIT REPL

## Long-Term

- Transition off Flex/Bison to a handwritten lexer/parser
- Transition off calling `cpp` to a handwritten preprocessor
- Invoke linker using `lld` instead of calling Clang
- Flesh out stdlib