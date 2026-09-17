# Roadmap

This document lays out the planned improvements and updates for the EnlightenedC project.

## Important To-dos

These are to-dos that are important, but do not constitute a milestone. They are simply internal or small to-dos that need to be noted down somewhere, and do not correspond to any particular goal timeframe as laid out below. Generally, something noted here means "implement whenever it is needed".

- Nail down semantics for extern and static, implement in CFG
- Create a symbol table for CFG (`CFGSymbolTable`)
- Turn the std::string on `cfg::Named` into a `CFGSymbol`
- Make the lexer hack scoped; add (scoped) typedef as its own AST node
- Make string literals compile-time evaluable, so constexpr and default arguments can accept it
- Make `eval` consistent with Validator; throws where Validator throws, does not throw where Validator doesn't
  - Properly catch EvalSemanticError
- Implement ArrayRef (modeled after `llvm::ArrayRef<T>`)
  - Implement edit distance algorithm (Levenshtein distance, see `llvm::ComputeEditDistance`)
- Implement remaining `llvm::StringRef` API
- Add general I/O framework (modelled after `llvm::RawOStream`) for file and stdio I/O
- Make `@[main]` or `@[print]` alongside `@[link_name]` a hard error
- Add `EnlightenedC` standard and `HolyC` standard differentiation
- Implement insertion of memmove, memset
- Implement AST/MIR matchers
- More granular location tracking (per operator, individual class parents, etc)
- Add options to integration tests to control what phase to stop compilation at, or dump mir, dump AST, etc.

## Short-Term

- Fully implement compilation pipeline
  - Validator, codegen, MVP test on Brainfuck
- Implement missing features
  - bitfields
- Implement Range expressions
  - For use in for-range loops (`for (U32 i : 0...5) {}`)
- Add proper unit testing, aim for >90% codecov
- Add nice error reporting, showing error location and context
- Add concrete config control and CLI args

## Medium-Term

- Implement and properly integrate the JIT REPL
- Implement various optimizations using CFG walkers
- Implement a basic stdlib
- Add try-catch (LLVM unwinding)
- Add relational operator evaluation chaining (a < b < c instead of (a < b) < c)
- Add designator-chain initializers (`{ .foo[idx].bar = 6 }`)
- Add inline assembly
- Finalize the ABI/runtime interface

## Long-Term

- Add freestanding mode to support OSDev
  - Add additional type qualifiers like `atomic` and `volatile`
- Add a handwritten codegen backend (learning track) alongside LLVM
- Transition off Flex/Bison to a handwritten lexer/parser
- Transition off calling `cpp` to a handwritten preprocessor
- Invoke linker using `lld` instead of calling Clang
- Flesh out stdlib
- Implement an LSP server
- Translate the implementation into a concrete spec
