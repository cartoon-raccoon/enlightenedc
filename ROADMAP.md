# Roadmap

This document lays out the planned improvements and updates for the EnlightenedC project.

## Short-Term

- Fully implement compilation pipeline
  - Validator, codegen, MVP test on Brainfuck
- Implement missing features
  - class inheritance
  - default arguments
  - bitfields
- Implement Range expressions
  - For use in for-range loops (`for (U32 i : 0...5) {}`)
- Add proper unit testing, aim for >90% codecov
- Add nice error reporting, showing error location and context
- Add concrete config control and CLI args

## Medium-Term

- Implement and properly integrate the JIT REPL
- Implement various optimizations using CFG, etc.
  - Use CFG to report optimizations to the LIRSynthesizer
- Implement a basic stdlib
- Add try-catch (LLVM unwinding)
- Add operation chaining (a < b < c vs (a < b) < c)

## Long-Term

- Transition off Flex/Bison to a handwritten lexer/parser
- Transition off calling `cpp` to a handwritten preprocessor
- Invoke linker using `lld` instead of calling Clang
- Flesh out stdlib
- Implement an LSP server
