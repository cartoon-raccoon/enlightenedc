# Roadmap

This document lays out the planned improvements and updates for the EnlightenedC project.

## Important To-dos

These are to-dos that are important, but do not constitute a milestone.

- Create a symbol table for CFG (`CFGSymbolTable`)
- Turn the std::string on `cfg::Named` into a `CFGSymbol`
- Make the lexer hack scoped; add (scoped) typedef as its own AST node
- Factor out constexpr parsing into its own node
- Implement insertion of memmove, memset
- Implement AST/MIR matchers
- More granular location tracking (per operator, individual class parents, etc)
- Add options to integration tests to control what phase to stop compilation at, or dump mir, dump AST, etc.

## Short-Term

- Fully implement compilation pipeline
  - Validator, codegen, MVP test on Brainfuck
- Implement missing features
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
- Add relational operator evaluation chaining (a < b < c instead of (a < b) < c)
- Add designator-chain initializers (`{ .foo[idx].bar = 6 }`)
- Add inline assembly
- Finalize the ABI/runtime interface

## Long-Term

- Add freestanding mode to support OSDev
- Transition off Flex/Bison to a handwritten lexer/parser
- Transition off calling `cpp` to a handwritten preprocessor
- Invoke linker using `lld` instead of calling Clang
- Flesh out stdlib
- Implement an LSP server
- Translate the implementation into a concrete spec
